/*
 * Copyright (c) 2019-2020 FoilPlanet. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define MODULE_TAG "mpp_hal_jpege_stub"

#include <string.h>

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_common.h"
#include "mpp_mem.h"

#include "mpp_hal.h"

#include "jpege_syntax.h"
#include "hal_jpege_stub.h"

#include "mpp_device.h"
#include "mpp_platform.h"

#include "turbojpeg.h"

#define LEN_PREPADDING          4       // MppEncoder::mPrePadding

/**
 * Context, refer to vpu/jpege/hal_jpege_bash.h
 */
typedef struct hal_jpege_ctx_s {
    IOInterruptCB       int_cb;
    MppDevCtx           dev_ctx;
  //JpegeBits           bits;
  //JpegeIocRegInfo     ioctl_info;
    tjhandle            tj_hdr;
    MppEncCfgSet        *cfg;
    MppEncCfgSet        *set;
    JpegeSyntax         syntax;
    MppHalApi           hal_api;
} HalJpegeCtx;

MPP_RET hal_jpege_stub_init(void *hal, MppHalCfg *cfg)
{
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;

  #ifdef LEN_PREPADDING
     mpp_log("init %p with prepadding %d bytes\n", ctx, LEN_PREPADDING);
  #else
     mpp_log("init %p\n", ctx);
  #endif

    ctx->tj_hdr = tjInitCompress();
    ctx->int_cb = cfg->hal_int_cb;
    ctx->cfg = cfg->cfg;
    ctx->set = cfg->set;

    return MPP_OK;
}

MPP_RET hal_jpege_stub_deinit(void *hal)
{
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;
    mpp_log("deinit %p\n", ctx);

    tjDestroy(ctx->tj_hdr);
    return MPP_OK;
}

MPP_RET hal_jpege_stub_gen_regs(void *hal, HalTaskInfo *task)
{
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;

    JpegeSyntax    *syntax  = &ctx->syntax;
    MppEncPrepCfg  *prep    = &ctx->cfg->prep;
    MppEncCodecCfg *codec   = &ctx->cfg->codec;
    RK_U32          width   = prep->width;
    RK_U32          height  = prep->height;
    MppFrameFormat  fmt     = prep->format;

    mpp_log("gen_regs v:%u\n", task->enc.valid);

    // initial frame settings in syntax
    syntax->width       = width;
    syntax->height      = height;
    syntax->hor_stride  = prep->hor_stride;
    syntax->ver_stride  = prep->ver_stride;
    syntax->format      = fmt;
    syntax->quality     = codec->jpeg.quant;

    return MPP_OK;
}

MPP_RET hal_jpege_stub_start(void *hal, HalTaskInfo *task)
{
    (void)hal;
    mpp_log("start v:%u\n", task->enc.valid);

    // DO NOTHING

    return MPP_OK;
}

static int fmt2TJPF(MppFrameFormat format) {
    int fmt = TJPF_RGB;

    switch (format) {
    case MPP_FMT_ARGB8888:
        fmt = TJPF_RGBA;
        break;
    case MPP_FMT_RGB888:
        fmt = TJPF_RGB;
        break;
    case MPP_FMT_ABGR8888:
        fmt = TJPF_BGRA;
        break;
    case MPP_FMT_YUV420SP_VU:
    default:
        mpp_err("unsupport MppFrameFormat %d\n", format);
        break;
    }
    return fmt;
}

MPP_RET hal_jpege_stub_wait(void *hal, HalTaskInfo *task)
{
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;

    JpegeSyntax *syntax = &ctx->syntax;
    HalEncTask  *info   = &task->enc;
    MppBuffer   input   = info->input;
    MppBuffer   output  = info->output;

    unsigned char *pout = mpp_buffer_get_ptr(output);
    size_t         size = mpp_buffer_get_size(output);
    JpegeFeedback  feedback;
    int res;

    mpp_log("wait s:%u q:%d v:%u\n", syntax->hor_stride, syntax->quality, task->enc.valid);

  #ifdef LEN_PREPADDING
    pout += LEN_PREPADDING;     // hacker header offset for minicap
  #endif

    res = tjCompress2(ctx->tj_hdr, 
        mpp_buffer_get_ptr(input), 
        syntax->width,
        syntax->hor_stride * 4,  // pitch = frame->stride * frame->bpp
        syntax->height,
        fmt2TJPF(syntax->format),
        &pout,
        &size,
        TJSAMP_420,
        syntax->quality * 10,    // 1 ~ 100
        TJFLAG_FASTDCT | TJFLAG_NOREALLOC
    );

    if (res != 0) {
        mpp_err("tjCompress2: %s\n", tjGetErrorStr());
        return MPP_NOK;
    }

    // feedback callback
    feedback.stream_length = size;
    feedback.hw_status = 0;
    task->enc.length = size;
    ctx->int_cb.callBack(ctx->int_cb.opaque, &feedback);

    return MPP_OK;
}

