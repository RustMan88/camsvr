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
#include "H264CameraCapture.h"

#include <libavfilter/buffersrc.h>

static char s_filterDesc[] 
    = "drawtext=fontfile=/usr/share/fonts/default/Type1/n019003l.pfb:x=w-tw-5:y=5:fontcolor=white:fontsize=30:text=\\'%{localtime\\:%Y/%m/%d %H\\\\\\:%M\\\\\\:%S}\\'";

int H264CameraCaptureInit(H264CameraCaptureContext* ctx, 
    const char* device, int width, int height, int fps)
{
    int i, ret;
    char args[512];
    enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUYV422, AV_PIX_FMT_NONE }; 

    memset(ctx, 0, sizeof(H264CameraCaptureContext));

    av_register_all();
    avdevice_register_all(); 
    avcodec_register_all();
    avfilter_register_all();

    ctx->formatCtx = avformat_alloc_context();
    if (!ctx->formatCtx)
    {
        ret = H264_CAMERA_CAPTURE_ERROR_ALLOCCONTEXT;
        goto cleanup;
    }

    AVInputFormat* ifmt = av_find_input_format("video4linux2");
    if (avformat_open_input(&ctx->formatCtx, device, ifmt, NULL) != 0)
    {  
        ret = H264_CAMERA_CAPTURE_ERROR_OPENINPUT;
        goto cleanup;
    }

    if (avformat_find_stream_info(ctx->formatCtx, NULL) < 0)  
    {  
        ret = H264_CAMERA_CAPTURE_ERROR_FINDSTREAMINFO;
        goto cleanup;
    }

    ctx->videoIndex = -1;  
    for (i = 0; i < ctx->formatCtx->nb_streams; i++)
    {
        if (ctx->formatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)  
        {  
            ctx->videoIndex = i;  
            break;  
        }  
    }
    if (ctx->videoIndex == -1)  
    {  
        ret = H264_CAMERA_CAPTURE_ERROR_MEDIANOVIDEO;
        goto cleanup;
    } 

    ctx->rawDecCodecCtx = ctx->formatCtx->streams[ctx->videoIndex]->codec;  
    ctx->rawDecCodec = avcodec_find_decoder(ctx->rawDecCodecCtx->codec_id);
    if (!ctx->rawDecCodec)
    {  
        ret = H264_CAMERA_CAPTURE_ERROR_FINDDECODER;
        goto cleanup;
    }
    if (avcodec_open2(ctx->rawDecCodecCtx, ctx->rawDecCodec, NULL) < 0)  
    {  
        ret = H264_CAMERA_CAPTURE_ERROR_OPENCODEC;
        goto cleanup;
    }

    ctx->H264EncCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!ctx->H264EncCodec)
    {  
        ret = H264_CAMERA_CAPTURE_ERROR_FINDENCODER;
        goto cleanup;
    }
    ctx->H264EncCodecCtx = avcodec_alloc_context3(ctx->H264EncCodec);
    if (!ctx->H264EncCodecCtx) 
    {
        ret = H264_CAMERA_CAPTURE_ERROR_ALLOCCONTEXT;
        goto cleanup;
    }
    /* put sample parameters */
    ctx->H264EncCodecCtx->bit_rate = 0;
    ctx->H264EncCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO; 
    /* resolution must be a multiple of two */
    ctx->H264EncCodecCtx->width = width;
    ctx->H264EncCodecCtx->height = height;
    //ctx->H264EncCodecCtx->frame_number = 1;
    /* frames per second */
    ctx->H264EncCodecCtx->time_base = (AVRational){1, fps};
    ctx->H264EncCodecCtx->gop_size = 1; /* emit one intra frame every ten frames */
    ctx->H264EncCodecCtx->max_b_frames = 0;
    ctx->H264EncCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

    av_opt_set(ctx->H264EncCodecCtx->priv_data, "preset", "ultrafast", 0);
    av_opt_set(ctx->H264EncCodecCtx->priv_data, "tune", "zerolatency", 0);

    if (avcodec_open2(ctx->H264EncCodecCtx, ctx->H264EncCodec, NULL) < 0)  
    {  
        ret = H264_CAMERA_CAPTURE_ERROR_OPENCODEC;
        goto cleanup;
    }

    ctx->yuyv422Frame = av_frame_alloc();
    ctx->yuyv422PlusTimeFrame = av_frame_alloc();
    ctx->yuv420Frame = av_frame_alloc();
    if (!ctx->yuyv422Frame || !ctx->yuyv422PlusTimeFrame || !ctx->yuyv422Frame)
    {
        ret = H264_CAMERA_CAPTURE_ERROR_ALLOCFRAME;
        goto cleanup;
    }

    if (avpicture_alloc((AVPicture*)ctx->yuv420Frame, PIX_FMT_YUV420P, width, height) < 0)
    {   
        ret = H264_CAMERA_CAPTURE_ERROR_ALLOCPICTURE;
        goto cleanup;
    }

    ctx->rawPacket = (AVPacket*)av_malloc(sizeof(AVPacket));
    ctx->H264Packet = (AVPacket*)av_malloc(sizeof(AVPacket));
    if (!ctx->rawPacket || !ctx->H264Packet)
    {
        ret = H264_CAMERA_CAPTURE_ERROR_ALLOCPACKET;
        goto cleanup;
    }

    ctx->filterGraph = avfilter_graph_alloc();
    if (!ctx->filterGraph)
    {
        ret = H264_CAMERA_CAPTURE_ERROR_ALLOCGRAPH;
        goto cleanup;
    }

    ctx->bufSrcFilter = avfilter_get_by_name("buffer");
    ctx->bufSinkFilter = avfilter_get_by_name("ffbuffersink");
    if (!ctx->bufSrcFilter || !ctx->bufSinkFilter)
    {
        ret = H264_CAMERA_CAPTURE_ERROR_GETFILTER;
        goto cleanup;
    }

    ctx->inFilterInOut = avfilter_inout_alloc();
    ctx->outFilterInOut = avfilter_inout_alloc();
    if (!ctx->inFilterInOut || !ctx->outFilterInOut)
    {
        ret = H264_CAMERA_CAPTURE_ERROR_ALLOCINOUT;
        goto cleanup;
    }

    snprintf(args, sizeof(args),  
        "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",  
        ctx->rawDecCodecCtx->width, ctx->rawDecCodecCtx->height, ctx->rawDecCodecCtx->pix_fmt,  
        ctx->rawDecCodecCtx->time_base.num, ctx->rawDecCodecCtx->time_base.den,  
        ctx->rawDecCodecCtx->sample_aspect_ratio.num, ctx->rawDecCodecCtx->sample_aspect_ratio.den);  

    if (avfilter_graph_create_filter(&ctx->bufSrcFilterCtx, ctx->bufSrcFilter, "in", args, NULL, ctx->filterGraph) < 0)
    {
        ret = H264_CAMERA_CAPTURE_ERROR_CREATEFILTER;
        goto cleanup;
    }
    if (avfilter_graph_create_filter(&ctx->bufSinkFilterCtx, ctx->bufSinkFilter, "out", NULL, NULL, ctx->filterGraph) < 0)
    {
        ret = H264_CAMERA_CAPTURE_ERROR_CREATEFILTER;
        goto cleanup;
    }

    av_opt_set_int_list(ctx->bufSinkFilterCtx, "pix_fmts", pix_fmts,  
        AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);

    /* Endpoints for the filter graph. */
    ctx->outFilterInOut->name = av_strdup("in");
    ctx->outFilterInOut->filter_ctx = ctx->bufSrcFilterCtx;
    ctx->outFilterInOut->pad_idx = 0;
    ctx->outFilterInOut->next = NULL;

    ctx->inFilterInOut->name = av_strdup("out");
    ctx->inFilterInOut->filter_ctx = ctx->bufSinkFilterCtx;
    ctx->inFilterInOut->pad_idx = 0;
    ctx->inFilterInOut->next = NULL;

    if (avfilter_graph_parse(ctx->filterGraph, s_filterDesc, 
        ctx->inFilterInOut, ctx->outFilterInOut, NULL) < 0)
    {
        return H264_CAMERA_CAPTURE_ERROR_GRAPHPARSE;
    }

    if (avfilter_graph_config(ctx->filterGraph, NULL) < 0)
    {
        return H264_CAMERA_CAPTURE_ERROR_GRAPHCONFIG;
    }

    ctx->outLen = avpicture_get_size(ctx->H264EncCodecCtx->pix_fmt, ctx->H264EncCodecCtx->width, ctx->H264EncCodecCtx->height);
    ctx->outBuf = (char*)av_malloc(ctx->outLen);

    ctx->swsCtx = sws_getContext(ctx->rawDecCodecCtx->width, ctx->rawDecCodecCtx->height, 
        ctx->rawDecCodecCtx->pix_fmt, ctx->rawDecCodecCtx->width, ctx->rawDecCodecCtx->height, PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);  

    ctx->ready = 1;
    return H264_CAMERA_CAPTURE_SUCCESS;

