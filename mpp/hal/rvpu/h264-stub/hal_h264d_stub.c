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

#define MODULE_TAG "mpp_hal_h264d_stub"

#include "mpp_device.h"
#include "mpp_common.h"
#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_allocator.h"

#include "h264_syntax.h"
#include "hal_h264d_global.h"

#include "hal_h264d_stub.h"
#include "rvpu_primitive.h"

#ifdef USE_VPU_NVIDIA
#include "hal_h264d_nv.h"
#endif

// TODO: save in context
#ifdef _VPU_STUB_
static int remote_fd = -1;
#endif

MPP_RET hal_h264d_stub_init(void *hal, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    H264dHalCtx_t *ctx = (H264dHalCtx_t *)hal;

    ctx->fast_mode = cfg->fast_mode;
    // ctx->priv = NULL;
    // ctx->reg_ctx
    // ctx->frame_slots

#if defined(USE_VPU_NVIDIA)
    ret = hal_h264d_nv_init(hal, cfg);
#endif

#ifdef _VPU_STUB_
    if ((remote_fd = connect_remote()) > 0) {
        ret = send_remote(remote_fd, RVPU_PRIM_INIT, NULL, 0);
    }
#endif

    return ret;
}

MPP_RET hal_h264d_stub_deinit(void *hal)
{
    MPP_RET ret = MPP_OK;

#if defined(USE_VPU_NVIDIA)
    ret = hal_h264d_nv_deinit(hal);
#endif

#ifdef _VPU_STUB_
    if (remote_fd > 0) {
        disconnect_remote(remote_fd);
    }
#endif

    return ret;
}

MPP_RET hal_h264d_stub_gen_regs(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;

#if defined(USE_VPU_NVIDIA)
    ret = hal_h264d_nv_gen_regs(hal, task);
#endif

#ifdef _VPU_STUB_
    if (remote_fd > 0) {
        send_remote(remote_fd, RVPU_PRIM_REGS, NULL, 0);
    }
#endif

    return ret;
}

MPP_RET hal_h264d_stub_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    (void)hal;
#if defined(USE_VPU_NVIDIA)
    ret = hal_h264d_nv_start(hal, task);
#endif

#ifdef _VPU_STUB_
    if (remote_fd > 0) {
        MppBuffer input = task->dec.input;
        MppBufferInfo binfo;
        mpp_buffer_info_get(input, &binfo);
        if (binfo.type == MPP_BUFFER_TYPE_ION) {
            // mpp_buffer_inc_ref(input);
            binfo.fd = 0;       // requires ion_map from binfo.hdl
            binfo.ptr = NULL;   // required ion_import
            send_remote(remote_fd, RVPU_PRIM_START, &binfo, sizeof(binfo));
        } else {
            send_remote(remote_fd, RVPU_PRIM_START, NULL, 0);
        }
    }
#endif

    return ret;
}

MPP_RET hal_h264d_stub_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    (void)hal;

#if defined(USE_VPU_NVIDIA)
    ret = hal_h264d_nv_wait(hal, task);
#endif

#ifdef _VPU_STUB_
    if (remote_fd > 0) {
        MppBuffer     output = task->dec.output;
        MppBufferInfo binfo;
        mpp_buffer_info_get(output, &binfo);
        if (binfo.type == MPP_BUFFER_TYPE_ION) {
            // unsigned char *pbuf  = mpp_buffer_get_ptr(output);
            // TODO: clear output buffer ready-flag
            // pbuf[0] = '\0';
            binfo.fd = 0;       // requires ion_map from binfo.hdl
            binfo.ptr = NULL;   // required ion_import
            send_remote(remote_fd, RVPU_PRIM_WAIT, &binfo, sizeof(binfo));
            // TODO: set output buffer ready-flag
            // while (!puf[0]) { usleep(100); }
        } else {
            send_remote(remote_fd, RVPU_PRIM_WAIT, NULL, 0);
        }
    }
#endif

    return ret;
    
}

MPP_RET hal_h264d_stub_reset(void *hal)
{
    MPP_RET ret = MPP_OK;
    mpp_log("reset %p\n", hal);

#if defined(USE_VPU_NVIDIA)
    ret = hal_h264d_nv_reset(hal);
#endif

#ifdef _VPU_STUB_
    if (remote_fd > 0) {
        send_remote(remote_fd, RVPU_PRIM_RESET, NULL, 0);
    }
#endif

    return ret;
}

MPP_RET hal_h264d_stub_flush(void *hal)
{
    MPP_RET ret = MPP_OK;
    mpp_log("flush %p\n", hal);

#if defined(USE_VPU_NVIDIA)
    ret = hal_h264d_nv_flush(hal);
#endif

#ifdef _VPU_STUB_
    if (remote_fd > 0) {
        send_remote(remote_fd, RVPU_PRIM_FLUSH, NULL, 0);
    }
#endif

    return ret;
}

MPP_RET hal_h264d_stub_control(void *hal, MpiCmd cmd_type, void *param)
{
    (void)hal;
    (void)cmd_type;
    (void)param;
    return MPP_OK;
}