#ifndef AUDIO_PIPELINE_H
#define AUDIO_PIPELINE_H

#include <zephyr/kernel.h>
#include <stdint.h>

#define AUDIO_BLOCK_SAMPLES 160 // number of samples per block
#define AUDIO_BLOCK_COUNT 8 // total number of blocks in the system

// each block is what flows through the pipeline
// fifo_reserved is required by Zephyr FIFO internally (must be first)
struct audio_block {
    void *fifo_reserved;
    uint32_t seq; // sequence number so we can track ordering/debug behavior
    int16_t samples[AUDIO_BLOCK_SAMPLES];
};

// simple stats so we can observe pipeline behavior in real time
// this is useful for debugging things like overflow
struct pipeline_stats {
    uint32_t produced;
    uint32_t consumed;
    uint32_t queue1_depth;
    uint32_t queue2_depth;
    uint32_t queue1_high_watermark;
    uint32_t queue2_high_watermark;
    uint32_t capture_misses; // failed allocations (no free blocks available)
    uint32_t sink_timeouts;
};

int pipeline_block_alloc(struct audio_block **block, k_timeout_t timeout);
void pipeline_block_free(struct audio_block *block);

void pipeline_submit_capture(struct audio_block *block);
struct audio_block *pipeline_receive_process(k_timeout_t timeout);

void pipeline_submit_process(struct audio_block *block);
struct audio_block *pipeline_receive_sink(k_timeout_t timeout);

// these are just helpers to track error conditions in stats
void pipeline_note_capture_miss(void);
void pipeline_note_sink_timeout(void);
void pipeline_snapshot(struct pipeline_stats *stats);

#endif
