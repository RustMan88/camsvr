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

#include "ADTSLiveServerMediaSubsession.hpp"
#include "ADTSLiveFramedSource.hpp"
#include "MPEG4GenericRTPSink.hh"

ADTSLiveServerMediaSubsession* ADTSLiveServerMediaSubsession::createNew(UsageEnvironment& env, 
    const char* device, int sampleRate, int channels) 
{printf("%s: %d.\n", __FILE__, __LINE__);
  return new ADTSLiveServerMediaSubsession(env, device, sampleRate, channels);
}

ADTSLiveServerMediaSubsession::ADTSLiveServerMediaSubsession(UsageEnvironment& env, 
    const char* device, int sampleRate, int channels)
    : OnDemandServerMediaSubsession(env, True), mSampleRate(sampleRate), mChannels(channels)
{printf("%s: %d.\n", __FILE__, __LINE__);
    mDevice = strDup(device);
}

ADTSLiveServerMediaSubsession
::~ADTSLiveServerMediaSubsession() 
{printf("%s: %d.\n", __FILE__, __LINE__);
    if (mDevice)
    {
        delete[] mDevice;
    }
}

FramedSource* ADTSLiveServerMediaSubsession::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) 
{printf("%s: %d.\n", __FILE__, __LINE__);
    estBitrate = 96; // kbps, estimate

    return ADTSLiveFramedSource::createNew(envir(), mDevice, mSampleRate, mChannels);
}

RTPSink* ADTSLiveServerMediaSubsession::createNewRTPSink(Groupsock* rtpGroupsock, 
    unsigned char rtpPayloadTypeIfDynamic, FramedSource* inputSource) 
{printf("%s: %d.\n", __FILE__, __LINE__);
    ADTSLiveFramedSource* adtsSource = (ADTSLiveFramedSource*)inputSource;
    return MPEG4GenericRTPSink::createNew(envir(), rtpGroupsock,
					rtpPayloadTypeIfDynamic,
					adtsSource->samplingFrequency(),
					"audio", "AAC-hbr", adtsSource->configStr(),
					adtsSource->numChannels());
}
