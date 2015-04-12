/*
 * S5P H264 Video Encoder
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
#include "mfc_interface.h"
#include "SsbSipMfcApi.h"

#define MFC_DEV_NAME        "/dev/s3c-mfc"
#define BUF_SIZE            1024000

static const enum AVPixelFormat pixfmt_rgb24[] = {
    AV_PIX_FMT_BGR24, AV_PIX_FMT_RGB32, AV_PIX_FMT_NONE };

typedef struct X264Context 
{
    void* open_handle;
    SSBSIP_MFC_ENC_H264_PARAM enc_param;
    SSBSIP_MFC_ENC_INPUT_INFO input_info;
    SSBSIP_MFC_ENC_OUTPUT_INFO output_info;
} X264Context;

static av_cold int x264_encode_init(AVCodecContext *avctx)
{
    X264Context* const c = avctx->priv_data;
    int ret;
    unsigned int cache = CACHE;
    
    c->open_handle = SsbSipMfcEncOpen(&cache);
    if (c->open_handle == NULL)
    {
        av_log(avctx, AV_LOG_ERROR, "Error: MFC encoder open failure.\n");
        return AVERROR_INVALIDDATA;
    }

    memset(&c->enc_param, 0, sizeof(c->enc_param));
    c->enc_param.codecType = H264_ENC;
    c->enc_param.SourceWidth = avctx->width;
    c->enc_param.SourceHeight = avctx->height;
    c->enc_param.FrameRate = avctx->time_base.den * 1000;  // real frame rate = FrameRate/1000 (refer to S5PV210 datasheet Section 6.3.4.2.2)
    c->enc_param.FrameQp = 20;
    c->enc_param.IDRPeriod = 100;
    c->enc_param.SliceMode = 0;    // 0,1,2,4
    c->enc_param.RandomIntraMBRefresh = 0;
    c->enc_param.EnableFRMRateControl = 1;  // this must be 1 otherwise init error
    c->enc_param.EnableMBRateControl = 0;
    c->enc_param.Bitrate = 10 * 1000 * 1000;
    c->enc_param.QSCodeMin = 10;  // <=51
    c->enc_param.QSCodeMax = 50;  // <=51
    c->enc_param.CBRPeriodRf = 20;
    c->enc_param.PadControlOn = 0;
    c->enc_param.LumaPadVal = 0;  // <=255
    c->enc_param.CbPadVal = 0;  //<=255
    c->enc_param.CrPadVal = 0;  //<=255
    c->enc_param.FrameMap = NV12_LINEAR; // encoding input mode (0=linear, 1=tiled) 
 
    // H264 specific
    c->enc_param.ProfileIDC = 2; // 0=main, 1=high, 2=baseline
    c->enc_param.LevelIDC = 30;  // level 3.0
    c->enc_param.FrameQp_P = c->enc_param.FrameQp + 1;
    c->enc_param.FrameQp_B = c->enc_param.FrameQp + 3;
    c->enc_param.SliceArgument = 1;
    c->enc_param.NumberBFrames = 0;  //<=2
    c->enc_param.NumberReferenceFrames = 2;  //<=2
    c->enc_param.NumberRefForPframes = 2;  //<=2
    c->enc_param.LoopFilterDisable = 0;  // 0=enable, 1=disable
    c->enc_param.LoopFilterAlphaC0Offset = 0;  // <=6
    c->enc_param.LoopFilterBetaOffset = 0;  // <=6
    c->enc_param.SymbolMode = 0;  // 0=CAVLC, 1=CABAC
    c->enc_param.PictureInterlace = 0;  // Picture AFF 0=frame coding, 1=field coding, 2=adaptive
    c->enc_param.Transform8x8Mode = 0;  // 0=only 4x4 transform, 1=allow 8x8 trans, 2=only 8x8 trans
    c->enc_param.DarkDisable = 0;
    c->enc_param.SmoothDisable = 0;
    c->enc_param.StaticDisable = 0;
    c->enc_param.ActivityDisable = 0;
    
    ret = SsbSipMfcEncInit(c->open_handle, &c->enc_param);
    if (ret != MFC_RET_OK)
    {
        return AVERROR_INVALIDDATA;
    }

    memset(&c->input_info, 0, sizeof(c->input_info));

    ret = SsbSipMfcEncGetInBuf(c->open_handle, &c->input_info);
    if (ret != MFC_RET_OK)
    {
        return AVERROR_INVALIDDATA;
    }
    
    memset(&c->output_info, 0, sizeof(c->output_info));
    
    ret = SsbSipMfcEncGetOutBuf(c->open_handle, &c->output_info);
    if (ret != MFC_RET_OK)
    {
        return AVERROR_INVALIDDATA;
    }

    return 0;
}

static int x264_encode_frame(AVCodecContext *avctx, AVPacket *pkt,
                             const AVFrame *frame, int *got_packet)
{
    X264Context* const c = avctx->priv_data;
    int ret;

    *got_packet = 0;

    if (frame)
    {
        memcpy((char*)c->input_info.YVirAddr, frame->data[0], avctx->width * avctx->height);
        memcpy((char*)c->input_info.CVirAddr, frame->data[1], avctx->width * avctx->height / 4);
        memcpy((char*)c->input_info.CVirAddr + avctx->width * avctx->height / 4, frame->data[2],
            avctx->width * avctx->height / 4);
    }
    else
        return 0;

    ret = SsbSipMfcEncExe(c->open_handle);
    if (ret != MFC_RET_OK)
    {
        return 0;
    }

    ret = SsbSipMfcEncGetOutBuf(c->open_handle, &c->output_info);
    if (ret != MFC_RET_OK)
    {
        return 0;
    }

    ret = ff_alloc_packet(pkt, c->output_info.dataSize);
    if (ret < 0) {
        av_log(avctx, AV_LOG_ERROR, "Error: Allocate packet failure.\n");
        return AVERROR(ENOMEM);
    }

    memcpy(pkt->data, c->output_info.StrmVirAddr, c->output_info.dataSize);
    pkt->size = c->output_info.dataSize;

    *got_packet = 1;
    return 0;
}

static av_cold int x264_encode_close(AVCodecContext *avctx)
{
    X264Context* const c = avctx->priv_data;
    
    printf("%s : Line %d.\n", __FILE__, __LINE__);
    
    SsbSipMfcEncClose(c->open_handle);
    
    printf("%s : Line %d.\n", __FILE__, __LINE__);

    return 0;
}

AVCodec ff_s5px264_encoder = {
    .name           = "s5px264",
    .long_name      = NULL_IF_CONFIG_SMALL("S5P H264 video"),
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = AV_CODEC_ID_H264,
    .priv_data_size = sizeof(X264Context),
    .init           = x264_encode_init,
    .encode2        = x264_encode_frame,
    .close          = x264_encode_close
};
