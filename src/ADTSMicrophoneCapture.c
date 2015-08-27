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

#include <stdio.h>
#include "ADTSMicrophoneCapture.h"

int ADTSMicrophoneCaptureInit(ADTSMicrophoneCaptureContext* ctx, 
    const char* device, int sampleRate, int channels)
{
    int i, ret;

    memset(ctx, 0, sizeof(ADTSMicrophoneCaptureContext));

    av_register_all();
    avdevice_register_all(); 
    avcodec_register_all();
    avfilter_register_all();

    ctx->formatCtx = avformat_alloc_context();
    if (!ctx->formatCtx)
    {
        ret = ADTS_MICROPHONE_CAPTURE_ERROR_ALLOCCONTEXT;
        goto cleanup;
    }

    AVInputFormat* ifmt = av_find_input_format("alsa");
    if (avformat_open_input(&ctx->formatCtx, device, ifmt, NULL) != 0)
    {  
        ret = ADTS_MICROPHONE_CAPTURE_ERROR_OPENINPUT;
        goto cleanup;
    }

    if (avformat_find_stream_info(ctx->formatCtx, NULL) < 0)  
    {  
        ret = ADTS_MICROPHONE_CAPTURE_ERROR_FINDSTREAMINFO;
        goto cleanup;
    }

    ctx->audioIndex = -1;  
    for (i = 0; i < ctx->formatCtx->nb_streams; i++)
    {
        if (ctx->formatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)  
        {  
            ctx->audioIndex = i;  
            break;  
        }  
    }
    if (ctx->audioIndex == -1)
    {  
        ret = ADTS_MICROPHONE_CAPTURE_ERROR_MEDIANOAUDIO;
        goto cleanup;
    } 

    ctx->rawDecCodecCtx = ctx->formatCtx->streams[ctx->audioIndex]->codec;  
    ctx->rawDecCodec = avcodec_find_decoder(ctx->rawDecCodecCtx->codec_id);
    if (!ctx->rawDecCodec)
    {  
        ret = ADTS_MICROPHONE_CAPTURE_ERROR_FINDDECODER;
        goto cleanup;
    }
    if (avcodec_open2(ctx->rawDecCodecCtx, ctx->rawDecCodec, NULL) < 0)  
    {  
        ret = ADTS_MICROPHONE_CAPTURE_ERROR_OPENCODEC;
        goto cleanup;
    }

    ctx->aacEncCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (!ctx->aacEncCodec)
    {  
        ret = ADTS_MICROPHONE_CAPTURE_ERROR_FINDENCODER;
        goto cleanup;
    }
    ctx->aacEncCodecCtx = avcodec_alloc_context3(ctx->aacEncCodec);
    if (!ctx->aacEncCodecCtx) 
    {
        ret = ADTS_MICROPHONE_CAPTURE_ERROR_ALLOCCONTEXT;
        goto cleanup;
    }
    /* put sample parameters */
    ctx->aacEncCodecCtx->bit_rate = 64000;
    ctx->aacEncCodecCtx->codec_type = AVMEDIA_TYPE_AUDIO; 
    ctx->aacEncCodecCtx->sample_fmt = AV_SAMPLE_FMT_S16;
    ctx->aacEncCodecCtx->sample_rate = sampleRate;
    ctx->aacEncCodecCtx->channels = channels;
    ctx->aacEncCodecCtx->channel_layout = av_get_default_channel_layout(channels);

    if (avcodec_open2(ctx->aacEncCodecCtx, ctx->aacEncCodec, NULL) < 0)  
    {  
        ret = ADTS_MICROPHONE_CAPTURE_ERROR_OPENCODEC;
        goto cleanup;
    }

    ctx->pcmFrame = av_frame_alloc();
    if (!ctx->pcmFrame)
    {
        ret = ADTS_MICROPHONE_CAPTURE_ERROR_ALLOCFRAME;
        goto cleanup;
    }
    ctx->pcmFrame->nb_samples = ctx->aacEncCodecCtx->frame_size;
    ctx->pcmFrame->format = ctx->aacEncCodecCtx->sample_fmt;

    int frameSize = av_samples_get_buffer_size(NULL, ctx->aacEncCodecCtx->channels, 
        ctx->aacEncCodecCtx->frame_size, ctx->aacEncCodecCtx->sample_fmt, 1);  
    ctx->frameBuf = (char*)av_malloc(frameSize);
    if (!ctx->frameBuf || (avcodec_fill_audio_frame(ctx->pcmFrame, ctx->aacEncCodecCtx->channels, 
        ctx->aacEncCodecCtx->sample_fmt, (const uint8_t*)ctx->frameBuf, frameSize, 1) < 0))
    {
        ret = ADTS_MICROPHONE_CAPTURE_ERROR_ALLOCAUDIO;
        goto cleanup;
    }

    ctx->rawPacket = (AVPacket*)av_malloc(sizeof(AVPacket));
    ctx->aacPacket = (AVPacket*)av_malloc(sizeof(AVPacket));
    if (!ctx->rawPacket || !ctx->aacPacket)
    {
        ret = ADTS_MICROPHONE_CAPTURE_ERROR_ALLOCPACKET;
        goto cleanup;
    }

    ctx->inLen = frameSize;
    ctx->inBuf = (char*)av_malloc(ctx->inLen);
    ctx->outLen = frameSize;
    ctx->outBuf = (char*)av_malloc(ctx->outLen);
    if (!ctx->inBuf || !ctx->outBuf)
    {
    	ret = ADTS_MICROPHONE_CAPTURE_ERROR_ALLOCBUF;
        goto cleanup;
    }

    ctx->ready = 1;
    return ADTS_MICROPHONE_CAPTURE_SUCCESS;

cleanup:
    if (ctx->outBuf)
    {
        av_free(ctx->outBuf);
        ctx->outBuf = NULL;
    }

    if (ctx->inBuf)
    {
        av_free(ctx->inBuf);
        ctx->inBuf = NULL;
    }

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

    if (ctx->frameBuf)
    {
        av_free(ctx->frameBuf);
        ctx->frameBuf = NULL;
    }

    if (ctx->pcmFrame)
    {
        av_frame_free(&ctx->pcmFrame);
        ctx->pcmFrame = NULL;
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

int ADTSMicrophoneCapture(ADTSMicrophoneCaptureContext* ctx, void** output, int* len)
{
    int ret = 0;
    int got_frame = 0, got_packet = 0;
    double timeDiff;

    if (!ctx->ready)
    {
        return ADTS_MICROPHONE_CAPTURE_ERROR_NOTREADY;
    }

    av_init_packet(ctx->rawPacket);
    ctx->rawPacket->data = ctx->inBuf;
    ctx->rawPacket->size = ctx->inLen;

    if (av_read_frame(ctx->formatCtx, ctx->rawPacket) < 0)
    {
        ret = ADTS_MICROPHONE_CAPTURE_ERROR_READFRAME;
        goto cleanup;
    }

    if (ctx->rawPacket->stream_index != ctx->audioIndex)  
    {
        ret = ADTS_MICROPHONE_CAPTURE_ERROR_INVALIDAUDIOINDEX;
        goto cleanup;
    }

    if (avcodec_decode_audio4(ctx->rawDecCodecCtx, ctx->pcmFrame, &got_frame, ctx->rawPacket) < 0)
    {
        ret = ADTS_MICROPHONE_CAPTURE_ERROR_DECODE;
        goto cleanup;
    }
    if (!got_frame)
    {
        ret = ADTS_MICROPHONE_CAPTURE_ERROR_GOTFRAME;
        goto cleanup;
    }

    av_init_packet(ctx->aacPacket);
    ctx->aacPacket->data = ctx->outBuf;
    ctx->aacPacket->size = ctx->outLen;

    if (avcodec_encode_audio2(ctx->aacEncCodecCtx, ctx->aacPacket, ctx->pcmFrame, &got_packet) < 0)
    {
        ret = ADTS_MICROPHONE_CAPTURE_ERROR_ENCODE;
        goto cleanup;
    }

    if (!got_packet)
    {
    	if (avcodec_encode_audio2(ctx->aacEncCodecCtx, ctx->aacPacket, NULL, &got_packet) < 0)
	    {
	        ret = ADTS_MICROPHONE_CAPTURE_ERROR_ENCODE;
	        goto cleanup;
	    }
    	if (!got_packet)
        {
	        ret = ADTS_MICROPHONE_CAPTURE_ERROR_GOTPACKET;
	        goto cleanup;
	    }
    }

    *output = ctx->aacPacket->data;
    *len = ctx->aacPacket->size;

    ret = ADTS_MICROPHONE_CAPTURE_SUCCESS;

cleanup:
    av_free_packet(ctx->rawPacket);
    //av_frame_unref(ctx->pcmFrame);
    av_free_packet(ctx->aacPacket);
    
    return ret;
}

void ADTSMicrophoneCaptureClose(ADTSMicrophoneCaptureContext* ctx)
{
    if (!ctx->ready)
    {
        return;
    }

    if (ctx->outBuf)
    {
        av_free(ctx->outBuf);
        ctx->outBuf = NULL;
    }
    
    if (ctx->inBuf)
    {
        av_free(ctx->inBuf);
        ctx->inBuf = NULL;
    }

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

    if (ctx->frameBuf)
    {
        av_free(ctx->frameBuf);
        ctx->frameBuf = NULL;
    }

    if (ctx->pcmFrame)
    {
        av_frame_free(&ctx->pcmFrame);
        ctx->pcmFrame = NULL;
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
