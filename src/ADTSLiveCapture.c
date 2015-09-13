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

#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif

#include <stdio.h>
#include "ADTSLiveCapture.h"

#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio

int ADTSLiveCaptureInit(ADTSLiveCaptureContext* ctx,
    const char* device, int sampleRate, int channels)
{
    int i, ret;
    AVInputFormat* ifmt;
    AVDictionary *opts = NULL;

    memset(ctx, 0, sizeof(ADTSLiveCaptureContext));

    av_register_all();
    avdevice_register_all();
    avcodec_register_all();
    avfilter_register_all();

    ctx->formatCtx = avformat_alloc_context();
    if (!ctx->formatCtx)
    {
        ret = ADTS_LIVE_CAPTURE_ERROR_ALLOCCONTEXT;
        goto cleanup;
    }

    ifmt = av_find_input_format("dshow");
    if (avformat_open_input(&ctx->formatCtx, device, ifmt, &opts) != 0)
    {  
        ret = ADTS_LIVE_CAPTURE_ERROR_OPENINPUT;
        goto cleanup;
    }
    if (avformat_find_stream_info(ctx->formatCtx, &opts) < 0)  
    {  
        ret = ADTS_LIVE_CAPTURE_ERROR_FINDSTREAMINFO;
        goto cleanup;
    }

    ctx->audioIndex = -1;  
    for (i = 0; i < (int)ctx->formatCtx->nb_streams; i++)
    {
        if (ctx->formatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)  
        {  
            ctx->audioIndex = i;  
            break;  
        }  
    }
    if (ctx->audioIndex == -1)
    {  
        ret = ADTS_LIVE_CAPTURE_ERROR_MEDIANOAUDIO;
        goto cleanup;
    } 

    ctx->rawDecCodecCtx = ctx->formatCtx->streams[ctx->audioIndex]->codec;  
    ctx->rawDecCodec = avcodec_find_decoder(ctx->rawDecCodecCtx->codec_id);
    if (!ctx->rawDecCodec)
    {  
        ret = ADTS_LIVE_CAPTURE_ERROR_FINDDECODER;
        goto cleanup;
    }

    ctx->rawDecCodecCtx->channel_layout = av_get_default_channel_layout(ctx->rawDecCodecCtx->channels);
    if (avcodec_open2(ctx->rawDecCodecCtx, ctx->rawDecCodec, NULL) < 0)  
    {  
        ret = ADTS_LIVE_CAPTURE_ERROR_OPENCODEC;
        goto cleanup;
    }

    ctx->aacEncCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (!ctx->aacEncCodec)
    {  
        ret = ADTS_LIVE_CAPTURE_ERROR_FINDENCODER;
        goto cleanup;
    }
    ctx->aacEncCodecCtx = avcodec_alloc_context3(ctx->aacEncCodec);
    if (!ctx->aacEncCodecCtx) 
    {
        ret = ADTS_LIVE_CAPTURE_ERROR_ALLOCCONTEXT;
        goto cleanup;
    }
    /* put sample parameters */
    //ctx->aacEncCodecCtx->bit_rate = 64000;
    ctx->aacEncCodecCtx->codec_type = AVMEDIA_TYPE_AUDIO; 
    ctx->aacEncCodecCtx->sample_fmt = ctx->aacEncCodecCtx->codec->sample_fmts[0]/*AV_SAMPLE_FMT_S16*/;
    ctx->aacEncCodecCtx->sample_rate = sampleRate;
    ctx->aacEncCodecCtx->channels = channels;
    ctx->aacEncCodecCtx->channel_layout = av_get_default_channel_layout(channels);
    ctx->aacEncCodecCtx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

    if (avcodec_open2(ctx->aacEncCodecCtx, ctx->aacEncCodec, NULL) < 0)  
    {  
        ret = ADTS_LIVE_CAPTURE_ERROR_OPENCODEC;
        goto cleanup;
    }

    ctx->pcmS16Frame = av_frame_alloc();
    ctx->pcmFLTPFrame = av_frame_alloc();
    if (!ctx->pcmS16Frame || !ctx->pcmFLTPFrame)
    {
        ret = ADTS_LIVE_CAPTURE_ERROR_ALLOCFRAME;
        goto cleanup;
    }

    //ctx->frameBufferLen = av_samples_get_buffer_size(NULL, ctx->aacEncCodecCtx->channels, 
    //    ctx->aacEncCodecCtx->frame_size, ctx->aacEncCodecCtx->sample_fmt, 0);
    //ctx->frameBuffer = (char*)av_malloc(ctx->frameBufferLen);
    av_samples_alloc((uint8_t**)&ctx->frameBuffer, &ctx->frameBufferLen, ctx->aacEncCodecCtx->channels,
        ctx->aacEncCodecCtx->frame_size, ctx->aacEncCodecCtx->sample_fmt, 0);
    ctx->pcmFLTPFrame->nb_samples = ctx->aacEncCodecCtx->frame_size;
    if (!ctx->frameBuffer || (avcodec_fill_audio_frame(ctx->pcmFLTPFrame, ctx->aacEncCodecCtx->channels, 
        ctx->aacEncCodecCtx->sample_fmt, (const uint8_t*)ctx->frameBuffer, ctx->frameBufferLen, 0) < 0))
    {
        ret = ADTS_LIVE_CAPTURE_ERROR_ALLOCAUDIO;
        goto cleanup;
    }

    //bufSize = av_samples_get_buffer_size(NULL, ctx->aacEncCodecCtx->channels, 
    //    ctx->aacEncCodecCtx->frame_size, ctx->aacEncCodecCtx->sample_fmt, 0);
    //ctx->pcmFLTPBuffer = (char*)av_malloc(bufSize);
    //if (!ctx->pcmFLTPBuffer || (avcodec_fill_audio_frame(ctx->pcmFLTPFrame, ctx->aacEncCodecCtx->channels, 
    //    ctx->aacEncCodecCtx->sample_fmt, (const uint8_t*)ctx->pcmFLTPBuffer, bufSize, 0) < 0))
    //{
    //    ret = ADTS_LIVE_CAPTURE_ERROR_ALLOCAUDIO;
    //    goto cleanup;
    //}

    ctx->rawPacket = (AVPacket*)av_malloc(sizeof(AVPacket));
    ctx->aacPacket = (AVPacket*)av_malloc(sizeof(AVPacket));
    if (!ctx->rawPacket || !ctx->aacPacket)
    {
        ret = ADTS_LIVE_CAPTURE_ERROR_ALLOCPACKET;
        goto cleanup;
    }

    //ctx->inLen = MAX_AUDIO_FRAME_SIZE * 2;/*av_samples_get_buffer_size(NULL, ctx->rawDecCodecCtx->channels, 
    //    ctx->rawDecCodecCtx->frame_size, ctx->rawDecCodecCtx->sample_fmt, 0);*/
    //ctx->inBuf = (char*)av_malloc(ctx->inLen);
    //ctx->outLen = av_samples_get_buffer_size(NULL, ctx->aacEncCodecCtx->channels, 
    //    ctx->aacEncCodecCtx->frame_size, ctx->aacEncCodecCtx->sample_fmt, 0);
    //ctx->outBuf = (char*)av_malloc(ctx->outLen);
    //if (!ctx->inBuf || !ctx->outBuf)
    //{
    //	ret = ADTS_LIVE_CAPTURE_ERROR_ALLOCBUF;
    //    goto cleanup;
    //}

    ctx->audioFifo = av_audio_fifo_alloc(*ctx->aacEncCodec->sample_fmts, ctx->aacEncCodecCtx->channels, 
        ctx->aacEncCodecCtx->frame_size);

    ctx->swrCtx = swr_alloc_set_opts(NULL,              // we're allocating a new context
        ctx->aacEncCodecCtx->channel_layout,            // out_ch_layout
        ctx->aacEncCodecCtx->sample_fmt,                // out_sample_fmt
        ctx->aacEncCodecCtx->sample_rate,               // out_sample_rate
        ctx->rawDecCodecCtx->channel_layout,            // in_ch_layout
        ctx->rawDecCodecCtx->sample_fmt,                // in_sample_fmt
        ctx->rawDecCodecCtx->sample_rate,               // in_sample_rate
        0,                                              // log_offset
        NULL);                                          // log_ctx
    if (!ctx->swrCtx || swr_init(ctx->swrCtx) < 0) 
    {  
        ret = ADTS_LIVE_CAPTURE_ERROR_ALLOCSWR;
        goto cleanup;
    }

    ctx->ready = 1;
    return ADTS_LIVE_CAPTURE_SUCCESS;

cleanup:
    if (ctx->swrCtx)
    {
        swr_free(&(ctx->swrCtx)); 
        ctx->swrCtx = NULL;
    }

    //if (ctx->outBuf)
    //{
    //    av_free(ctx->outBuf);
    //    ctx->outBuf = NULL;
    //}

    //if (ctx->inBuf)
    //{
    //    av_free(ctx->inBuf);
    //    ctx->inBuf = NULL;
    //}

    if (ctx->rawPacket)
    {
        av_free_packet(ctx->rawPacket);
        av_free(ctx->rawPacket); 
        ctx->rawPacket = NULL;
    }

    if (ctx->aacPacket)
    {
        av_free_packet(ctx->aacPacket);
        av_free(ctx->aacPacket);
        ctx->aacPacket = NULL;
    }

    //if (ctx->pcmFLTPBuffer)
    //{
    //    av_free(ctx->pcmFLTPBuffer);
    //    ctx->pcmFLTPBuffer = NULL;
    //}

    //if (ctx->pcmS16Buffer)
    //{
    //    av_free(ctx->pcmS16Buffer);
    //    ctx->pcmS16Buffer = NULL;
    //}

    if (ctx->pcmFLTPFrame)
    {
        av_frame_free(&ctx->pcmFLTPFrame);
        ctx->pcmFLTPFrame = NULL;
    }

    if (ctx->pcmS16Frame)
    {
        av_frame_free(&ctx->pcmS16Frame);
        ctx->pcmS16Frame = NULL;
    }

    if (ctx->aacEncCodecCtx)
    {
        avcodec_close(ctx->aacEncCodecCtx);
        ctx->aacEncCodecCtx = NULL;
    }

    if (ctx->rawDecCodecCtx)
    {
        avcodec_close(ctx->rawDecCodecCtx);
        ctx->rawDecCodecCtx = NULL;
    }

    if (ctx->formatCtx)
    {
        avformat_close_input(&ctx->formatCtx);
        avformat_free_context(ctx->formatCtx);
        ctx->formatCtx = NULL;
    }

    return ret;
}

