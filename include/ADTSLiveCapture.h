/*
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/
#ifndef _ADTS_LIVE_CAPTURE_H
#define _ADTS_LIVE_CAPTURE_H


#define ADTS_LIVE_CAPTURE_SUCCESS                 0
#define ADTS_LIVE_CAPTURE_ERROR_ALLOCCONTEXT      -1
#define ADTS_LIVE_CAPTURE_ERROR_OPENINPUT         -2
#define ADTS_LIVE_CAPTURE_ERROR_FINDSTREAMINFO    -3
#define ADTS_LIVE_CAPTURE_ERROR_MEDIANOAUDIO      -4
#define ADTS_LIVE_CAPTURE_ERROR_FINDDECODER       -5
#define ADTS_LIVE_CAPTURE_ERROR_OPENCODEC         -6
#define ADTS_LIVE_CAPTURE_ERROR_FINDENCODER       -7
#define ADTS_LIVE_CAPTURE_ERROR_ALLOCFRAME        -8
#define ADTS_LIVE_CAPTURE_ERROR_NOTREADY          -9
#define ADTS_LIVE_CAPTURE_ERROR_READFRAME         -10
#define ADTS_LIVE_CAPTURE_ERROR_INVALIDAUDIOINDEX -11
#define ADTS_LIVE_CAPTURE_ERROR_DECODE            -12
#define ADTS_LIVE_CAPTURE_ERROR_GOTFRAME          -13
#define ADTS_LIVE_CAPTURE_ERROR_ENCODE            -14
#define ADTS_LIVE_CAPTURE_ERROR_GOTPACKET         -15
#define ADTS_LIVE_CAPTURE_ERROR_ALLOCAUDIO        -16
#define ADTS_LIVE_CAPTURE_ERROR_RESAMPLE          -17
#define ADTS_LIVE_CAPTURE_ERROR_ALLOCPACKET       -18
#define ADTS_LIVE_CAPTURE_ERROR_ALLOCSWR          -19
#define ADTS_LIVE_CAPTURE_ERROR_GETFILTER         -20
#define ADTS_LIVE_CAPTURE_ERROR_ALLOCINOUT        -21
#define ADTS_LIVE_CAPTURE_ERROR_CREATEFILTER      -22
#define ADTS_LIVE_CAPTURE_ERROR_GRAPHPARSE        -23
#define ADTS_LIVE_CAPTURE_ERROR_GRAPHCONFIG       -24
#define ADTS_LIVE_CAPTURE_ERROR_SRCADDFRAME       -25
#define ADTS_LIVE_CAPTURE_ERROR_SINKGETFRAME      -26
#define ADTS_LIVE_CAPTURE_ERROR_ALLOCBUF		  -27

#ifdef __cplusplus
#define  __STDC_LIMIT_MACROS
#define  __STDC_CONSTANT_MACROS
#endif


#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>  
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/audio_fifo.h>

typedef struct ADTSLiveCaptureContext
{
    int ready;
    int audioIndex;

    AVFormatContext* formatCtx;
    AVCodecContext* rawDecCodecCtx;
    AVCodecContext* aacEncCodecCtx;
    AVCodec* rawDecCodec;
    AVCodec* aacEncCodec;
    AVFrame* pcmS16Frame;
    AVFrame* pcmFLTPFrame;
    AVPacket* rawPacket;
    AVPacket* aacPacket;
    //char* pcmS16Buffer;
    char* frameBuffer;
    int frameBufferLen;
    //char* inBuf;
    //int inLen;
    char* outBuf;
    int outLen;
    AVAudioFifo* audioFifo;

    struct SwrContext* swrCtx;
} ADTSLiveCaptureContext;

#ifdef __cplusplus
extern "C" {
#endif

int ADTSLiveCaptureInit(ADTSLiveCaptureContext* ctx, 
    const char* device, int sampleRate, int channels);
int ADTSLiveCapture(ADTSLiveCaptureContext* ctx, void** output, int* len);
void ADTSLiveCaptureClose(ADTSLiveCaptureContext* ctx);

#ifdef __cplusplus
}
#endif

#endif
