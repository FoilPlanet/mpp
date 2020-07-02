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

#define MODULE_TAG "mpp_hal_h264d_nv"

#include <string.h>
#include <limits.h>

extern "C" {
#include "mpp_device.h"
#include "mpp_common.h"
#include "mpp_mem.h"
#include "h264_syntax.h"
#include "hal_h264d_nv.h"
}

MPP_RET hal_h264d_nv_init(void *hal, MppHalCfg *cfg)
{
    return MPP_OK;
}

MPP_RET hal_h264d_nv_deinit(void *hal)
{
    return MPP_OK;
}

MPP_RET hal_h264d_nv_gen_regs(void *hal, HalTaskInfo *task)
{
    return MPP_OK;
}

MPP_RET hal_h264d_nv_start(void *hal, HalTaskInfo *task)
{
    return MPP_OK;
}

MPP_RET hal_h264d_nv_wait(void *hal, HalTaskInfo *task)
{
    return MPP_OK;
}

MPP_RET hal_h264d_nv_reset(void *hal)
{
    return MPP_OK;
}

MPP_RET hal_h264d_nv_flush(void *hal)
{
    return MPP_OK;
}

MPP_RET hal_h264d_nv_control(void *hal, MpiCmd cmd_type, void *param)
{
    return MPP_OK;
}

