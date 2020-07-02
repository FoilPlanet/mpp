
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

#define MODULE_TAG "mpp_rvpu"

#include <fcntl.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include <string>
#include <thread>
#include <functional>

#include "rk_mpi.h"
#include "mpp_buffer.h"
#include "mpp_common.h"
#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpi_impl.h"
#include "mpp_info.h"

#include "hal/rvpu/rvpu_api.h"
#include "hal/rvpu/rvpu_primitive.h"
//#include "rk_venc_cmd.h"

#define MAX_MPP_TRIES           3

static rvpu_callback_t on_ctrl_prep_cb  = NULL;
static rvpu_callback_t on_packet_cb     = NULL;
static rvpu_callback_t on_flush_cb      = NULL;

static MppApi *mpp_api = nullptr;

static void stream_init(MppCtx ctx, void *cfg)
{
    MPP_RET ret;
    (void)cfg;
    MppCodingType type = MPP_VIDEO_CodingAVC;   // H.264/AVC
    MppCtxType ctxtype = MPP_CTX_ENC;

    if (MPP_OK != (ret = mpp_init(ctx, ctxtype, type))) {
        mpp_err("mpp_init failed ret %d\n", ret);
        return;
    }

    mpp_log("mpp_init succuss.");
}

static void stream_control_dec(MppCtx ctx, RVPU_PRIM_CODE prim, void *cfg)
{
    (void)ctx;
    (void)prim;
    (void)cfg;
    // TODO
}

static void stream_control_enc(MppCtx ctx, RVPU_PRIM_CODE prim, void *cfg)
{
    switch (prim) {
    case RVPU_PRIM_CONTROL_PREP: {
        MppEncPrepCfg *prep = (MppEncPrepCfg *)(cfg);
        mpp_log("preferences %dx%d format %d", prep->width, prep->height, prep->format);
        if (on_ctrl_prep_cb) {
            // m_width  = prep->width;
            // m_height = prep->height;
            // m_format = prep->format;
            // m_hor_stride = prep->hor_stride;
            // m_ver_stride = prep->ver_stride;
            on_ctrl_prep_cb(prep, sizeof(*prep));
        }
        (void)mpp_api->control(ctx, MPP_ENC_SET_PREP_CFG, prep);
        break;
    }
    case RVPU_PRIM_CONTROL_RC: {
        MppEncRcCfg *prc = (MppEncRcCfg *)(cfg);
        mpp_log("rc_mode %s, fps in/out %d/%d", 
             (prc->rc_mode == MPP_ENC_RC_MODE_VBR) ? "VBR" : "CBR", 
             prc->fps_in_num, prc->fps_out_num);
        (void)mpp_api->control(ctx, MPP_ENC_SET_RC_CFG, prc);
        break;
    }
    case RVPU_PRIM_CONTROL_CODEC: {
      #if 0
        MppEncH264Cfg *pcfg = (MppEncH264Cfg *)(cfg);
        mpp_log("H264 profile %d level %d, qp(init %d max %d min %d)", 
             pcfg->profile, pcfg->level, 
             pcfg->qp_init, pcfg->qp_max, pcfg->qp_min);
      #endif
        (void)mpp_api->control(ctx, MPP_ENC_SET_CODEC_CFG, cfg);
        break;
    }
    case RVPU_PRIM_CONTROL_SEI: {
        MppEncSeiMode *sei = (MppEncSeiMode *)(cfg);
        mpp_log("Sei mode %d", *sei);
        (void)mpp_api->control(ctx, MPP_ENC_SET_SEI_CFG, sei);
        break;
    }
    default:
        mpp_assert(0);
        break;
    } // (prim)
}

static void stream_control(MppCtx ctx, RVPU_PRIM_CODE prim, void *cfg)
{
    stream_control_enc(ctx, prim, cfg);
    // TODO: 
    //stream_control_dec(ctx, prim. cfg);
}

static void stream_reset(MppCtx ctx)
{
    MPP_RET ret;
    if (MPP_OK != (ret = mpp_api->reset(ctx))) {
        mpp_err("mpp reset: %d", ret);
    }
}

