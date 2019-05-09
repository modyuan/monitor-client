#ifndef FFMPEG_TEST_FFMPEG_H
#define FFMPEG_TEST_FFMPEG_H


extern "C" {
#define __STDC_CONSTANT_MACROS
#include "libavutil/avutil.h"
#include "libavutil/audio_fifo.h"
#include "libavutil/imgutils.h"

#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavfilter/avfilter.h"
#include "libswscale/swscale.h"

#include <libswresample/swresample.h>
}

#endif //FFMPEG_TEST_FFMPEG_H
