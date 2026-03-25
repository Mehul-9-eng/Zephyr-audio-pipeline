#include "capture_backend.h"

#include <stdint.h>

// simple synthetic waveform generator
// lets us test pipeline without real hardware input
int capture_backend_read(struct audio_block *block)
{
    static int16_t level = -14000;
    static int16_t step = 350;

    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
        block->samples[i] = level;
        level += step;

        // bounce between bounds to create triangle-like waveform
        if (level >= 14000) {
            level = 14000;
            step = -step;
        } else if (level <= -14000) {
            level = -14000;
            step = -step;
        }
    }

    return 0;
}