static void stream_flush(MppCtx ctx)
{
    (void)ctx;
    // NOTHING
}

static void stream_end(MppCtx ctx)
{
    (void)ctx;
    // NOTHING
}

static void encode_packet(MppCtx ctx, void *data, size_t size)
{
    MPP_RET   ret;
    MppBuffer frmbuf;
    MppFrame  frame  = NULL;

    MppBufferInfo *info = static_cast<MppBufferInfo *>(data);

    if (size != sizeof(*info)) {
        mpp_err("Mismatched type size %d", size);
        return;
    }

    if (MPP_OK != (ret = mpp_buffer_import(&frmbuf, info))) {
        mpp_err("mpp_buffer_import (type:%d size:%06X fd:%d hnd:%p ptr:%p) failed: %d", 
                info->type, info->size, info->fd, info->hnd, info->ptr, ret);
        return;
    }

    if (MPP_OK != (ret = mpp_frame_init(&frame))) {
        mpp_err("mpp_frame_init failed");
        mpp_buffer_put(frmbuf);
        return;
    }

#if 0
    // TODO
    mpp_frame_set_width(frame, m_width);
    mpp_frame_set_height(frame, m_height);
    mpp_frame_set_hor_stride(frame, m_hor_stride);
    mpp_frame_set_ver_stride(frame, m_ver_stride);
    mpp_frame_set_fmt(frame, static_cast<MppFrameFormat>(m_format));
    DEBUG("frame %dx%d", m_width, m_height);
#endif
    
    mpp_frame_set_buffer(frame, frmbuf);
    mpp_frame_set_eos(frame, (frmbuf == nullptr) ? 1 : 0);

    if (MPP_OK != (ret = mpp_api->encode_put_frame(ctx, frame))) {
        mpp_err("mpp encode_put_frame: %d", ret);
    }
    
    // mpp_frame_deinit(&frame);
    mpp_buffer_put(frmbuf);
}

static void wait_packet(MppCtx ctx, void *data, size_t size)
{
    MPP_RET   ret;
    MppBufferInfo *info = static_cast<MppBufferInfo *>(data);
    MppPacket packet = nullptr;

    if (size != sizeof(*info)) {
        mpp_err("Mismatched type size %d", size);
        return;
    }

    int n = 0;
    do {
        if (MPP_OK != (ret = mpp_api->encode_get_packet(ctx, &packet))) {
            mpp_err("mpp encode_get_packet: %d", ret);
            return;
        }

        if (nullptr == packet) {
            mpp_err("encode_get_packet timeout %d", ++n);
            std::this_thread::sleep_for(std::chrono::milliseconds(1)); // usleep(1000);
        }
    } while (nullptr == packet && n < MAX_MPP_TRIES);

    if (nullptr != packet) {
        size_t pkt_len = mpp_packet_get_length(packet);
        // send to peer via socket in callback
        if (on_packet_cb) {
            void  *pkt_ptr = mpp_packet_get_pos(packet);
            on_packet_cb(pkt_ptr, pkt_len);
        }

      #ifndef SEND_SOCKET
        // send to remote via ion session
        MppBuffer outbuf;
        MppBuffer pktbuf = mpp_packet_get_buffer(packet);
        if (pktbuf && MPP_OK == (ret = mpp_buffer_import(&outbuf, info))) {
            MppBufferInfo binfo;
            mpp_buffer_info_get(pktbuf, &binfo);
            if (binfo.type == MPP_BUFFER_TYPE_ION) {
                uint32_t *ptag  = (uint32_t *)mpp_buffer_get_ptr(outbuf);
                binfo.fd  = 0;      // requires ion_map from binfo.hdl
                binfo.ptr = NULL;   // required ion_import
                memcpy(ptag + 1, &binfo, sizeof(binfo));
                // mpp_buffer_inc_ref(pktbuf);
                // mpp_log("size %X (%X) => %X", *ptag, info->size, pkt_len);
                *ptag = pkt_len;   // ready-flag
            } else {
                // mpp_err("packet buffer type is %d", binfo.type);
            }
            mpp_buffer_put(outbuf);
        }
      #endif /* !SEND_SOCKET */

        // release packet
        mpp_packet_deinit(&packet);
    }
}

