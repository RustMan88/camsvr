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
    memset(&mLastCapTime, 0, sizeof(mLastCapTime));
}

H264CameraFramedSource::~H264CameraFramedSource()
{
    envir().taskScheduler().unscheduleDelayedTask(mToken);
    H264CameraCaptureClose(&mCtx);
}

void H264CameraFramedSource::doGetNextFrame()
{
    struct timeval nowTime;
    int usDiff, usToDelay;
    int usInterval = 1000000 / (mCtx.H264EncCodecCtx->time_base.den);

    if ((mLastCapTime.tv_sec == 0) && (mLastCapTime.tv_usec == 0))
    {
        mToken = envir().taskScheduler().scheduleDelayedTask(0,
            getNextFrame, this);
        return;
    }

    gettimeofday(&nowTime, NULL);

    usDiff = (nowTime.tv_sec - mLastCapTime.tv_sec) * 1000000 + (nowTime.tv_usec - mLastCapTime.tv_usec);

    if (usDiff >= usInterval)
    {
#ifdef IS_DEBUG
        cout << "Duration: " << usDiff << "." << endl; 
        cout << "Capture and encode time is too long." << endl;
#endif
        usToDelay = 0;
    }
    else
    {
        usToDelay = usInterval - usDiff;
    }
    
    mToken = envir().taskScheduler().scheduleDelayedTask(usToDelay,
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
    int usInterval = 1000000 / (mCtx.H264EncCodecCtx->time_base.den);
    
    gettimeofday(&fPresentationTime, NULL);
    mLastCapTime = fPresentationTime;
    mLastCapTime.tv_usec = mLastCapTime.tv_usec / usInterval * usInterval;

#ifdef IS_DEBUG
    struct tm* ptm;
    char time_string[40];
    long milliseconds;

    ptm = localtime (&fPresentationTime.tv_sec);
    strftime (time_string, sizeof (time_string), "%Y-%m-%d %H:%M:%S", ptm);
    milliseconds = fPresentationTime.tv_usec / 1000;
    printf ("Presentation time: %s.%03ld\n", time_string, milliseconds);

    int time_use = 0;
    struct timeval start;
    struct timeval end;
    //struct timezone tz;

    gettimeofday(&start, NULL);
#endif

    ret = H264CameraCapture(&mCtx, &outBuf, &outLen);
    if (H264_CAMERA_CAPTURE_SUCCESS != ret)
    {
        if (H264_CAMERA_CAPTURE_ERROR_GOTPACKET == ret)
        {
            doGetNextFrame();
        }

        return;
    }

#ifdef IS_DEBUG
    gettimeofday(&end, NULL);
    time_use = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
    cout << "Use time: " << time_use << "us." << endl;
#endif

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



