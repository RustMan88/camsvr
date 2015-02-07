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
#ifndef _H264_CAMERA_FRAMED_SOURCE_HPP
#define _H264_CAMERA_FRAMED_SOURCE_HPP

#include <liveMedia.hh>
#include "H264CameraCapture.h"

class H264CameraFramedSource : public FramedSource
{
public:
    static H264CameraFramedSource* createNew(UsageEnvironment& env, 
        const char* device, int width, int height, int fps);

    static void getNextFrame(void* ptr);
    void getNextFrame1();

protected:
    H264CameraFramedSource(UsageEnvironment& env, H264CameraCaptureContext ctx);
    ~H264CameraFramedSource();

	virtual void doGetNextFrame();
    virtual unsigned int maxFrameSize() const; 

private:
    void* mToken;
    H264CameraCaptureContext mCtx;
    struct timeval mLastCapTime;
};

#endif
