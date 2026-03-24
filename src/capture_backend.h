#ifndef CAPTURE_BACKEND_H
#define CAPTURE_BACKEND_H

#include "audio_pipeline.h"

int capture_backend_read(struct audio_block *block);

#endif
