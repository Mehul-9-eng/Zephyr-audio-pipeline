#include "audio_pipeline.h"

#include <zephyr/kernel.h>

K_MEM_SLAB_DEFINE(audio_block_slab, sizeof(struct audio_block), AUDIO_BLOCK_COUNT, 4);
K_FIFO_DEFINE(audio_block_fifo);
K_MUTEX_DEFINE(stats_lock);

static struct pipeline_stats stats;
static uint32_t depth;

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
        return NULL;
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
    k_mutex_lock(&stats_lock, K_FOREVER);
    stats.capture_misses++;
    k_mutex_unlock(&stats_lock);
}

void pipeline_note_sink_timeout(void)
{
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