MPP_RET hal_jpege_stub_reset(void *hal)
{
    mpp_log("reset %p\n", hal);
    return MPP_OK;
}

MPP_RET hal_jpege_stub_flush(void *hal)
{
    mpp_log("flush %p\n", hal);
    return MPP_OK;
}

MPP_RET hal_jpege_stub_control(void *hal, RK_S32 cmd, void *param)
{
    MPP_RET ret = MPP_OK;
    MppEncPrepCfg *cfg;
    HalJpegeCtx   *ctx;

    switch (cmd) {
    case MPP_ENC_SET_PREP_CFG:
        cfg = (MppEncPrepCfg *)param;
        if (cfg->width < 16 && cfg->width > 8192) {
            mpp_err("jpege: invalid width %d is not in range [16..8192]\n", cfg->width);
            ret = MPP_NOK;
        }

        if (cfg->height < 16 && cfg->height > 8192) {
            mpp_err("jpege: invalid height %d is not in range [16..8192]\n", cfg->height);
            ret = MPP_NOK;
        }

        if (cfg->format != MPP_FMT_YUV420SP     &&      // NV12
            cfg->format != MPP_FMT_YUV420P      &&
            cfg->format != MPP_FMT_YUV420SP_VU  &&      // NV21
            cfg->format != MPP_FMT_YUV422SP_VU  &&      // NV42
            cfg->format != MPP_FMT_YUV422_YUYV  &&
            cfg->format != MPP_FMT_YUV422_UYVY  &&
            cfg->format != MPP_FMT_RGB888       &&
            cfg->format != MPP_FMT_BGR888       &&
            cfg->format != MPP_FMT_ARGB8888) {
            mpp_err("jpege: invalid format %d is not support\n", cfg->format);
            ret = MPP_NOK;
        }
        break;

    case MPP_ENC_SET_RC_CFG:
        mpp_log("control with cmd MPP_ENC_SET_RC_CFG\n");
        break;

    case MPP_ENC_SET_OSD_PLT_CFG:
        mpp_log("control with cmd MPP_ENC_SET_OSD_PLT_CFG\n");
        break;

    case MPP_ENC_SET_SEI_CFG:
        mpp_log("control with cmd MPP_ENC_SET_SEI_CFG\n");
        break;

    case MPP_ENC_GET_PREP_CFG:
    case MPP_ENC_GET_CODEC_CFG:
    case MPP_ENC_SET_IDR_FRAME:
    case MPP_ENC_SET_OSD_DATA_CFG:
    case MPP_ENC_GET_OSD_CFG:
    case MPP_ENC_GET_HDR_SYNC:
    case MPP_ENC_GET_EXTRA_INFO:
    case MPP_ENC_GET_SEI_DATA:
        mpp_log("ignored cmd %d\n", cmd);
        break;

    case MPP_ENC_SET_CODEC_CFG:
        ctx = (HalJpegeCtx *)hal;
        MppEncJpegCfg *src = &ctx->set->codec.jpeg;
        MppEncJpegCfg *dst = &ctx->cfg->codec.jpeg;
        RK_U32 change = src->change;

        if (change & MPP_ENC_JPEG_CFG_CHANGE_QP) {
            if (src->quant < 0 || src->quant > 10) {
                mpp_err("jpege: invalid quality level %d is not in range [0..10] set to default 8\n");
                src->quant = 8;
            }
            dst->quant = src->quant;
        }

        dst->change = 0;
        src->change = 0;
        mpp_log("control with cmd MPP_ENC_SET_CODEC_CFG\n");
        break;

    default:
        mpp_err("No correspond cmd(%08x) found, and can not config!", cmd);
        ret = MPP_NOK;
        break;
    } // switch (cmd)

    return ret;
}