int ADTSLiveCapture(ADTSLiveCaptureContext* ctx, void** output, int* len)
{
    int ret = 0;
    int got_frame = 0, got_packet = 0;
    int wantedFrameSize = 0, dstSampleSize = 0;

    if (!ctx->ready)
    {
        return ADTS_LIVE_CAPTURE_ERROR_NOTREADY;
    }

    if (av_audio_fifo_size(ctx->audioFifo) < ctx->aacEncCodecCtx->frame_size)
    {
        av_init_packet(ctx->rawPacket);
        ctx->rawPacket->data = NULL;
        ctx->rawPacket->size = 0;

        if (av_read_frame(ctx->formatCtx, ctx->rawPacket) < 0)
        {
            ret = ADTS_LIVE_CAPTURE_ERROR_READFRAME;
            goto cleanup;
        }

        if (ctx->rawPacket->stream_index != ctx->audioIndex)
        {
            ret = ADTS_LIVE_CAPTURE_ERROR_INVALIDAUDIOINDEX;
            goto cleanup;
        }

        if (avcodec_decode_audio4(ctx->rawDecCodecCtx, ctx->pcmS16Frame, &got_frame, ctx->rawPacket) < 0)
        {
            ret = ADTS_LIVE_CAPTURE_ERROR_DECODE;
            goto cleanup;
        }
        if (!got_frame)
        {
            ret = ADTS_LIVE_CAPTURE_ERROR_GOTFRAME;
            goto cleanup;
        }

        dstSampleSize = (int)av_rescale_rnd(swr_get_delay(ctx->swrCtx, ctx->aacEncCodecCtx->sample_rate) + ctx->pcmS16Frame->nb_samples,
            ctx->aacEncCodecCtx->sample_rate, ctx->aacEncCodecCtx->sample_rate, AV_ROUND_UP);

        wantedFrameSize = av_samples_get_buffer_size(NULL, ctx->aacEncCodecCtx->channels, 
                dstSampleSize, ctx->aacEncCodecCtx->sample_fmt, 0);

        if (wantedFrameSize > ctx->frameBufferLen)
        {
            av_free(ctx->frameBuffer);
            //ctx->frameBufferLen = wantedFrameSize;
            //ctx->frameBuffer = (char*)av_malloc(ctx->frameBufferLen);
            av_samples_alloc((uint8_t**)&ctx->frameBuffer, &ctx->frameBufferLen, ctx->aacEncCodecCtx->channels,
                dstSampleSize, ctx->aacEncCodecCtx->sample_fmt, 0);
            if (!ctx->frameBuffer)
            {
                ret = ADTS_LIVE_CAPTURE_ERROR_ALLOCAUDIO;
                goto cleanup;
            }
        }

        if (swr_convert(ctx->swrCtx, (uint8_t**)&ctx->frameBuffer, dstSampleSize,
            (const uint8_t**)ctx->pcmS16Frame->extended_data, ctx->pcmS16Frame->nb_samples) < 0)
        {
            ret = ADTS_LIVE_CAPTURE_ERROR_RESAMPLE;
            goto cleanup;
        }

        if (av_audio_fifo_write(ctx->audioFifo, (void**)&ctx->frameBuffer, dstSampleSize) < 0)
        {
            ret = ADTS_LIVE_CAPTURE_ERROR_RESAMPLE;
            goto cleanup;
        }
    }

    wantedFrameSize = av_samples_get_buffer_size(NULL, ctx->aacEncCodecCtx->channels, 
        ctx->aacEncCodecCtx->frame_size, ctx->aacEncCodecCtx->sample_fmt, 0);

    if (wantedFrameSize > ctx->frameBufferLen)
    {
        av_free(ctx->frameBuffer);
        //ctx->frameBufferLen = wantedFrameSize;
        //ctx->frameBuffer = (char*)av_malloc(ctx->frameBufferLen);
        av_samples_alloc((uint8_t**)&ctx->frameBuffer, &ctx->frameBufferLen, ctx->aacEncCodecCtx->channels,
            ctx->aacEncCodecCtx->frame_size, ctx->aacEncCodecCtx->sample_fmt, 0);
        if (!ctx->frameBuffer)
        {
            ret = ADTS_LIVE_CAPTURE_ERROR_ALLOCAUDIO;
            goto cleanup;
        }
    }

    if (av_audio_fifo_read(ctx->audioFifo, (void**)&ctx->frameBuffer, ctx->aacEncCodecCtx->frame_size) < 0)
    {
        ret = ADTS_LIVE_CAPTURE_ERROR_RESAMPLE;
        goto cleanup;
    }

    if (avcodec_fill_audio_frame(ctx->pcmFLTPFrame, ctx->aacEncCodecCtx->channels,
        ctx->aacEncCodecCtx->sample_fmt, (const uint8_t*)ctx->frameBuffer, ctx->frameBufferLen, 0) < 0)
    {
        ret = ADTS_LIVE_CAPTURE_ERROR_ALLOCAUDIO;
        goto cleanup;
    }

    av_init_packet(ctx->aacPacket);
    ctx->aacPacket->data = NULL;
    ctx->aacPacket->size = 0;

    if (avcodec_encode_audio2(ctx->aacEncCodecCtx, ctx->aacPacket, ctx->pcmFLTPFrame, &got_packet) < 0)
    {
        ret = ADTS_LIVE_CAPTURE_ERROR_ENCODE;
        goto cleanup;
    }

    if (!got_packet)
    {
    	if (avcodec_encode_audio2(ctx->aacEncCodecCtx, ctx->aacPacket, NULL, &got_packet) < 0)
	    {
	        ret = ADTS_LIVE_CAPTURE_ERROR_ENCODE;
	        goto cleanup;
	    }
    	if (!got_packet)
        {
	        ret = ADTS_LIVE_CAPTURE_ERROR_GOTPACKET;
	        goto cleanup;
	    }
    }

    *output = ctx->aacPacket->data;
    *len = ctx->aacPacket->size;

    ret = ADTS_LIVE_CAPTURE_SUCCESS;

cleanup:
    av_free_packet(ctx->rawPacket);
    av_frame_unref(ctx->pcmS16Frame);
    //av_frame_unref(ctx->pcmFLTPFrame);
    av_free_packet(ctx->aacPacket);
    
    return ret;
}

