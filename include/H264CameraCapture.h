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
#ifndef _H264_CAMERA_CAPTURE_H
#define _H264_CAMERA_CAPTURE_H


#define H264_CAMERA_CAPTURE_SUCCESS                 0
#define H264_CAMERA_CAPTURE_ERROR_ALLOCCONTEXT      -1
#define H264_CAMERA_CAPTURE_ERROR_OPENINPUT         -2
#define H264_CAMERA_CAPTURE_ERROR_FINDSTREAMINFO    -3
#define H264_CAMERA_CAPTURE_ERROR_MEDIANOVIDEO      -4
#define H264_CAMERA_CAPTURE_ERROR_FINDDECODER       -5
#define H264_CAMERA_CAPTURE_ERROR_OPENCODEC         -6
#define H264_CAMERA_CAPTURE_ERROR_FINDENCODER       -7
#define H264_CAMERA_CAPTURE_ERROR_ALLOCFRAME        -8
#define H264_CAMERA_CAPTURE_ERROR_NOTREADY          -9
#define H264_CAMERA_CAPTURE_ERROR_READFRAME         -10
#define H264_CAMERA_CAPTURE_ERROR_INVALIDVIDEOINDEX -11
#define H264_CAMERA_CAPTURE_ERROR_DECODE            -12
#define H264_CAMERA_CAPTURE_ERROR_GOTFRAME          -13
#define H264_CAMERA_CAPTURE_ERROR_ENCODE            -14
#define H264_CAMERA_CAPTURE_ERROR_GOTPACKET         -15
#define H264_CAMERA_CAPTURE_ERROR_ALLOCPICTURE      -16
#define H264_CAMERA_CAPTURE_ERROR_SCALE             -17
#define H264_CAMERA_CAPTURE_ERROR_ALLOCPACKET       -18

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>  
#include <libswscale/swscale.h>  
#include <libavdevice/avdevice.h>

typedef struct H264CameraCaptureContext
{
    int ready;
    int videoIndex;  

    AVFormatContext* formatCtx;  
    AVCodecContext* rawDecCodecCtx;
    AVCodecContext* H264EncCodecCtx;
    AVCodec* rawDecCodec;
    AVCodec* H264EncCodec;
    AVFrame* yuyv422Frame;
    AVFrame* yuv420Frame;
    AVPacket* rawPacket;
    AVPacket* H264Packet;
    char* outBuf;
    int outLen;

    struct SwsContext* swsCtx;

} H264CameraCaptureContext;

#ifdef __cplusplus
extern "C" {
#endif

int H264CameraCaptureInit(H264CameraCaptureContext* ctx, 
    const char* device, int width, int height, int fps);
int H264CameraCapture(H264CameraCaptureContext* ctx, void** output, int* len);
void H264CameraCaptureClose(H264CameraCaptureContext* ctx);

#ifdef __cplusplus
}
#endif

#endif
