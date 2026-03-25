#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "audio_pipeline.h"
#include "capture_backend.h"

// capture runs slightly faster. This creates pressure on pipeline
#define CAPTURE_PERIOD_MS 10
#define PLAYBACK_PERIOD_MS 11 // this is intentional

// simple processing stage (low-pass smoothing)
// shows how processing can be inserted between capture and playback
static void process_block(struct audio_block *block)
{
    static int16_t prev;

    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
        int32_t y = (3 * (int32_t)prev + block->samples[i]) / 4;
        block->samples[i] = (int16_t)y;
        prev = (int16_t)y;
    }
}

// producer thread: capture + process + submit
void capture_thread(void)
{
    uint32_t seq = 0;

    while (1) {
        struct audio_block *block;

        if (pipeline_block_alloc(&block, K_NO_WAIT) != 0) {
            pipeline_note_capture_miss();
            k_msleep(CAPTURE_PERIOD_MS);
            continue;
        }

        block->seq = seq++;

        capture_backend_read(block);
        process_block(block);
        pipeline_submit(block);
        k_msleep(CAPTURE_PERIOD_MS);
    }
}

// consumer thread: receive + release
void playback_thread(void)
{
    while (1) {
        struct audio_block *block;

        block = pipeline_receive(K_MSEC(20));

        if (block == NULL) {
            pipeline_note_sink_timeout(); // no data available
            k_msleep(PLAYBACK_PERIOD_MS);
            continue;
        }
        // occasional print just to confirm flow is working
        if ((block->seq % 25) == 0) {
            printk("play %u %d\n", block->seq, block->samples[0]);
        }

        pipeline_block_free(block);

        k_msleep(PLAYBACK_PERIOD_MS);
    }
}

// define threads (same priority for simplicity)
K_THREAD_DEFINE(cap_id, 2048, capture_thread, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(play_id, 2048, playback_thread, NULL, NULL, NULL, 5, 0, 0);

int main(void)
{
    struct pipeline_stats stats;

    printk("start\n");

    while (1) {
        pipeline_snapshot(&stats);

        printk("p=%u c=%u d=%u h=%u m=%u t=%u\n",
               stats.produced,
               stats.consumed,
               stats.queued,
               stats.high_watermark,
               stats.capture_misses,
               stats.sink_timeouts);

        k_msleep(1000);
    }
}
