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

#define MODULE_TAG "mpp_hal_h264e_stub"

#include <string.h>
#include <math.h>
#include <limits.h>

#include "mpp_device.h"
#include "mpp_common.h"
#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_allocator.h"

#include "h264_syntax.h"
#include "hal_h264e_stub.h"
#include "rvpu_primitive.h"

#ifdef USE_VPU_NVIDIA
#include "hal_h264e_nv.h"
#endif

#ifdef USE_SOFT_VPU_X264
#include "hal_h264e_x264.h"
#endif

const int MAX_WAIT_TIMES = 50;     // 50ms

MPP_RET hal_h264e_stub_init(void *hal, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    H264eHalContext *ctx = (H264eHalContext *)hal;
    
    // mpp_log("init %p\n", ctx);

    ctx->int_cb = cfg->hal_int_cb;
  //ctx->buffers = mpp_calloc(h264e_hal_vpu_buffers, 1);
    ctx->param_buf = NULL;
    ctx->cfg = cfg->cfg;
    ctx->set = cfg->set;

#if defined(USE_VPU_NVIDIA)
    ret = hal_h264e_nv_init(hal, cfg);
#elif defined(USE_SOFT_VPU_X264)
    ret = hal_h264e_x264_init(hal, cfg);
#elif defined(_VPU_STUB_)
    if (NULL == (ctx->extra_info = mpp_calloc(HalRvpuInfo, 1))) {
        ret = MPP_ERR_NOMEM;
    } else {
        HalRvpuInfo *info = (HalRvpuInfo *)ctx->extra_info;
        if ((info->remote_fd = connect_remote()) > 0) {
            ret = send_remote(info->remote_fd, RVPU_PRIM_INIT, NULL, 0);
        } else {
            ret = MPP_NOK;
        }
    }
#endif

    return ret;
}

MPP_RET hal_h264e_stub_deinit(void *hal)
{
    MPP_RET ret = MPP_OK;
    H264eHalContext *ctx = (H264eHalContext *)hal;
    // mpp_log("deinit %p\n", ctx);
    
    if (ctx->param_buf) {
        mpp_free(ctx->param_buf);
        ctx->param_buf = NULL;
    }

#if defined(USE_VPU_NVIDIA)
    ret = hal_h264e_nv_deinit(hal);
#elif defined(USE_SOFT_VPU_X264)
    ret = hal_h264e_x264_deinit(hal);
#elif defined(_VPU_STUB_)
    if (ctx->extra_info != NULL) {
        HalRvpuInfo *info = (HalRvpuInfo *)ctx->extra_info;
        if (info->remote_fd > 0) {
            disconnect_remote(info->remote_fd);
        }
        MPP_FREE(ctx->extra_info);
    }
#endif

    return ret;
}

MPP_RET hal_h264e_stub_gen_regs(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    (void) task;
    // MppEncPrepCfg   *prep = &ctx->cfg->prep;
    
    // H264eHwCfg *hw_cfg   = &ctx->hw_cfg;
    // HalEncTask *enc_task = &task->enc;

    // RK_U32 mbs_in_row, mbs_in_col; 

    // mpp_log("gen_regs v:%u\n", task->enc.valid);

    // mbs_in_row = (prep->width + 15) / 16;
    // mbs_in_col = (prep->height + 15) / 16;

#if defined(USE_VPU_NVIDIA)
    ret = hal_h264e_nv_gen_regs(hal, task);
#elif defined(USE_SOFT_VPU_X264)
    ret = hal_h264e_x264_gen_regs(hal, task);
#endif

#ifdef _VPU_STUB_
    H264eHalContext *ctx  = (H264eHalContext *)hal;
    HalRvpuInfo *info = (HalRvpuInfo *)ctx->extra_info;

    if (info && info->remote_fd > 0) {
        send_remote(info->remote_fd, RVPU_PRIM_REGS, NULL, 0);
    }
#endif

    return ret;
}

MPP_RET hal_h264e_stub_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;

    // mpp_log("start v:%u\n", task->enc.valid);

#if defined(USE_VPU_NVIDIA)
    ret = hal_h264e_nv_start(hal, task);
#elif defined(USE_SOFT_VPU_X264)
    ret = hal_h264e_x264_start(hal, task);
#endif

