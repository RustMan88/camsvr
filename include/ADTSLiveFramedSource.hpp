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
#ifndef _ADTS_LIVE_SOURCE_HPP
#define _ADTS_LIVE_SOURCE_HPP

#include <liveMedia.hh>
#include "ADTSLiveCaptureThread.hpp"

class ADTSLiveFramedSource: public FramedSource
{
public:
    static ADTSLiveFramedSource* createNew(UsageEnvironment& env,
				       const char* device, int sampleRate, int channels);

    static void getNextFrame(void* ptr);
    void getNextFrame1();

    unsigned samplingFrequency() const { return fSamplingFrequency; }
    unsigned numChannels() const { return fNumChannels; }
    char const* configStr() const { return fConfigStr; }
    // returns the 'AudioSpecificConfig' for this stream (in ASCII form)

protected:
    ADTSLiveFramedSource(UsageEnvironment& env, ADTSLiveCaptureThread* thread, 
        int samplingFrequency, int numChannels);
	// called only by createNew()
    virtual ~ADTSLiveFramedSource();

    // redefined virtual functions:
    virtual void doGetNextFrame();

private:
    void* mToken;
    ADTSLiveCaptureThread* mThread;
    unsigned fSamplingFrequency;
    unsigned fNumChannels;
    unsigned fuSecsPerFrame;
    char fConfigStr[5];
};

#endif
