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

#define MODULE_TAG "mpp_hal_h264e_nv"

#include <string.h>
#include <math.h>
#include <limits.h>

#include "mpp_device.h"
#include "mpp_common.h"
#include "mpp_mem.h"

#include "h264_syntax.h"
#include "hal_h264e_nv.h"


MPP_RET hal_h264e_nv_init(void *hal, MppHalCfg *cfg)
{
    H264eHalContext *ctx = (H264eHalContext *)hal;

    mpp_log("init %p", ctx);

    return MPP_OK;
}

MPP_RET hal_h264e_nv_deinit(void *hal)
{
    H264eHalContext *ctx = (H264eHalContext *)hal;

    mpp_log("deinit %p", ctx);

    return MPP_OK;
}

MPP_RET hal_h264e_nv_gen_regs(void *hal, HalTaskInfo *task)
{
    H264eHalContext *ctx = (H264eHalContext *)hal;
    
    //MppEncPrepCfg  *prep  = &ctx->cfg->prep;
    //MppEncCodecCfg *codec = &ctx->cfg->codec;
    (void)ctx;

    mpp_log("gen_regs v:%u\n", task->enc.valid);

#ifdef _VPU_STUB_
    // in container, set task regs
#else
    // in host, create task
#endif

    return MPP_OK;
}

MPP_RET hal_h264e_nv_start(void *hal, HalTaskInfo *task)
{
    H264eHalContext *ctx = (H264eHalContext *)hal;
    (void)ctx;

    mpp_log("start v:%u\n", task->enc.valid);

#ifdef _VPU_STUB_
    // in container, send task to queue
#else
    // in host, send task to nvenc
#endif

    return MPP_OK;
}

MPP_RET hal_h264e_nv_wait(void *hal, HalTaskInfo *task)
{
    H264eHalContext *ctx = (H264eHalContext *)hal;

#ifdef _VPU_STUB_
    // in container, wait result from queue
#else
    // in host, wait nvenc response
#endif

    return MPP_OK;
}

MPP_RET hal_h264e_nv_reset(void *hal)
{
    mpp_log("reset %p\n", hal);

    return MPP_OK;
}

MPP_RET hal_h264e_nv_flush(void *hal)
{
    H264eHalContext *ctx = (H264eHalContext *)hal;

    mpp_log("flush %p\n", ctx);

    return MPP_OK;
}

MPP_RET hal_h264e_nv_control(void *hal, RK_S32 cmd, void *arg)
{
    MPP_RET ret = MPP_OK;
    H264eHalContext  *ctx = (H264eHalContext *)hal;
    (void)cmd;
    (void)arg;

    switch (cmd) {
    case MPP_ENC_SET_PREP_CFG:
        break;
    
    case MPP_ENC_SET_RC_CFG:
        break;

    case MPP_ENC_SET_CODEC_CFG:
        break;

    default:
        // had handled in hal_h264e_stub_control
        break;
    } // (cmd)

    return ret;
}
