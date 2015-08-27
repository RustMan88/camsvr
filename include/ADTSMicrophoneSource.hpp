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
#ifndef _ADTS_MICROPHONE_SOURCE_HPP
#define _ADTS_MICROPHONE_SOURCE_HPP

class ADTSMicrophoneSource: public FramedSource
{
public:
    static ADTSMicrophoneSource* createNew(UsageEnvironment& env,
				       const char* device, int sampleRate, int channels);

    unsigned samplingFrequency() const { return fSamplingFrequency; }
    unsigned numChannels() const { return fNumChannels; }
    char const* configStr() const { return fConfigStr; }
    // returns the 'AudioSpecificConfig' for this stream (in ASCII form)

protected:
    ADTSMicrophoneSource(UsageEnvironment& env, ADTSMicrophoneCaptureThread* thread);
	// called only by createNew()
    virtual ~ADTSMicrophoneSource();

    // redefined virtual functions:
    virtual void doGetNextFrame();
    virtual unsigned int maxFrameSize() const; 

private:
    ADTSMicrophoneCaptureThread* mThread;
    unsigned fSamplingFrequency;
    unsigned fNumChannels;
    unsigned fuSecsPerFrame;
    char fConfigStr[5];
};

#endif