#ifdef _VPU_STUB_
    H264eHalContext *ctx  = (H264eHalContext *)hal;
    HalRvpuInfo *info = (HalRvpuInfo *)ctx->extra_info;

    if (info && info->remote_fd > 0) {
        MppBuffer input = task->enc.input;
        MppBufferInfo binfo;
        mpp_buffer_info_get(input, &binfo);
        if (binfo.type == MPP_BUFFER_TYPE_ION) {
            // mpp_buffer_inc_ref(input);
            binfo.fd = 0;       // requires ion_map from binfo.hdl
            binfo.ptr = NULL;   // required ion_import
            send_remote(info->remote_fd, RVPU_PRIM_START, &binfo, sizeof(binfo));
        } else {
            send_remote(info->remote_fd, RVPU_PRIM_START, NULL, 0);
        }
    }
#endif

    return ret;
}

MPP_RET hal_h264e_stub_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;

    // mpp_log("wait w:%u v:%u\n", ctx->cfg->prep.hor_stride, task->enc.valid);

#if defined(USE_VPU_NVIDIA)
    ret = hal_h264e_nv_wait(hal, task);
#elif defined(USE_SOFT_VPU_X264)
    ret = hal_h264e_x264_wait(hal, task);
#endif

#ifdef _VPU_STUB_
    H264eHalContext *ctx  = (H264eHalContext *)hal;
    HalRvpuInfo *info = (HalRvpuInfo *)ctx->extra_info;

    if (info && info->remote_fd > 0) {
        size_t    outsize = 0;
        MppBuffer output  = task->enc.output;
        size_t    bufsize = mpp_buffer_get_size(output);
        volatile uint32_t *ptag = (uint32_t *)mpp_buffer_get_ptr(output);

        MppBufferInfo binfo;
        mpp_buffer_info_get(output, &binfo);
        if (binfo.type == MPP_BUFFER_TYPE_ION) {
            int times = MAX_WAIT_TIMES;

          #ifndef RECV_SOCKET
            // set output buffer ready-flag
            *ptag = bufsize;
          #endif

            binfo.fd = 0;       // requires ion_map from binfo.hdl
            binfo.ptr = NULL;   // required ion_import
            send_remote(info->remote_fd, RVPU_PRIM_WAIT, &binfo, sizeof(binfo));
            // wait output buffer ready
            while (times-- > 0) {
                usleep(1000);

              #ifdef RECV_SOCKET
                MppBufferInfo resp;
                RVPU_PRIM_CODE prim = RVPU_PRIM_UNDEFINED;
                if (prim == RVPU_PRIM_WAIT && 
                    sizeof(resp) == recv_remote(info->remote_fd, &prim, &resp, sizeof(resp))) {
                    outsize = resp.size;
                    break;
                }
              #else /* !RECV_SOCKET */
                if (*ptag < bufsize) {
                    output = NULL;
                    mpp_buffer_import(&output, (MppBufferInfo *)(ptag + 1));
                    if (NULL != output) {
                        //mpp_buffer_put(task->enc.output);
                        task->enc.output = output;
                        mpp_buffer_inc_ref(output);
                        outsize = *ptag;    // packet size
                    } else {
                        mpp_err("invalid packet buffer with size %u", *ptag);
                    }
                    break;
                }
              #endif /* RECV_SOCKET */
            }   // while (times)

            if (outsize == 0) {
                mpp_log("read response timeout");
            }
        } else {
            send_remote(info->remote_fd, RVPU_PRIM_WAIT, NULL, 0);
        }

        if (outsize > 0) {
            h264e_feedback *feedback = &ctx->feedback;
            IOInterruptCB  *int_cb   = &ctx->int_cb;
            feedback->hw_status = 0;
            feedback->out_strm_size = outsize;
            task->enc.length = outsize;
            if (int_cb->callBack) {
                RcHalResult result;
                result.bits = outsize * 8;
                result.type = ((RcSyntax *)task->enc.syntax.data)->type;
                feedback->result = &result;
                // mpp_log("callback %p", int_cb->opaque);
                int_cb->callBack(int_cb->opaque, feedback);
            }
        }
    }
#endif

    return ret;
}

MPP_RET hal_h264e_stub_reset(void *hal)
{
    MPP_RET ret = MPP_OK;

    mpp_log("reset %p\n", hal);

#if defined(USE_VPU_NVIDIA)
    ret = hal_h264e_nv_reset(hal);
#elif defined(USE_SOFT_VPU_X264)
    ret = hal_h264e_x264_reset(hal);
#endif

#ifdef _VPU_STUB_
    H264eHalContext *ctx  = (H264eHalContext *)hal;
    HalRvpuInfo *info = (HalRvpuInfo *)ctx->extra_info;
    if (info && info->remote_fd > 0) {
        send_remote(info->remote_fd, RVPU_PRIM_RESET, NULL, 0);
    }
#endif

    return ret;
}