static int valid_cfg_size(MppCtx ctx, size_t len)
{
    (void)ctx;
    // if (len != sizeof(MppEncH264Cfg)) {
    return len;
}

int rvpu_target_init(void *self, MppCtx ctx)
{
    MpiImpl *p = (MpiImpl*)ctx;

    if (mpp_api == nullptr) {
         mpp_api = p->api;
    }

    mpp_log("Assigned context %p", ctx);

    return 0;
}

void set_rvpu_callback(MppCtx ctx,
    rvpu_callback_t on_ctrl_prep, 
    rvpu_callback_t on_packet,
    rvpu_callback_t on_flush)
{
    (void)ctx;
    on_ctrl_prep_cb = on_ctrl_prep;
    on_packet_cb    = on_packet;
    on_flush_cb     = on_flush;
}

int process_rvpu(int fd, MppCtx ctx)
{
    uint8_t buf[MAX_CONTEXT_SIZE];
    ssize_t ret = recv(fd, buf, sizeof(buf), MSG_NOSIGNAL);

    if (ret >= 0)
        return process_rvpu_message(buf, ret, ctx);
    return MPP_NOK;
}

int process_rvpu_message(const uint8_t *data, size_t size, MppCtx ctx)
{
    const uint8_t *pch  = data;
    const uint8_t *pend = data + size;

    // mpp_log("process_data %d bytes: %s", size, data);

    if (NULL == ctx) {
        return -1;
    }

    // if (size == 0) {
    //  stream_reset(ctx);
    //}

    while (pch < pend) {
        RVPU_PRIM_CODE prim;
        size_t len;
        pch += parse_header(pch, pend, &prim, &len);

        switch (prim) {
        case RVPU_PRIM_INIT:
            stream_init(ctx, (uint8_t *)(pch));
            break;
 
        case RVPU_PRIM_DEINIT:
            stream_end(ctx);
            break;

        case RVPU_PRIM_REGS:
            break;

        case RVPU_PRIM_START:
            encode_packet(ctx, (uint8_t *)(pch), len);
            break;

        case RVPU_PRIM_WAIT:
            wait_packet(ctx, (uint8_t *)(pch), len);
            break;

        case RVPU_PRIM_CONTROL_PREP:
            if (len != sizeof(MppEncPrepCfg)) {
                mpp_log("Invalid primitive %d: invalid param size %d", prim, len);
            } else {
                stream_control(ctx, RVPU_PRIM_CONTROL_PREP, (uint8_t *)(pch));
            }
            break;

        case RVPU_PRIM_CONTROL_RC:
            if (len != sizeof(MppEncRcCfg)) {
                mpp_log("Invalid primitive %d: invalid param size %d", prim, len);
            } else {
                stream_control(ctx, RVPU_PRIM_CONTROL_RC, (uint8_t *)(pch));
            }
            break;

        case RVPU_PRIM_CONTROL_CODEC:
            if (!valid_cfg_size(ctx, len)) {
                mpp_log("Invalid primitive %d: invalid param size %d", prim, len);
            } else {
                stream_control(ctx, RVPU_PRIM_CONTROL_CODEC, (uint8_t *)(pch));
            }
            break;

        case RVPU_PRIM_CONTROL_SEI:
            if (len != sizeof(MppEncSeiMode)) {
                mpp_log("Invalid primitive %d: invalid param size %d", prim, len);
            } else {
                stream_control(ctx, RVPU_PRIM_CONTROL_SEI, (uint8_t *)(pch));
            }
            break;

        case RVPU_PRIM_CONTROL:
            // TODO
            break;

        case RVPU_PRIM_RESET:
            stream_reset(ctx);
            break;

        case RVPU_PRIM_FLUSH:
            stream_flush(ctx);
            break;

        default:
            mpp_log("Unknown primitive '%d' len %d: %s", prim, len, data);
            break;
        }
        
        pch += len;
    } // while (pch < pend)

    if (pch > pend) {
        mpp_log("Recv bytes %d, parsed %d", size, pend - pch);
    }

    return (int)MPP_OK;
}