#ifndef AUDIO_PIPELINE_H
#define AUDIO_PIPELINE_H

#include <zephyr/kernel.h>
#include <stdint.h>

#define AUDIO_BLOCK_SAMPLES 160
#define AUDIO_BLOCK_COUNT 8

struct audio_block {
    void *fifo_reserved;
    uint32_t seq;
    int16_t samples[AUDIO_BLOCK_SAMPLES];
};

struct pipeline_stats {
    uint32_t produced;
    uint32_t consumed;
    uint32_t queued;
    uint32_t high_watermark;
    uint32_t capture_misses;
    uint32_t sink_timeouts;
};

int pipeline_block_alloc(struct audio_block **block, k_timeout_t timeout);
void pipeline_block_free(struct audio_block *block);
void pipeline_submit(struct audio_block *block);
struct audio_block *pipeline_receive(k_timeout_t timeout);

void pipeline_note_capture_miss(void);
void pipeline_note_sink_timeout(void);
void pipeline_snapshot(struct pipeline_stats *stats);

#endif
