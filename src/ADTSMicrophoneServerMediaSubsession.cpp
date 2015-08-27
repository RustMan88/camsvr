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

#include "ADTSMicrophoneServerMediaSubsession.hh"
#include "ADTSMicrophoneSource.hh"
#include "MPEG4GenericRTPSink.hh"

ADTSMicrophoneServerMediaSubsession*
ADTSMicrophoneServerMediaSubsession::createNew(UsageEnvironment& env, const char* device) 
{
  return new ADTSMicrophoneServerMediaSubsession(env, device);
}

ADTSMicrophoneServerMediaSubsession
::ADTSMicrophoneServerMediaSubsession(UsageEnvironment& env, const char* device)
  : OnDemandServerMediaSubsession(env, True)
{
    mDevice = strDup(device);
}

ADTSMicrophoneServerMediaSubsession
::~ADTSMicrophoneServerMediaSubsession() 
{
    if (mAuxSDPLine)
    {
        delete[] mAuxSDPLine;
    }

    if (mDevice)
    {
        delete[] mDevice;
    }
}

FramedSource* ADTSMicrophoneServerMediaSubsession::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) 
{
    estBitrate = 96; // kbps, estimate

    return ADTSMicrophoneSource::createNew(envir(), fFileName);
}

RTPSink* ADTSMicrophoneServerMediaSubsession::createNewRTPSink(Groupsock* rtpGroupsock, 
    unsigned char rtpPayloadTypeIfDynamic, FramedSource* inputSource) 
{
    ADTSMicrophoneSource* adtsSource = (ADTSMicrophoneSource*)inputSource;
    return MPEG4GenericRTPSink::createNew(envir(), rtpGroupsock,
					rtpPayloadTypeIfDynamic,
					adtsSource->samplingFrequency(),
					"audio", "AAC-hbr", adtsSource->configStr(),
					adtsSource->numChannels());
}
