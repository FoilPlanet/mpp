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

#ifndef __HAL_RVPU_PRIMITIVE_H__
#define __HAL_RVPU_PRIMITIVE_H__

#include "mpp_hal.h"

#ifndef MAX_CONTEXT_SIZE
// #define MAX_CONTEXT_SIZE        sizeof(H264eHalContext)
#define MAX_CONTEXT_SIZE            512
#endif

#ifdef __cplusplus
extern "C" {
#endif

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


/**
 * Context for encoder/decoder, keep in HalContext::extra_info
 */
typedef struct hal_rvpu_info_s {
    int remote_fd;
} HalRvpuInfo;

/**
 * Connect to remote vpu instance
 * @return remote fd
 */
int connect_remote();

/**
 * Disconnect to remote vpu instance
 * @param fd returned by connect_remote
 * @return None
 */
void disconnect_remote(int fd);

/**
 * Send buffer or data to remote
 * @param fd returned by connect_remote
 * @param prim rvpu primitive
 * @param data data to sent
 * @param datasiz data size
 * @return MPP_OK on success
 */
MPP_RET send_remote(int fd, RVPU_PRIM_CODE prim, void *data, size_t datasiz);

/**
 * Receive buffer or data from remote
 * @param fd returned by connect_remote
 * @param prim OUT recved rvpu primitive
 * @param data OUT data recved
 * @param maxsiz max data size
 * @return data size, or -ERRNO
 */
ssize_t recv_remote(int fd, RVPU_PRIM_CODE *prim, void *data, size_t maxsiz);

ssize_t parse_header(const uint8_t *data, const uint8_t *pend, RVPU_PRIM_CODE *prim, size_t *plen);

#ifdef __cplusplus
}
#endif

#endif /* __HAL_RVPU_PRIMITIVE_H__ */