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

#define MODULE_TAG "mpp_hal_h264e_x264"

#include <string.h>
#include <math.h>
#include <limits.h>

#include "mpp_device.h"
#include "mpp_common.h"
#include "mpp_mem.h"

#include "h264_syntax.h"
#include "hal_h264e_x264.h"

#include "h264_syntax.h"

#include "x264.h"
#include "x264_config.h"

/**
 * Context for x264 encoder, keep in H264eHalContext::extra_info
 */
typedef struct hal_x264_info_s {
    x264_param_t    param;
    x264_t         *encoder;
    x264_picture_t *in_pic;
    x264_picture_t *out_pic;

    uint32_t        bitrate;
} Halx264ExtraInfo;

/**
 * __assert2 referenced in libx264.a
 */
extern int __assert2(int) __attribute__((weak));
int __assert2(int val)
{
    if (!val) {
        mpp_err("assert in x264\n");
    }
}

static int get_rf_constant(Halx264ExtraInfo *info)
{
    (void)info;
    /*
     *  bitrate level      | rf_constant   |  bitrate
     *  -------------------|---------------|-----------
     *  BIT_LOW_LEVEL      | 32            |  (0, 64]
     *  BIT_MEDIUM_LEVEL   | 29            |  [65, 128]
     *  BIT_STANDARD_LEVEL | 26            |  [129, 256]
     *  BIT_HIGH_LEVEL     | 24            |  [257, 384]
     *                     |               |  [385, 512]
     */
    return 24;
}

static MPP_RET reinit_x264_encoder(x264_param_t *param, Halx264ExtraInfo *info)
{
    int max_cached_frames;

    mpp_log("init x264 %dx%d %d fps", param->i_width, param->i_height, param->i_fps_num);

    if (NULL != info->encoder) {
        x264_picture_clean(info->in_pic);
        x264_encoder_close(info->encoder);
        MPP_FREE(info->in_pic);
        MPP_FREE(info->out_pic);
    }

    if (NULL == (info->encoder = x264_encoder_open(param))) {
        mpp_err("x264_encoder_open failed");
        return MPP_NOK;
    }

    max_cached_frames = x264_encoder_maximum_delayed_frames(info->encoder);
    mpp_log("max caches frames %d", max_cached_frames);

    info->in_pic  = mpp_calloc(x264_picture_t, 1);
    info->out_pic = mpp_calloc(x264_picture_t, 1);
    x264_picture_init(info->out_pic);

    int i_csp = param->i_csp; // X264_CSP_I420
    if (x264_picture_alloc(info->in_pic, i_csp, param->i_width, param->i_height)) {
        mpp_err("x264_picture_alloc failed");
        return MPP_ERR_NOMEM;
    }
    info->in_pic->img.i_csp   = i_csp;
    info->in_pic->img.i_plane = 3;

    return MPP_OK;
}

