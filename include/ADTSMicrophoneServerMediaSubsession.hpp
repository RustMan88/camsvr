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
#ifndef _ADTS_MICROPHONE_SERVER_MEDIA_SUBSESSION_HPP
#define _ADTS_MICROPHONE_SERVER_MEDIA_SUBSESSION_HPP

#include <liveMedia.hh>

class ADTSMicrophoneServerMediaSubsession: public OnDemandServerMediaSubsession
{
public:
    static ADTSMicrophoneServerMediaSubsession* createNew(UsageEnvironment& env, const char* device);

protected:
    ADTSMicrophoneServerMediaSubsession(UsageEnvironment& env, const char* device);
    // called only by createNew();
    virtual ~ADTSMicrophoneServerMediaSubsession();

    // redefined virtual functions
    void setDoneFlag() { mDoneFlag = ~0; }

    virtual char const* getAuxSDPLine(RTPSink* rtpSink,
        FramedSource* inputSource);
    virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
        unsigned& estBitrate);
    // "estBitrate" is the stream's estimated bitrate, in kbps
    virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
        unsigned char rtpPayloadTypeIfDynamic,
        FramedSource* inputSource);

private:
    char* mAuxSDPLine;
    char mDoneFlag; // used when setting up "fAuxSDPLine"
    RTPSink* mDummyRTPSink; // ditto
};

#endif