void ADTSLiveCaptureClose(ADTSLiveCaptureContext* ctx)
{
    if (!ctx->ready)
    {
        return;
    }

    if (ctx->swrCtx)
    {
        swr_free(&(ctx->swrCtx)); 
        ctx->swrCtx = NULL;
    }

    if (ctx->audioFifo)
    {
        av_audio_fifo_free(ctx->audioFifo);
        ctx->audioFifo = NULL;
    }

    //if (ctx->outBuf)
    //{
    //    av_free(ctx->outBuf);
    //    ctx->outBuf = NULL;
    //}

    //if (ctx->inBuf)
    //{
    //    av_free(ctx->inBuf);
    //    ctx->inBuf = NULL;
    //}

    if (ctx->rawPacket)
    {
        av_free_packet(ctx->rawPacket);
        av_free(ctx->rawPacket);
        ctx->rawPacket = NULL;
    }

    if (ctx->aacPacket)
    {
        av_free_packet(ctx->aacPacket);
        av_free(ctx->aacPacket);
        ctx->aacPacket = NULL;
    }

    //if (ctx->pcmFLTPBuffer)
    //{
    //    av_free(ctx->pcmFLTPBuffer);
    //    ctx->pcmFLTPBuffer = NULL;
    //}

    //if (ctx->pcmS16Buffer)
    //{
    //    av_free(ctx->pcmS16Buffer);
    //    ctx->pcmS16Buffer = NULL;
    //}

    if (ctx->pcmFLTPFrame)
    {
        av_frame_free(&ctx->pcmFLTPFrame);
        ctx->pcmFLTPFrame = NULL;
    }

    if (ctx->pcmS16Frame)
    {
        av_frame_free(&ctx->pcmS16Frame);
        ctx->pcmS16Frame = NULL;
    }

    if (ctx->aacEncCodecCtx)
    {
        avcodec_close(ctx->aacEncCodecCtx);
        ctx->aacEncCodecCtx = NULL;
    }

    if (ctx->rawDecCodecCtx)
    {
        avcodec_close(ctx->rawDecCodecCtx);
        ctx->rawDecCodecCtx = NULL;
    }

    if (ctx->formatCtx)
    {
        avformat_close_input(&ctx->formatCtx);
        avformat_free_context(ctx->formatCtx);
        ctx->formatCtx = NULL;
    }

    ctx->ready = 0;
}