cleanup:
    if (ctx->swsCtx)
    {
        sws_freeContext(ctx->swsCtx); 
    }

    if (ctx->outBuf)
    {
        av_free(ctx->outBuf);
    }

    //if (ctx->inFilterInOut)
    //{
    //    avfilter_inout_free(&ctx->inFilterInOut);
    //}

    //if (ctx->outFilterInOut)
    //{
    //    avfilter_inout_free(&ctx->outFilterInOut);
    //}

    if (ctx->filterGraph)
    {
        avfilter_graph_free(&ctx->filterGraph);
    }

    if (ctx->rawPacket)
    {
        av_free_packet(ctx->rawPacket); 
    }

    if (ctx->H264Packet)
    {
        av_free_packet(ctx->H264Packet); 
    }

    if (ctx->yuv420Frame)
    {
        av_frame_free(&ctx->yuv420Frame);
    }

    if (ctx->yuyv422PlusTimeFrame)
    {
        av_frame_free(&ctx->yuyv422PlusTimeFrame);
    }

    if (ctx->yuyv422Frame)
    {
        av_frame_free(&ctx->yuyv422Frame);
    }

    if (ctx->H264EncCodecCtx)
    {
        avcodec_close(ctx->H264EncCodecCtx);
    }
    
    if (ctx->rawDecCodecCtx)
    {
        avcodec_close(ctx->rawDecCodecCtx);
    }

    if (ctx->formatCtx)
    {
        avformat_close_input(&ctx->formatCtx);
        avformat_free_context(ctx->formatCtx);
    }

    return ret;
}