MPP_RET hal_h264e_x264_init(void *hal, MppHalCfg *cfg)
{
    H264eHalContext *ctx = (H264eHalContext *)hal;
    Halx264ExtraInfo *info;
    x264_param_t *param;
    MppEncCfgSet *ecfg = ctx->cfg;
    int ret;

    (void)cfg;
    mpp_log("init %p", hal);

    ctx->extra_info = mpp_calloc(Halx264ExtraInfo, 1);
    if (NULL == (info = (Halx264ExtraInfo *)ctx->extra_info)) {
        return MPP_ERR_NOMEM;
    }

    param = &info->param;
    if (0 != (ret = x264_param_default_preset(param, "fast" , "zerolatency"))) {
        mpp_log("x264_param_default_preset: %d", ret);
        return MPP_ERR_VPU_CODEC_INIT;
    }

    param->rc.f_rf_constant = get_rf_constant(info);
    param->b_repeat_headers = 1;                // PS/PPS
    param->rc.i_rc_method = X264_RC_CRF;        // CQP, CRF, ABR

    param->i_width       = ecfg->prep.width;
    param->i_height      = ecfg->prep.height;
    param->i_csp         = X264_CSP_I420;       // X264_CSP_BGRA
    param->i_frame_total = 0;                   // 编码总帧数. 默认用0.
    param->i_keyint_max  = 3;

    param->i_fps_den     = ecfg->rc.fps_in_denorm;  // normally 1
    param->i_fps_num     = ecfg->rc.fps_in_num;     // frame rate
    param->i_timebase_den = param->i_fps_num;
    param->i_timebase_num = param->i_fps_den;
    param->i_cqm_preset   = X264_CQM_FLAT;

    param->analyse.i_me_method = X264_ME_HEX;
    param->analyse.i_subpel_refine = 2;
    param->i_frame_reference = 1;
    param->analyse.b_mixed_references = 0;
    param->analyse.i_trellis = 0;

    // i_threads = N并行编码的时候如果b_sliced_threads=1那么是并行slice编码，
    // 如果b_sliced_threads=0，那么是并行frame编码。并行slice无延时，并行frame有延时
    param->b_sliced_threads  = 0;
    param->i_threads = 4; 
        
    param->analyse.b_transform_8x8 = 0;
    param->b_cabac = 0;
	param->b_deblocking_filter = 1;
    param->psz_cqm_file = NULL;
    param->analyse.i_weighted_pred = X264_WEIGHTP_NONE;
    param->rc.i_lookahead = 10;
    param->i_bframe = 0;

    return MPP_OK;
}

MPP_RET hal_h264e_x264_deinit(void *hal)
{
    H264eHalContext *ctx = (H264eHalContext *)hal;

    mpp_log("deinit %p", hal);

    hal_h264e_x264_flush(ctx);

    if (ctx->extra_info) {
        Halx264ExtraInfo *info = (Halx264ExtraInfo *)ctx->extra_info;
        x264_picture_clean(info->in_pic);
        x264_encoder_close(info->encoder);
        MPP_FREE(info->in_pic);
        MPP_FREE(info->out_pic);
        MPP_FREE(ctx->extra_info);
    }

    return MPP_OK;
}

MPP_RET hal_h264e_x264_gen_regs(void *hal, HalTaskInfo *task)
{
    H264eHalContext *ctx = (H264eHalContext *)hal;
    //MppEncPrepCfg  *prep  = &ctx->cfg->prep;
    //MppEncCodecCfg *codec = &ctx->cfg->codec;
    (void)ctx;
    (void)task;

    // mpp_log("gen_regs v:%u\n", task->enc.valid);

    // DO NOTHING

    return MPP_OK;
}

MPP_RET hal_h264e_x264_start(void *hal, HalTaskInfo *task)
{
    H264eHalContext *ctx = (H264eHalContext *)hal;
    (void)ctx;
    (void)task;

    // mpp_log("start v:%u\n", task->enc.valid);

    // DO NOTHING

    return MPP_OK;
}

