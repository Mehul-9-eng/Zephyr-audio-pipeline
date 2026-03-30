#include "sink_backend.h"

#include <zephyr/sys/printk.h>

// simple simulated sink
// lets us keep the pipeline structure separate from real hardware output
int sink_backend_write(struct audio_block *block)
{
    // occasional print just to confirm flow is working
    if ((block->seq % 25) == 0) {
        printk("play %u %d\n", block->seq, block->samples[0]);
    }

    return 0;
}
