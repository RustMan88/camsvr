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
#ifndef _ADTS_MICROPHONE_CAPTURE_H
#define _ADTS_MICROPHONE_CAPTURE_H


#define ADTS_MICROPHONE_CAPTURE_SUCCESS                 0
#define ADTS_MICROPHONE_CAPTURE_ERROR_ALLOCCONTEXT      -1
#define ADTS_MICROPHONE_CAPTURE_ERROR_OPENINPUT         -2
#define ADTS_MICROPHONE_CAPTURE_ERROR_FINDSTREAMINFO    -3
#define ADTS_MICROPHONE_CAPTURE_ERROR_MEDIANOAUDIO      -4
#define ADTS_MICROPHONE_CAPTURE_ERROR_FINDDECODER       -5
#define ADTS_MICROPHONE_CAPTURE_ERROR_OPENCODEC         -6
#define ADTS_MICROPHONE_CAPTURE_ERROR_FINDENCODER       -7
#define ADTS_MICROPHONE_CAPTURE_ERROR_ALLOCFRAME        -8
#define ADTS_MICROPHONE_CAPTURE_ERROR_NOTREADY          -9
#define ADTS_MICROPHONE_CAPTURE_ERROR_READFRAME         -10
#define ADTS_MICROPHONE_CAPTURE_ERROR_INVALIDAUDIOINDEX -11
#define ADTS_MICROPHONE_CAPTURE_ERROR_DECODE            -12
#define ADTS_MICROPHONE_CAPTURE_ERROR_GOTFRAME          -13
#define ADTS_MICROPHONE_CAPTURE_ERROR_ENCODE            -14
#define ADTS_MICROPHONE_CAPTURE_ERROR_GOTPACKET         -15
#define ADTS_MICROPHONE_CAPTURE_ERROR_ALLOCAUDIO        -16
#define ADTS_MICROPHONE_CAPTURE_ERROR_SCALE             -17
#define ADTS_MICROPHONE_CAPTURE_ERROR_ALLOCPACKET       -18
#define ADTS_MICROPHONE_CAPTURE_ERROR_ALLOCGRAPH        -19
#define ADTS_MICROPHONE_CAPTURE_ERROR_GETFILTER         -20
#define ADTS_MICROPHONE_CAPTURE_ERROR_ALLOCINOUT        -21
#define ADTS_MICROPHONE_CAPTURE_ERROR_CREATEFILTER      -22
#define ADTS_MICROPHONE_CAPTURE_ERROR_GRAPHPARSE        -23
#define ADTS_MICROPHONE_CAPTURE_ERROR_GRAPHCONFIG       -24
#define ADTS_MICROPHONE_CAPTURE_ERROR_SRCADDFRAME       -25
#define ADTS_MICROPHONE_CAPTURE_ERROR_SINKGETFRAME      -26
#define ADTS_MICROPHONE_CAPTURE_ERROR_ALLOCBUF			-27


#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>  
#include <libswscale/swscale.h>  
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>

typedef struct ADTSMicrophoneCaptureContext
{
    int ready;
    int audioIndex;

    AVFormatContext* formatCtx;
    AVCodecContext* rawDecCodecCtx;
    AVCodecContext* aacEncCodecCtx;
    AVCodec* rawDecCodec;
    AVCodec* aacEncCodec;
    AVFrame* pcmFrame;
    AVPacket* rawPacket;
    AVPacket* aacPacket;
    char* frameBuf;
    char* inBuf;
    int inLen;
    char* outBuf;
    int outLen;
} ADTSMicrophoneCaptureContext;

#ifdef __cplusplus
extern "C" {
#endif

int ADTSMicrophoneCaptureInit(ADTSMicrophoneCaptureContext* ctx, 
    const char* device, int sampleRate, int channels);
int ADTSMicrophoneCapture(ADTSMicrophoneCaptureContext* ctx, void** output, int* len);
void ADTSMicrophoneCaptureClose(ADTSMicrophoneCaptureContext* ctx);

#ifdef __cplusplus
}
#endif

#endif
