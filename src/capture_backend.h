#ifndef CAPTURE_BACKEND_H
#define CAPTURE_BACKEND_H

#include "audio_pipeline.h"

// abstraction for audio source (currently synthetic, later could be I2S/DMIC/etc
int capture_backend_read(struct audio_block *block);

#endif
