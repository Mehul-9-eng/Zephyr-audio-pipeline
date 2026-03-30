#include "audio_pipeline.h"

#include <zephyr/kernel.h>

// Fixed-size pool of blocks. This avoids malloc, keeps timing predictable
K_MEM_SLAB_DEFINE(audio_block_slab, sizeof(struct audio_block), AUDIO_BLOCK_COUNT, 4);

// separate queues between stages
K_FIFO_DEFINE(capture_to_process_fifo);
K_FIFO_DEFINE(process_to_sink_fifo);

// protect stats since multiple threads update them
K_MUTEX_DEFINE(stats_lock);

static struct pipeline_stats stats;
static uint32_t queue1_depth;
static uint32_t queue2_depth;

int pipeline_block_alloc(struct audio_block **block, k_timeout_t timeout)
{
    return k_mem_slab_alloc(&audio_block_slab, (void **)block, timeout);
}

void pipeline_block_free(struct audio_block *block)
{
    k_mem_slab_free(&audio_block_slab, block);
}

void pipeline_submit_capture(struct audio_block *block)
{
    k_mutex_lock(&stats_lock, K_FOREVER);

    stats.produced++;
    queue1_depth++;
    stats.queue1_depth = queue1_depth;

    // track max queue usage (helps understand buffering behavior)
    if (queue1_depth > stats.queue1_high_watermark) {
        stats.queue1_high_watermark = queue1_depth;
    }

    k_mutex_unlock(&stats_lock);

    k_fifo_put(&capture_to_process_fifo, block);
}

struct audio_block *pipeline_receive_process(k_timeout_t timeout)
{
    struct audio_block *block;

    block = k_fifo_get(&capture_to_process_fifo, timeout);
    if (block == NULL) {
        return NULL;
    }

    k_mutex_lock(&stats_lock, K_FOREVER);

    if (queue1_depth > 0) {
        queue1_depth--;
    }

    stats.queue1_depth = queue1_depth;

    k_mutex_unlock(&stats_lock);

    return block;
}

void pipeline_submit_process(struct audio_block *block)
{
    k_mutex_lock(&stats_lock, K_FOREVER);

    queue2_depth++;
    stats.queue2_depth = queue2_depth;

    if (queue2_depth > stats.queue2_high_watermark) {
        stats.queue2_high_watermark = queue2_depth;
    }

    k_mutex_unlock(&stats_lock);

    k_fifo_put(&process_to_sink_fifo, block);
}

struct audio_block *pipeline_receive_sink(k_timeout_t timeout)
{
    struct audio_block *block;

    block = k_fifo_get(&process_to_sink_fifo, timeout);
    if (block == NULL) {
        return NULL;
    }

    k_mutex_lock(&stats_lock, K_FOREVER);

    if (queue2_depth > 0) {
        queue2_depth--;
    }

    stats.consumed++;
    stats.queue2_depth = queue2_depth;

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
    // happens when sink waits but no data is available
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
