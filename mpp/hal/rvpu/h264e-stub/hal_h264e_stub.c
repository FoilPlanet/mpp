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

#include <fcntl.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "mpp_device.h"
#include "mpp_common.h"
#include "mpp_mem.h"
#include "mpp_log.h"

#include "h264_syntax.h"
#include "hal_h264e_stub.h"

#ifdef USE_VPU_NVIDIA
#include "hal_h264e_nv.h"
#endif

#ifdef USE_SOFT_VPU_X264
#include "hal_h264e_x264.h"
#endif

#ifdef _VPU_STUB_
#define DEF_RVPU_SOCKNAME       "/dev/anbox_audio"
static int remote_fd = -1;
#endif

/**
 * RVPU channel client info
 */
typedef struct _client_info {
    unsigned char type;         /**< 0:playback 1:recording 2:screenshot(*) */
} ClientInfo;

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
#endif

#ifdef _VPU_STUB_
    if (-1 == remote_fd) {  // connect to unix socket
        struct sockaddr_un addr;
        const char *srvname = DEF_RVPU_SOCKNAME;
        int fd, len;

        if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
            mpp_err("create socket with error: %s", strerror(errno));
            return MPP_ERR_OPEN_FILE;
        }

        // bind to a temporary path
    #ifdef CLIENT_BIND_PATH
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        sprintf(addr.sun_path, "/var/run/mpp%05d.socket", getpid());
        unlink(addr.sun_path);
        len = offsetof(struct sockaddr_un, sun_path) + strlen(addr.sun_path);
        if (bind(fd, (struct sockaddr*)&addr, len) < 0) {
            close(fd);
            mpp_err("bind to '%s' with error: %s", addr.sun_path, strerror(errno));
            return MPP_ERR_OPEN_FILE;
        }
    #endif /* CLIENT_BIND_PATH */

        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, srvname, strlen(srvname));
        len = offsetof(struct sockaddr_un, sun_path) + strlen(addr.sun_path);
        if (connect(fd, (struct sockaddr *)&addr, len) < 0) {
            close(fd);
            mpp_err("connect to '%s' with error: %s", addr.sun_path, strerror(errno));
            return MPP_ERR_OPEN_FILE;
        }

        if ((remote_fd = fd) > 0) { // create a channel for streaming
            int nread;
            ClientInfo cmd = { 0x02 }, rsp;
            mpp_log("connected to '%s'", addr.sun_path);
            (void) write(remote_fd, &cmd, sizeof(cmd));
            nread = read(remote_fd, &rsp, sizeof(rsp));
            if (nread < 0 || rsp.type != cmd.type) {
                mpp_err("invalid response type %d (%s)", rsp.type, sizeof(rsp));
            }
        }
    }

    if (remote_fd > 0) {
        char buf[64];
        size_t len = sizeof(H264eHalContext);
        int n = sprintf(buf, "init:%d:", len);
        send(remote_fd, buf, n,   MSG_NOSIGNAL);
        send(remote_fd, ctx, len, MSG_NOSIGNAL);
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
#endif

#ifdef _VPU_STUB_
    if (remote_fd > 0) {
        char buf[64];
        int n = sprintf(buf, "deinit:0:");
        send(remote_fd, buf, n, MSG_NOSIGNAL);
        close(remote_fd);
        remote_fd = -1;
    }
#endif

    return ret;
}

MPP_RET hal_h264e_stub_gen_regs(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    H264eHalContext *ctx  = (H264eHalContext *)hal;
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
    if (remote_fd > 0) {
        char buf[64];
        int n = sprintf(buf, "gen_regs:0:");
        send(remote_fd, buf, n, MSG_NOSIGNAL);
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
    if (remote_fd > 0) {
        char buf[64];
        int n = sprintf(buf, "start:0:");
        send(remote_fd, buf, n, MSG_NOSIGNAL);
    }
#endif

    return ret;
}

MPP_RET hal_h264e_stub_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    H264eHalContext *ctx = (H264eHalContext *)hal;
    // HalEncTask  *info   = &task->enc;
    // MppBuffer   input   = info->input;
    // MppBuffer   output  = info->output;
    // unsigned char *pout = mpp_buffer_get_ptr(output);
    // size_t         size = mpp_buffer_get_size(output);

    // mpp_log("wait w:%u v:%u\n", ctx->cfg->prep.hor_stride, task->enc.valid);

#if defined(USE_VPU_NVIDIA)
    ret = hal_h264e_nv_wait(hal, task);
#elif defined(USE_SOFT_VPU_X264)
    ret = hal_h264e_x264_wait(hal, task);
#endif

#ifdef _VPU_STUB_
    if (remote_fd > 0) {
        char buf[64];
        int n = sprintf(buf, "wait:0:");
        send(remote_fd, buf, n, MSG_NOSIGNAL);
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
    if (remote_fd > 0) {
        char buf[64];
        int n = sprintf(buf, "reset:0:");
        send(remote_fd, buf, n, MSG_NOSIGNAL);
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
    if (remote_fd > 0) {
        char buf[64];
        int n = sprintf(buf, "flush:0:");
        send(remote_fd, buf, n, MSG_NOSIGNAL);
    }
#endif

    return ret;
}

MPP_RET hal_h264e_stub_control(void *hal, RK_S32 cmd, void *param)
{
    MPP_RET ret = MPP_OK;
    H264eHalContext *ctx = (H264eHalContext *)hal;
    MppEncCfgSet    *set;
    MppPacket        pkt;

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
        }
        break;

    case MPP_ENC_SET_RC_CFG:
        // TODO: do rate control check here
        mpp_log("control with cmd MPP_ENC_SET_RC_CFG\n");
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
        }
        break;

    case MPP_ENC_SET_OSD_PLT_CFG:
        mpp_log("control with cmd MPP_ENC_SET_OSD_PLT_CFG\n");
        break;

    case MPP_ENC_SET_SEI_CFG:
        mpp_log("control with cmd MPP_ENC_SET_SEI_CFG\n");
        ctx->sei_mode = *((MppEncSeiMode *)param);
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

#ifdef _VPU_STUB_
    if (remote_fd > 0) {
        char buf[64];
        size_t len = sizeof(H264eHalContext);
        int n = sprintf(buf, "control:%d:", len);
        send(remote_fd, buf, n,   MSG_NOSIGNAL);
        send(remote_fd, ctx, len, MSG_NOSIGNAL);
    }
#endif

    return ret;
}
