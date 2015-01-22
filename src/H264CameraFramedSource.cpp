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

#include <iostream>
#include "H264CameraFramedSource.hpp"

using namespace std;

H264CameraFramedSource* H264CameraFramedSource::createNew(UsageEnvironment& env, 
    const char* device, int width, int height, int fps)
{
    H264CameraCaptureContext ctx;

    memset(&ctx, 0, sizeof(ctx));
    if (H264_CAMERA_CAPTURE_SUCCESS != H264CameraCaptureInit(&ctx, device, width, height, fps))
    {
        return NULL;
    }

    H264CameraFramedSource* newSource = new H264CameraFramedSource(env, ctx);

    return newSource;
}

H264CameraFramedSource::H264CameraFramedSource(UsageEnvironment& env, 
    H264CameraCaptureContext ctx)
    : FramedSource(env), mCtx(ctx)
{
}

H264CameraFramedSource::~H264CameraFramedSource()
{
    envir().taskScheduler().unscheduleDelayedTask(mToken);
    H264CameraCaptureClose(&mCtx);
}

void H264CameraFramedSource::doGetNextFrame()
{
    double delay = 1000.0 / 25;
    int uSecsToDelay = delay * 1000; 
    mToken = envir().taskScheduler().scheduleDelayedTask(uSecsToDelay,
        getNextFrame, this);
}

void H264CameraFramedSource::getNextFrame(void* ptr)  
{  
    ((H264CameraFramedSource*)ptr)->getNextFrame1();  
} 

void H264CameraFramedSource::getNextFrame1()
{
    void* outBuf = NULL;  
    int outLen = 0;
    int ret = 0;

    // capture and compress
    do 
    {
        ret = H264CameraCapture(&mCtx, &outBuf, &outLen);
        if (H264_CAMERA_CAPTURE_SUCCESS == ret)
        {
            break;
        }
    } 
    while (H264_CAMERA_CAPTURE_ERROR_GOTPACKET == ret);

    gettimeofday(&fPresentationTime, 0);

    fFrameSize = outLen;  
    if (fFrameSize > fMaxSize) 
    {  
        fNumTruncatedBytes = fFrameSize - fMaxSize;  
        fFrameSize = fMaxSize;  
    }  
    else 
    {  
        fNumTruncatedBytes = 0;  
    }

    memmove(fTo, outBuf, fFrameSize);

    // notify  
    afterGetting(this); 
}

unsigned int H264CameraFramedSource::maxFrameSize() const
{
    return 100 * 1024;
}



