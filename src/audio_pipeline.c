#include "audio_pipeline.h"

#include <zephyr/kernel.h>

// fixed-size pool of blocks → avoids malloc, keeps timing predictable
K_MEM_SLAB_DEFINE(audio_block_slab, sizeof(struct audio_block), AUDIO_BLOCK_COUNT, 4);

// FIFO connects producer (capture) to consumer (playback)
K_FIFO_DEFINE(audio_block_fifo);

// protect stats since both threads update them
K_MUTEX_DEFINE(stats_lock);

static struct pipeline_stats stats;
static uint32_t depth;   // current number of blocks in FIFO

int pipeline_block_alloc(struct audio_block **block, k_timeout_t timeout)
{
    return k_mem_slab_alloc(&audio_block_slab, (void **)block, timeout);
}

void pipeline_block_free(struct audio_block *block)
{
    k_mem_slab_free(&audio_block_slab, block);
}

void pipeline_submit(struct audio_block *block)
{
    k_mutex_lock(&stats_lock, K_FOREVER);

    stats.produced++;
    depth++;
    stats.queued = depth;

    // track max queue usage (helps understand buffering behavior)
    if (depth > stats.high_watermark) {
        stats.high_watermark = depth;
    }

    k_mutex_unlock(&stats_lock);

    k_fifo_put(&audio_block_fifo, block);
}

struct audio_block *pipeline_receive(k_timeout_t timeout)
{
    struct audio_block *block;

    block = k_fifo_get(&audio_block_fifo, timeout);
    if (block == NULL) {
        return NULL;   // nothing available
    }

    k_mutex_lock(&stats_lock, K_FOREVER);

    if (depth > 0) {
        depth--;
    }

    stats.consumed++;
    stats.queued = depth;

    k_mutex_unlock(&stats_lock);

    return block;
}

void pipeline_note_capture_miss(void)
{
    // happens when all blocks are in use (pipeline is full)
    k_mutex_lock(&stats_lock, K_FOREVER);
    stats.capture_misses++;
    k_mutex_unlock(&stats_lock);
}

void pipeline_note_sink_timeout(void)
{
    // happens when playback waits but no data is available
    k_mutex_lock(&stats_lock, K_FOREVER);
    stats.sink_timeouts++;
    k_mutex_unlock(&stats_lock);
}

void pipeline_snapshot(struct pipeline_stats *out)
{
    k_mutex_lock(&stats_lock, K_FOREVER);
    *out = stats;
    k_mutex_unlock(&stats_lock);
}

