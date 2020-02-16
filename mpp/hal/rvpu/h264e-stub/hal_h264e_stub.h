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

#ifndef __HAL_H264E_STUB_H__
#define __HAL_H264E_STUB_H__

#include "mpp_buffer.h"
#include "mpp_hal.h"
#include "hal_task.h"
#include "hal_h264e_com.h"

/**
 * Codeing primtive sent to remote server
 *   message format: {code}:{length}:{data}
 */
typedef enum rvpu_primitive_t {
    RVPU_PRIM_UNDEFINED = 0,
    RVPU_PRIM_INIT,
    RVPU_PRIM_DEINIT,
    RVPU_PRIM_REGS,
    RVPU_PRIM_START,
    RVPU_PRIM_WAIT,
    RVPU_PRIM_CONTROL_PREP,
    RVPU_PRIM_CONTROL_RC,
    RVPU_PRIM_CONTROL_CODEC,
    RVPU_PRIM_CONTROL_SEI,
    RVPU_PRIM_CONTROL,
    RVPU_PRIM_RESET,
    RVPU_PRIM_FLUSH,
    RVPU_PRIM_MAX
} RVPU_PRIM_CODE;

MPP_RET hal_h264e_stub_init    (void *hal, MppHalCfg *cfg);
MPP_RET hal_h264e_stub_deinit  (void *hal);
MPP_RET hal_h264e_stub_gen_regs(void *hal, HalTaskInfo *task);
MPP_RET hal_h264e_stub_start   (void *hal, HalTaskInfo *task);
MPP_RET hal_h264e_stub_wait    (void *hal, HalTaskInfo *task);
MPP_RET hal_h264e_stub_reset   (void *hal);
MPP_RET hal_h264e_stub_flush   (void *hal);
MPP_RET hal_h264e_stub_control (void *hal, RK_S32 cmd_type, void *param);

#endif /* __HAL_H264E_STUB_H__ */
