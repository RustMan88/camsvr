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
    H264CameraCaptureThread* thread = new H264CameraCaptureThread();
    if (NULL == thread)
    {
        return NULL;
    }

    if (!thread->Create(device, width, height, fps))
    {
        delete thread;
        return NULL;
    }

    H264CameraFramedSource* newSource = new H264CameraFramedSource(env, thread);

    return newSource;
}

H264CameraFramedSource::H264CameraFramedSource(UsageEnvironment& env, 
    H264CameraCaptureThread* thread)
    : FramedSource(env), mThread(thread)
{
}

H264CameraFramedSource::~H264CameraFramedSource()
{
    mThread->Destroy();
    envir().taskScheduler().unscheduleDelayedTask(mToken);
}

void H264CameraFramedSource::doGetNextFrame()
{ 
    mToken = envir().taskScheduler().scheduleDelayedTask(0,
        getNextFrame, this);
}

void H264CameraFramedSource::getNextFrame(void* ptr)
{  
    ((H264CameraFramedSource*)ptr)->getNextFrame1();  
} 

void H264CameraFramedSource::getNextFrame1()
{
    int frameSize, truncatedSize;

    mThread->Export(fTo, maxFrameSize(), &frameSize, &truncatedSize);
    fFrameSize = frameSize;
    fNumTruncatedBytes = truncatedSize;
    mThread->Capture();

    // notify  
    afterGetting(this); 
}

unsigned int H264CameraFramedSource::maxFrameSize() const
{
    return H264_MAX_FRAME_SIZE;
}




