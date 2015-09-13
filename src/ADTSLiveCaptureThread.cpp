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
#include "ADTSLiveCaptureThread.hpp"

using namespace std;

ADTSLiveCaptureThread::ADTSLiveCaptureThread()
    : mRunning(false), mExitFlag(false), mFrameBufLen(0), mTruncatedLen(0)
{printf("%s: %d.\n", __FILE__, __LINE__);
    memset(&mCtx, 0, sizeof(mCtx));
    memset(mFrameBuf, 0, sizeof(mFrameBuf));
}

ADTSLiveCaptureThread::~ADTSLiveCaptureThread()
{
printf("%s: %d.\n", __FILE__, __LINE__);
}

bool ADTSLiveCaptureThread::Create(const char* device, int sampleRate, int channels)
{printf("%s: %d.\n", __FILE__, __LINE__);
    if (mRunning)
    {
        return false;
    }
printf("%s: %d.\n", __FILE__, __LINE__);
    memset(&mCtx, 0, sizeof(mCtx));
    if (ADTS_LIVE_CAPTURE_SUCCESS != ADTSLiveCaptureInit(&mCtx, device, sampleRate, channels))
    {
        return false;
    }
printf("%s: %d.\n", __FILE__, __LINE__);
    if (0 != pthread_cond_init(&mCondThread, NULL))
    {
        ADTSLiveCaptureClose(&mCtx);
        return false;
    }
printf("%s: %d.\n", __FILE__, __LINE__);
    if (0 != pthread_mutex_init(&mLockThread, NULL))
    {
        ADTSLiveCaptureClose(&mCtx);
        return false;
    }
printf("%s: %d.\n", __FILE__, __LINE__);
    if (0 != pthread_mutex_init(&mLockBuf, NULL))
    {
        ADTSLiveCaptureClose(&mCtx);
        return false;
    }
printf("%s: %d.\n", __FILE__, __LINE__);
    if (0 != pthread_create(&mThread, NULL, ADTSLiveCaptureThread::ADTSLiveCaptureProc, this))
    {
        ADTSLiveCaptureClose(&mCtx);
        return false;
    }
printf("%s: %d.\n", __FILE__, __LINE__);
    mRunning = true;

    Capture();

    return true;
}

void ADTSLiveCaptureThread::Destroy()
{printf("%s: %d.\n", __FILE__, __LINE__);
    if (mRunning)
    {printf("%s: %d.\n", __FILE__, __LINE__);
        mExitFlag = true;
        pthread_cond_signal(&mCondThread);

        pthread_join(mThread, NULL);

        pthread_mutex_destroy(&mLockBuf);
        pthread_mutex_destroy(&mLockThread);
        pthread_cond_destroy(&mCondThread);
printf("%s: %d.\n", __FILE__, __LINE__);
        ADTSLiveCaptureClose(&mCtx);
printf("%s: %d.\n", __FILE__, __LINE__);
        mRunning = false;
    }
}

void ADTSLiveCaptureThread::Capture()
{
    int ret;
    void* outBuf = NULL;  
    int outLen = 0;
printf("%s: %d.\n", __FILE__, __LINE__);
    if (!mRunning)
    {
        return;
    }
printf("%s: %d.\n", __FILE__, __LINE__);
    pthread_cond_signal(&mCondThread);
}

void ADTSLiveCaptureThread::Export(void* buf, int len, int* frameLen, int* truncatedLen)
{printf("%s: %d.\n", __FILE__, __LINE__);
    int exportLen = mFrameBufLen;

    pthread_mutex_lock(&mLockBuf);
    if (mFrameBufLen > len) 
    {   
        *truncatedLen = mTruncatedLen + mFrameBufLen - len;
        exportLen = len;
    }
    else
    {
        *truncatedLen = mTruncatedLen;
    }
    memcpy(buf, mFrameBuf, exportLen);
    *frameLen = exportLen;
printf("%s: %d.\n", __FILE__, __LINE__);
    pthread_mutex_unlock(&mLockBuf);
}

bool ADTSLiveCaptureThread::GetExitFlag()
{
    return mExitFlag;
}

void* ADTSLiveCaptureThread::ADTSLiveCaptureProc(void* ptr)
{
    ADTSLiveCaptureThread* thread = (ADTSLiveCaptureThread*)ptr;
printf("%s: %d.\n", __FILE__, __LINE__);
    while (1)
    {
        if (thread->GetExitFlag())
        {
            break;
        }

        thread->CaptureProc();
    }
printf("%s: %d.\n", __FILE__, __LINE__);
    pthread_exit(NULL);
}

void ADTSLiveCaptureThread::CaptureProc()
{
    int ret;
    void* outBuf = NULL;  
    int outLen = 0;
printf("%s: %d.\n", __FILE__, __LINE__);
    pthread_mutex_lock(&mLockThread);
    pthread_cond_wait(&mCondThread, &mLockThread);
    pthread_mutex_unlock(&mLockThread);
printf("%s: %d.\n", __FILE__, __LINE__);
    ret = ADTSLiveCapture(&mCtx, &outBuf, &outLen);
    if (ADTS_LIVE_CAPTURE_SUCCESS == ret)
    {
        int frameSize = outLen;
        int truncatedSize = 0;
        if (frameSize > sizeof(mFrameBuf))
        {
            truncatedSize = frameSize - sizeof(mFrameBuf);
            frameSize = sizeof(mFrameBuf); 
        }
printf("%s: %d.\n", __FILE__, __LINE__);
        pthread_mutex_lock(&mLockBuf);
        memcpy(mFrameBuf, outBuf, frameSize);
        mFrameBufLen = frameSize;
        mTruncatedLen = truncatedSize;
        pthread_mutex_unlock(&mLockBuf);
    }
    else
    {printf("%s: %d.\n", __FILE__, __LINE__);
        pthread_mutex_lock(&mLockBuf);
        mFrameBufLen = 0;
        mTruncatedLen = 0;
        pthread_mutex_unlock(&mLockBuf);
    }
}
