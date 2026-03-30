#ifndef SINK_BACKEND_H
#define SINK_BACKEND_H

#include "audio_pipeline.h"

// abstraction for audio sink (currently simulated, later could be I2S/codec/etc)
int sink_backend_write(struct audio_block *block);

#endif
