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

#include "ADTSLiveFramedSource.hpp"
#include <GroupsockHelper.hh>

static unsigned const samplingFrequencyTable[16] = {
    96000, 88200, 64000, 48000,
    44100, 32000, 24000, 22050,
    16000, 12000, 11025, 8000,
    7350, 0, 0, 0
};

ADTSLiveFramedSource* ADTSLiveFramedSource::createNew(UsageEnvironment& env, 
    const char* device, int sampleRate, int channels) 
{printf("%s: %d.\n", __FILE__, __LINE__);
    ADTSLiveCaptureThread* thread = new ADTSLiveCaptureThread();
    if (NULL == thread)
    {
        return NULL;
    }
printf("%s: %d.\n", __FILE__, __LINE__);
    if (!thread->Create(device, sampleRate, channels))
    {
        delete thread;
        return NULL;
    }
printf("%s: %d.\n", __FILE__, __LINE__);
    ADTSLiveFramedSource* newSource = new ADTSLiveFramedSource(env, thread, sampleRate, channels);

    return newSource;
}

ADTSLiveFramedSource::ADTSLiveFramedSource(UsageEnvironment& env, ADTSLiveCaptureThread* thread,
    int samplingFrequency, int numChannels)
  : FramedSource(env), mThread(thread)
{printf("%s: %d.\n", __FILE__, __LINE__);
    u_int8_t profile = 1;
    fSamplingFrequency = samplingFrequency;
    u_int8_t samplingFrequencyIndex = 0, channelConfiguration;
    for (int i = 0; i < 16; i++)
    {
        if (samplingFrequencyTable[i] == samplingFrequency)
        {
            samplingFrequencyIndex = i;
            break;
        }
    }
printf("%s: %d.\n", __FILE__, __LINE__);
    fNumChannels = numChannels;
    channelConfiguration = numChannels;
    fuSecsPerFrame = (1024/*samples-per-frame*/*1000000) / fSamplingFrequency/*samples-per-second*/;
printf("%s: %d.\n", __FILE__, __LINE__);
    // Construct the 'AudioSpecificConfig', and from it, the corresponding ASCII string:
    unsigned char audioSpecificConfig[2];
    u_int8_t const audioObjectType = profile + 1;
    audioSpecificConfig[0] = (audioObjectType<<3) | (samplingFrequencyIndex>>1);
    audioSpecificConfig[1] = (samplingFrequencyIndex<<7) | (channelConfiguration<<3);
    sprintf(fConfigStr, "%02X%02x", audioSpecificConfig[0], audioSpecificConfig[1]);
}

ADTSLiveFramedSource::~ADTSLiveFramedSource() 
{printf("%s: %d.\n", __FILE__, __LINE__);
    mThread->Destroy();
    envir().taskScheduler().unscheduleDelayedTask(mToken);
}

void ADTSLiveFramedSource::doGetNextFrame()
{printf("%s: %d.\n", __FILE__, __LINE__);
    mToken = envir().taskScheduler().scheduleDelayedTask(0,
        getNextFrame, this);
}

void ADTSLiveFramedSource::getNextFrame(void* ptr)
{printf("%s: %d.\n", __FILE__, __LINE__);
    ((ADTSLiveFramedSource*)ptr)->getNextFrame1();  
}

void ADTSLiveFramedSource::getNextFrame1()
{printf("%s: %d.\n", __FILE__, __LINE__);
    int frameSize, truncatedSize;

    mThread->Export(fTo, maxFrameSize(), &frameSize, &truncatedSize);
    fFrameSize = frameSize;
    fNumTruncatedBytes = truncatedSize;
    mThread->Capture();
printf("%s: %d.\n", __FILE__, __LINE__);
    // notify  
    afterGetting(this); 
}