MPP_RET hal_h264e_stub_flush(void *hal)
{
    MPP_RET ret = MPP_OK;

    mpp_log("flush %p\n", hal);

#if defined(USE_VPU_NVIDIA)
    ret = hal_h264e_nv_flush(hal);
#elif defined(USE_SOFT_VPU_X264)
    ret = hal_h264e_x264_flush(hal);
#endif

#ifdef _VPU_STUB_
    H264eHalContext *ctx  = (H264eHalContext *)hal;
    HalRvpuInfo *info = (HalRvpuInfo *)ctx->extra_info;

    if (info && info->remote_fd > 0) {
        send_remote(info->remote_fd, RVPU_PRIM_FLUSH, NULL, 0);
    }
#endif

    return ret;
}

MPP_RET hal_h264e_stub_control(void *hal, RK_S32 cmd, void *param)
{
    MPP_RET ret = MPP_OK;
    H264eHalContext *ctx  = (H264eHalContext *)hal;
    MppEncCfgSet    *set;
    MppPacket        pkt;

#ifdef _VPU_STUB_
    HalRvpuInfo *info = (HalRvpuInfo *)ctx->extra_info;
#endif

    switch (cmd) {
    case MPP_ENC_GET_EXTRA_INFO:
        mpp_log("control with cmd MPP_ENC_GET_EXTRA_INFO\n");
        pkt = ctx->packeted_param;
        if (pkt) {
            // size_t offset = 0;
            MppPacket *pkt_out  = (MppPacket *)param;
            *pkt_out = pkt;
        }
        break;

    case MPP_ENC_SET_PREP_CFG:
        mpp_log("control with cmd MPP_ENC_SET_PREP_CFG\n");
        if (NULL != (set = ctx->set)) {
            MppEncPrepCfg *prep = &set->prep;
            RK_U32 change = prep->change;

            if (change & MPP_ENC_PREP_CFG_CHANGE_INPUT) {
                if ((prep->width < 0 || prep->width > 1920) ||
                    (prep->height < 0 || prep->height > 3840) ||
                    (prep->hor_stride < 0 || prep->hor_stride > 3840) ||
                    (prep->ver_stride < 0 || prep->ver_stride > 3840)) {
                    mpp_err("invalid input w:h [%d:%d] [%d:%d]\n",
                            prep->width, prep->height,
                            prep->hor_stride, prep->ver_stride);
                    ret = MPP_NOK;
                }
            }

            if (change & MPP_ENC_PREP_CFG_CHANGE_FORMAT) {
                if ((prep->format < MPP_FRAME_FMT_RGB &&
                    prep->format >= MPP_FMT_YUV_BUTT) ||
                    prep->format >= MPP_FMT_RGB_BUTT) {
                    mpp_err("invalid format %d\n", prep->format);
                    ret = MPP_NOK;
                }
            }

          #ifdef _VPU_STUB_
            if (info && info->remote_fd > 0) {
                send_remote(info->remote_fd, RVPU_PRIM_CONTROL_PREP, prep, sizeof(*prep));
            }
          #endif
        }
        break;

    case MPP_ENC_SET_RC_CFG:
        // TODO: do rate control check here
        mpp_log("control with cmd MPP_ENC_SET_RC_CFG\n");
        if (NULL != (set = ctx->set)) {
          #ifdef _VPU_STUB_
            MppEncRcCfg *rc = &ctx->set->rc;
            if (info && info->remote_fd > 0) {
                send_remote(info->remote_fd, RVPU_PRIM_CONTROL_RC, rc, sizeof(*rc));
            }
          #endif
        }
        break;

    case MPP_ENC_SET_CODEC_CFG:
        mpp_log("control with cmd MPP_ENC_SET_CODEC_CFG\n");
        if (NULL != (set = ctx->set)) {
            MppEncH264Cfg *src = &ctx->set->codec.h264;
            MppEncH264Cfg *dst = &ctx->cfg->codec.h264;
            RK_U32 change = src->change;

            // TODO: do codec check first

            if (change & MPP_ENC_H264_CFG_STREAM_TYPE)
                dst->stream_type = src->stream_type;
            if (change & MPP_ENC_H264_CFG_CHANGE_PROFILE) {
                dst->profile = src->profile;
                dst->level = src->level;
            }
            if (change & MPP_ENC_H264_CFG_CHANGE_ENTROPY) {
                dst->entropy_coding_mode = src->entropy_coding_mode;
                dst->cabac_init_idc = src->cabac_init_idc;
            }
            if (change & MPP_ENC_H264_CFG_CHANGE_TRANS_8x8)
                dst->transform8x8_mode = src->transform8x8_mode;
            if (change & MPP_ENC_H264_CFG_CHANGE_CONST_INTRA)
                dst->constrained_intra_pred_mode = src->constrained_intra_pred_mode;
            if (change & MPP_ENC_H264_CFG_CHANGE_CHROMA_QP) {
                dst->chroma_cb_qp_offset = src->chroma_cb_qp_offset;
                dst->chroma_cr_qp_offset = src->chroma_cr_qp_offset;
            }
            if (change & MPP_ENC_H264_CFG_CHANGE_DEBLOCKING) {
                dst->deblock_disable = src->deblock_disable;
                dst->deblock_offset_alpha = src->deblock_offset_alpha;
                dst->deblock_offset_beta = src->deblock_offset_beta;
            }
            if (change & MPP_ENC_H264_CFG_CHANGE_LONG_TERM)
                dst->use_longterm = src->use_longterm;
            if (change & MPP_ENC_H264_CFG_CHANGE_QP_LIMIT) {
                dst->qp_init = src->qp_init;
                dst->qp_max = src->qp_max;
                dst->qp_min = src->qp_min;
                dst->qp_max_step = src->qp_max_step;
            }
            if (change & MPP_ENC_H264_CFG_CHANGE_INTRA_REFRESH) {
                dst->intra_refresh_mode = src->intra_refresh_mode;
                dst->intra_refresh_arg = src->intra_refresh_arg;
            }
            if (change & MPP_ENC_H264_CFG_CHANGE_SLICE_MODE) {
                dst->slice_mode = src->slice_mode;
                dst->slice_arg = src->slice_arg;
            }
            if (change & MPP_ENC_H264_CFG_CHANGE_VUI) {
                dst->vui = src->vui;
            }
            if (change & MPP_ENC_H264_CFG_CHANGE_SEI) {
                dst->sei = src->sei;
            }

            /*
            * NOTE: use OR here for avoiding overwrite on multiple config
            * When next encoding is trigger the change flag will be clear
            */
            dst->change |= change;
            src->change = 0;

          #ifdef _VPU_STUB_
            if (info && info->remote_fd > 0) {
                send_remote(info->remote_fd, RVPU_PRIM_CONTROL_CODEC, dst, sizeof(*dst));
            }
          #endif
        }
        break;

    case MPP_ENC_SET_OSD_PLT_CFG:
        mpp_log("control with cmd MPP_ENC_SET_OSD_PLT_CFG\n");
        break;

    case MPP_ENC_SET_SEI_CFG:
        // mpp_log("control with cmd MPP_ENC_SET_SEI_CFG\n");
        ctx->sei_mode = *((MppEncSeiMode *)param);

      #ifdef _VPU_STUB_
        if (info && info->remote_fd > 0) {
            send_remote(info->remote_fd, RVPU_PRIM_CONTROL_SEI, &ctx->sei_mode, sizeof(ctx->sei_mode));
        }
      #endif
        break;

    case MPP_ENC_GET_HDR_SYNC:
    case MPP_ENC_GET_PREP_CFG:
    case MPP_ENC_GET_CODEC_CFG:
    case MPP_ENC_SET_ROI_CFG:
    case MPP_ENC_SET_OSD_DATA_CFG:
    case MPP_ENC_GET_OSD_CFG:
    case MPP_ENC_PRE_ALLOC_BUFF:
    case MPP_ENC_GET_SEI_DATA:
        mpp_log("ignored cmd %d\n", cmd);
        break;

    default:
        mpp_err("unrecognizable cmd type %d", cmd);
        ret = MPP_NOK;
        break;
    }

#if defined(USE_VPU_NVIDIA)
    ret = hal_h264e_nv_control(hal, cmd, param);
#elif defined(USE_SOFT_VPU_X264)
    ret = hal_h264e_x264_control(hal, cmd, param);
#endif

    return ret;
}