int H264CameraCapture(H264CameraCaptureContext* ctx, void** output, int* len)
{
    int got_frame = 0, got_packet = 0;
    double timeDiff;

    if (!ctx->ready)
    {
        return H264_CAMERA_CAPTURE_ERROR_NOTREADY;
    }

    av_init_packet(ctx->rawPacket);
    ctx->rawPacket->data = NULL;
    ctx->rawPacket->size = 0;

    if (av_read_frame(ctx->formatCtx, ctx->rawPacket) < 0)
    {
        return H264_CAMERA_CAPTURE_ERROR_READFRAME;
    }

    if (ctx->rawPacket->stream_index != ctx->videoIndex)  
    {
        return H264_CAMERA_CAPTURE_ERROR_INVALIDVIDEOINDEX;
    }

    if (avcodec_decode_video2(ctx->rawDecCodecCtx, ctx->yuyv422Frame, &got_frame, ctx->rawPacket) < 0)
    {
        av_free_packet(ctx->rawPacket);
        return H264_CAMERA_CAPTURE_ERROR_DECODE;
    }
    av_free_packet(ctx->rawPacket);
    if (!got_frame)
    {
        return H264_CAMERA_CAPTURE_ERROR_GOTFRAME;
    }

    ctx->yuyv422Frame->pts = av_frame_get_best_effort_timestamp(ctx->yuyv422Frame);

    if (av_buffersrc_add_frame_flags(ctx->bufSrcFilterCtx, ctx->yuyv422Frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0)
    {
        return H264_CAMERA_CAPTURE_ERROR_SRCADDFRAME;
    }

    if (av_buffersink_get_frame(ctx->bufSinkFilterCtx, ctx->yuyv422PlusTimeFrame) < 0)
    {
        return H264_CAMERA_CAPTURE_ERROR_SINKGETFRAME;
    }

    if (sws_scale(ctx->swsCtx, (const uint8_t* const*)ctx->yuyv422PlusTimeFrame->data, ctx->yuyv422PlusTimeFrame->linesize, 
        0, ctx->rawDecCodecCtx->height, ctx->yuv420Frame->data, ctx->yuv420Frame->linesize) < 0)
    {
        return H264_CAMERA_CAPTURE_ERROR_SCALE;
    }

    av_init_packet(ctx->H264Packet);
    ctx->H264Packet->data = ctx->outBuf;
    ctx->H264Packet->size = ctx->outLen;

    gettimeofday(&ctx->capNowTime, NULL);
    if (0 == ctx->H264EncCodecCtx->frame_number)
    {
        ctx->capStartTime = ctx->capNowTime;
    }
    timeDiff = ctx->capNowTime.tv_sec - ctx->capStartTime.tv_sec 
        + 0.000001 * (ctx->capNowTime.tv_usec - ctx->capStartTime.tv_usec);
    ctx->yuv420Frame->pts = 90000 * timeDiff;

    if (avcodec_encode_video2(ctx->H264EncCodecCtx, ctx->H264Packet, ctx->yuv420Frame, &got_packet) < 0)
    {
        return H264_CAMERA_CAPTURE_ERROR_ENCODE;
    }
    if (!got_packet)
    {
        return H264_CAMERA_CAPTURE_ERROR_GOTPACKET;
    }

    if (ctx->H264Packet->pts != AV_NOPTS_VALUE)
        ctx->H264Packet->pts = av_rescale_q(ctx->H264Packet->pts, ctx->H264EncCodecCtx->time_base, ctx->H264EncCodecCtx->time_base);
    if (ctx->H264Packet->dts != AV_NOPTS_VALUE)
        ctx->H264Packet->dts = av_rescale_q(ctx->H264Packet->dts, ctx->H264EncCodecCtx->time_base, ctx->H264EncCodecCtx->time_base);

    *output = ctx->H264Packet->data;
    *len = ctx->H264Packet->size;

    return H264_CAMERA_CAPTURE_SUCCESS;
}

void H264CameraCaptureClose(H264CameraCaptureContext* ctx)
{
    if (!ctx->ready)
    {
        return;
    }

    if (ctx->swsCtx)
    {
        sws_freeContext(ctx->swsCtx); 
    }

    if (ctx->outBuf)
    {
        av_free(ctx->outBuf);
    }

    //if (ctx->inFilterInOut)
    //{
    //    avfilter_inout_free(&ctx->inFilterInOut);
    //}

    //if (ctx->outFilterInOut)
    //{
    //    avfilter_inout_free(&ctx->outFilterInOut);
    //}

    if (ctx->filterGraph)
    {
        avfilter_graph_free(&ctx->filterGraph);
    }

    if (ctx->rawPacket)
    {
        av_free_packet(ctx->rawPacket); 
    }

    if (ctx->H264Packet)
    {
        av_free_packet(ctx->H264Packet); 
    }

    if (ctx->yuv420Frame)
    {
        av_frame_free(&ctx->yuv420Frame);
    }

    if (ctx->yuyv422PlusTimeFrame)
    {
        av_frame_free(&ctx->yuyv422PlusTimeFrame);
    }

    if (ctx->yuyv422Frame)
    {
        av_frame_free(&ctx->yuyv422Frame);
    }

    if (ctx->H264EncCodecCtx)
    {
        avcodec_close(ctx->H264EncCodecCtx);
    }

    if (ctx->rawDecCodecCtx)
    {
        avcodec_close(ctx->rawDecCodecCtx);
    }

    if (ctx->formatCtx)
    {
        avformat_close_input(&ctx->formatCtx);
        avformat_free_context(ctx->formatCtx);
    }

    ctx->ready = 0;
}
