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

#ifndef __HAL_RVPU_API_H__
#define __HAL_RVPU_API_H__

// defined in rk_type.h
#ifndef __RK_TYPE_H__
typedef void* MppCtx;
#endif

#ifndef __RK_MPI_H__
typedef void* MppApi;
#endif

#ifdef __cplusplus
#include <functional>

typedef std::function<void (void *, size_t)> rvpu_callback_t;
void set_rvpu_callback(MppCtx ctx,
    rvpu_callback_t on_ctrl_prep, 
    rvpu_callback_t on_packet,
    rvpu_callback_t on_flush);

extern "C" {

#else /* !__cplusplus */
typedef void (*rvpu_callback_t)(void *, size_t);
#endif

int rvpu_target_init(void *owner, MppCtx ctx);

/**
 * Process socket fd for rvpu-stub, \ref process_rvpu
 * @param fd socket fd
 * @param ctx mpp context
* @return 0 for success, else wise errcode
 */
int process_rvpu(int fd, MppCtx ctx);

/**
 * Process incoming data from rvpu-stub
 * @param data message data
 * @param size message size
 * @param ctx mpp context
 * @return 0 for success, else wise errcode
 */
int process_rvpu_message(const uint8_t *data, size_t size, MppCtx ctx);

#ifdef __cplusplus
}   /* extern C */
#endif

#endif /* __HAL_RVPU_API_H__ */