MPP_RET hal_h264e_x264_wait(void *hal, HalTaskInfo *task)
{
    H264eHalContext *ctx = (H264eHalContext *)hal;
    Halx264ExtraInfo *info = (Halx264ExtraInfo *)ctx->extra_info;
    x264_picture_t *in_pic = info->in_pic;
    x264_nal_t *nal;
    int i, i_nal = 0, ret;
    MppBuffer   input  = task->enc.input;
    MppBuffer   output = task->enc.output;

    // mpp_log("wait w:%u v:%u\n", ctx->cfg->prep.hor_stride, task->enc.valid);

    // if (info->in_pic->img.i_plane == 3)
    size_t size = info->param.i_width * info->param.i_height;
    unsigned char *pbuf = mpp_buffer_get_ptr(input);

    memcpy(in_pic->img.plane[0], pbuf, size);
    memcpy(in_pic->img.plane[1], pbuf + size, size / 4);
    memcpy(in_pic->img.plane[2], pbuf + size * 5 / 4, size / 4);

    in_pic->i_type    = X264_TYPE_AUTO;
    in_pic->i_qpplus1 = 0;
    in_pic->param     = &info->param;
    in_pic->i_pts     = 0;

    ret = x264_encoder_encode(info->encoder, &nal, &i_nal, in_pic, info->out_pic);
    // is_keyframe == out_pic->b_keyframe;
    in_pic->i_pts++;

    if (ret < 0) {
        mpp_err("x264_encoder_encode error: %d", ret);
        return MPP_NOK;
    } else if (ret == 0) {
        // data is cached, do NOTHING
        mpp_log("x264_encoder_encode cached");
    } else {
        IOInterruptCB  *int_cb;
        h264e_feedback *feedback;
        int bsize = 0;
        pbuf = mpp_buffer_get_ptr(output);
        for (i = 0; i < i_nal; i++) {
            RK_U8 *payload = nal[i].p_payload;
            if (payload != NULL) {
                int i_payload = nal[i].i_payload;
                memcpy(pbuf, payload, i_payload);
                pbuf  += i_payload;
                bsize += i_payload;
            }
        }

        feedback = &ctx->feedback;
        feedback->hw_status = 0;
        feedback->out_strm_size = bsize;
        task->enc.length = bsize;

        int_cb = &ctx->int_cb;
        if (int_cb->callBack) {
            RcHalResult result;
            result.bits = bsize * 8;
            result.type = ((RcSyntax *)task->enc.syntax.data)->type;
            feedback->result = &result;
            // mpp_log("callback %p", int_cb->opaque);
            int_cb->callBack(int_cb->opaque, feedback);
        }
    }

    return MPP_OK;
}

MPP_RET hal_h264e_x264_reset(void *hal)
{
    mpp_log("reset %p\n", hal);
    return MPP_OK;
}

MPP_RET hal_h264e_x264_flush(void *hal)
{
    H264eHalContext *ctx = (H264eHalContext *)hal;

    mpp_log("flush %p\n", hal);

    if (ctx->extra_info) {
        Halx264ExtraInfo *info = (Halx264ExtraInfo *)ctx->extra_info;
        x264_picture_t pic_out;
        x264_nal_t *nal;
        int i_nal;
        if (x264_encoder_encode(info->encoder, &nal, &i_nal, NULL, &pic_out) < 0) {
            // NOTHING
        }
    }

    return MPP_OK;
}

MPP_RET hal_h264e_x264_control(void *hal, RK_S32 cmd, void *arg)
{
    MPP_RET ret = MPP_OK;
    H264eHalContext  *ctx = (H264eHalContext *)hal;
    Halx264ExtraInfo *info = ctx->extra_info;
    (void)cmd;
    (void)arg;

    switch (cmd) {
    case MPP_ENC_SET_PREP_CFG:
        if (info) {
            x264_param_t *param = &info->param;
            MppEncCfgSet *ecfg = ctx->set;
            param->i_width  = ecfg->prep.width;
            param->i_height = ecfg->prep.height;
        }
        break;
    
    case MPP_ENC_SET_RC_CFG:
        if (info) {
            x264_param_t *param = &info->param;
            MppEncCfgSet *ecfg = ctx->set;
            param->i_fps_den = ecfg->rc.fps_in_denorm;
            param->i_fps_num = ecfg->rc.fps_in_num;
        }
        break;

    case MPP_ENC_SET_CODEC_CFG:
        if (info) {
            x264_param_t *param = &info->param;
            MppEncCfgSet *ecfg = ctx->set;

            // TODO
            (void)ecfg;
            // ecfg->codec.h264.qp_init
            // ecfg->codec.h264.profile
            // ecfg->codec.h264.level

            ret = reinit_x264_encoder(param, info);
        }
        break;

    default:
        // had handled in hal_h264e_stub_control
        break;
    } // (cmd)

    return ret;
}