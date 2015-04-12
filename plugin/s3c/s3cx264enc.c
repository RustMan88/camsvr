/*
 * S3C H264 Video Encoder
 * Copyright (C) 2015 Jim Wang
 *
 */


#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include "libavutil/internal.h"
#include "libavutil/intreadwrite.h"
#include "avcodec.h"
#include "internal.h"
#include "MfcDriver.h"
#include "MfcDrvParams.h"

#define MFC_DEV_NAME        "/dev/s3c-mfc"
#define BUF_SIZE            1024000

static const enum AVPixelFormat pixfmt_rgb24[] = {
    AV_PIX_FMT_BGR24, AV_PIX_FMT_RGB32, AV_PIX_FMT_NONE };

typedef struct X264Context 
{
    int dev_fd;
    uint8_t *addr;
    MFC_ENC_INIT_ARG enc_init;
    MFC_ENC_EXE_ARG enc_exe;
    MFC_GET_BUF_ADDR_ARG get_buf_addr;
    uint8_t *in_buf, *out_buf;
    AVFrame out_pic;
} X264Context;

static av_cold int x264_encode_init(AVCodecContext *avctx)
{
    X264Context* const c = avctx->priv_data;
    int ret;
    
    c->dev_fd = open(MFC_DEV_NAME, O_RDWR | O_NDELAY);
    if (c->dev_fd < 0)
    {
        av_log(avctx, AV_LOG_ERROR, "Error: MFC device open failure: %s.\n",
               MFC_DEV_NAME);
        return AVERROR_INVALIDDATA;
    }

    c->addr = (uint8_t *)mmap(0, BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, c->dev_fd, 0);
    if (c->addr == MAP_FAILED)
    {
        av_log(avctx, AV_LOG_ERROR, "Error: MFC mmap failure.\n");
        return AVERROR_INVALIDDATA;
    }
    
    memset(&c->enc_init, 0, sizeof(c->enc_init));
    c->enc_init.in_width = avctx->width;
    c->enc_init.in_height = avctx->height;
    c->enc_init.in_frameRateRes = avctx->time_base.den;
    c->enc_init.in_frameRateDiv = 0;
    c->enc_init.in_bitrate = avctx->bit_rate;
    c->enc_init.in_gopNum = avctx->gop_size;

    printf("Width: %d, Height: %d, FrameRateRes: %d, FrameRateDiv: %d, Bitrate: %d, GopNum: %d\n",
        c->enc_init.in_width, c->enc_init.in_height, c->enc_init.in_frameRateRes, c->enc_init.in_frameRateDiv,
        c->enc_init.in_bitrate, c->enc_init.in_gopNum);
    
    ret = ioctl(c->dev_fd, IOCTL_MFC_H264_ENC_INIT, &c->enc_init);
    if ((ret < 0) || (c->enc_init.ret_code < 0))
    {
        return AVERROR_INVALIDDATA;
    }
    
    memset(&c->get_buf_addr, 0, sizeof(c->get_buf_addr));
    c->get_buf_addr.in_usr_data = (int)c->addr;
    ret = ioctl(c->dev_fd, IOCTL_MFC_GET_FRAM_BUF_ADDR, &c->get_buf_addr);
    if ((ret < 0) || (c->get_buf_addr.ret_code < 0))
    {
        return AVERROR_INVALIDDATA;
    }
    c->in_buf = (uint8_t*)c->get_buf_addr.out_buf_addr;

    memset(&c->get_buf_addr, 0, sizeof(c->get_buf_addr));
    c->get_buf_addr.in_usr_data = (int)c->addr;
    ret = ioctl(c->dev_fd, IOCTL_MFC_GET_LINE_BUF_ADDR, &c->get_buf_addr);
    if ((ret < 0) || (c->get_buf_addr.ret_code < 0))
    {
        return AVERROR_INVALIDDATA;
    }
    c->out_buf = (uint8_t*)c->get_buf_addr.out_buf_addr;

    return 0;
}

static int x264_encode_frame(AVCodecContext *avctx, AVPacket *pkt,
                             const AVFrame *frame, int *got_packet)
{
    X264Context* const c = avctx->priv_data;
    int ret;
    int payload = 0;
    
    *got_packet = 0;

    if (frame) 
    {
        memcpy(c->in_buf, frame->data[0], avctx->width * avctx->height);
        memcpy(c->in_buf + avctx->width * avctx->height, frame->data[1], avctx->width * avctx->height / 4);
        memcpy(c->in_buf + avctx->width * avctx->height + avctx->width * avctx->height / 4, frame->data[2],
            avctx->width * avctx->height / 4);
    }
    else
        return 0;

    ret = ioctl(c->dev_fd, IOCTL_MFC_H264_ENC_EXE, &c->enc_exe);
    if ((ret < 0) || (c->enc_exe.ret_code < 0))
    {
        av_log(avctx, AV_LOG_ERROR, "Error: H264 encode video failure.\n");
        return AVERROR_EXTERNAL;
    }

    payload = c->enc_exe.out_encoded_size;
    
    ret = ff_alloc_packet(pkt, payload);
    if (ret < 0) {
        av_log(avctx, AV_LOG_ERROR, "Error: Allocate packet failure.\n");
        return AVERROR(ENOMEM);
    }

    memcpy(pkt->data, c->out_buf, payload);
    
    pkt->pts = frame->pts;

    *got_packet = 1;
    return 0;
}

static av_cold int x264_encode_close(AVCodecContext *avctx)
{
    X264Context* const c = avctx->priv_data;
    
    munmap(c->addr, BUF_SIZE);
    close(c->dev_fd);

    return 0;
}

AVCodec ff_s3cx264_encoder = {
    .name           = "s3cx264",
    .long_name      = NULL_IF_CONFIG_SMALL("S3C H264 video"),
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = AV_CODEC_ID_H264,
    .priv_data_size = sizeof(X264Context),
    .init           = x264_encode_init,
    .encode2        = x264_encode_frame,
    .close          = x264_encode_close
};
