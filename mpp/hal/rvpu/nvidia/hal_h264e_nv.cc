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

extern "C" {
#include "mpp_device.h"
#include "mpp_common.h"
#include "mpp_mem.h"
#include "h264_syntax.h"
#include "hal_h264e_nv.h"
}

#include <iostream>
#include <memory>
#include <cuda.h>
#include "NvEncoderCuda.h"
#include "NvEncoderOutputInVidMemCuda.h"

/**
 * Context for nvidia encoder, keep in H264eHalContext::extra_info
 */
typedef struct hal_nvenc_info_s {
    CUdevice        cu_device;
    CUcontext       cu_context;
    NvEncoder      *encoder;
    CUstream        in_stream;      /**< input stream for pre processing CUDA tasks   */
    CUstream        out_stream;     /**< output stream for post processing CUDA tasks */
} HalNvEncInfo;

static MPP_RET reinit_nvenc(MppEncCfgSet *cfg, HalNvEncInfo *info)
{
    int max_cached_frames;
    int width  = cfg->prep.width;
    int height = cfg->prep.height;
    NV_ENC_BUFFER_FORMAT format = NV_ENC_BUFFER_FORMAT_IYUV; // I420

    mpp_log("init nvenc %dx%d %d fps", width, height, cfg->rc.fps_in_num);

#ifdef _VPU_STUB_

#else /* !_VPU_STUB_ */
    if (NULL != info->encoder) {
        info->encoder->DestroyEncoder();
        delete info->encoder;
    }

    NvEncoderOutputInVidMemCuda *encoder = new NvEncoderOutputInVidMemCuda(info->cu_context, width, height, format);
    if (nullptr == encoder) {
        mpp_err("NvEncoderCuda create failed");
        return MPP_ERR_VPU_CODEC_INIT;
    }

    // info->stream = new NvCUStream(info->cu_context, 1, encoder);
    CUDA_DRVAPI_CALL(cuCtxPushCurrent(info->cu_context));
    if (CUDA_SUCCESS != cuStreamCreate(&info->in_stream, CU_STREAM_DEFAULT)) {
        mpp_err("cuStreamCreate");
    }
    // (void)cuStreamCreate(&info->out_stream, CU_STREAM_DEFAULT);
    info->out_stream = info->in_stream;

    CUDA_DRVAPI_CALL(cuCtxPopCurrent(NULL));

    // Set input and output CUDA streams in driver
    encoder->SetIOCudaStreams(&info->in_stream, &info->out_stream);

    // info->cu_crc = new NvCUCrc(info->cu_device, encoder->GetOutputBufferSize());

    NV_ENC_INITIALIZE_PARAMS param = { 0 };
    NV_ENC_CONFIG encodeConfig = { NV_ENC_CONFIG_VER };
    param.version      = NV_ENC_INITIALIZE_PARAMS_VER;
    param.encodeConfig = &encodeConfig;
    GUID encode_guid = NV_ENC_CODEC_H264_GUID;
    GUID preset_guid = NV_ENC_PRESET_DEFAULT_GUID;
    encoder->CreateDefaultEncoderParams(&param, encode_guid, preset_guid);
    encoder->CreateEncoder(&param);

    info->encoder = encoder;

    int frame_size = encoder->GetFrameSize();
    mpp_log("frame size %d", frame_size);

#endif /* _VPU_STUB_ */

    return MPP_OK;
}

MPP_RET hal_h264e_nv_init(void *hal, MppHalCfg *cfg)
{
    H264eHalContext *ctx = (H264eHalContext *)hal;
    HalNvEncInfo *info;
    MppEncCfgSet *ecfg = ctx->cfg;
    CUresult ret;

    mpp_log("init %p", ctx);

#ifdef _VPU_STUB_

#else /* !_VPU_STUB_ */

    ctx->extra_info = mpp_calloc(HalNvEncInfo, 1);
    if (NULL == (info = (HalNvEncInfo *)ctx->extra_info)) {
        return MPP_ERR_NOMEM;
    }

    if (CUDA_SUCCESS != (ret = cuInit(0))) {
        mpp_err("cuInit: %d", ret);
        return MPP_ERR_INIT;
    }

    int n_gpu = 0;
    ret = cuDeviceGetCount(&n_gpu);
    if (CUDA_SUCCESS != ret || n_gpu <= 0) {
        mpp_err("cuDeviceGetCount: %d - ngpu: %d", ret, n_gpu);
        return MPP_ERR_INIT;
    }

    int i_gpu = 0;
    info->cu_device = 0;
    cuDeviceGet(&info->cu_device, i_gpu);
    
    char devname[80];
    cuDeviceGetName(devname, sizeof(devname), info->cu_device);
    mpp_log("GPU in use: [%d] %s", i_gpu, devname);

    cuCtxCreate(&info->cu_context, 0, info->cu_device);

#endif /* _VPU_STUB_ */

    return MPP_OK;
}

MPP_RET hal_h264e_nv_deinit(void *hal)
{
    H264eHalContext *ctx = (H264eHalContext *)hal;

    mpp_log("deinit %p", ctx);

#ifdef _VPU_STUB_

#else /* !_VPU_STUB_ */

    if (ctx->extra_info) {
        HalNvEncInfo *info = (HalNvEncInfo *)ctx->extra_info;

        if (CUDA_SUCCESS != cuCtxPushCurrent(info->cu_context)) {
            mpp_err("cuCtxPushCurrent");
        }

        if (info->in_stream != NULL) {
            cuStreamDestroy(info->in_stream);
        }

        if (info->out_stream != NULL) {
            cuStreamDestroy(info->out_stream);
        }

        (void)cuCtxPopCurrent(NULL);

        if (info->encoder) {
            info->encoder->DestroyEncoder();
            delete info->encoder;
        }
        cuCtxDestroy(info->cu_context);
        MPP_FREE(ctx->extra_info);
    }

#endif /* _VPU_STUB_ */

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
    HalNvEncInfo *info = (HalNvEncInfo *)ctx->extra_info;

#ifdef _VPU_STUB_
    // in container, send task to queue

#else /* !_VPU_STUB_ */
    // in host, send task to nvenc

    MppBuffer   input  = task->enc.input;
  //MppBuffer   output = task->enc.output;

    mpp_log("start v:%u\n", task->enc.valid);

    NvEncoderCuda *encoder = static_cast<NvEncoderCuda *>(info->encoder);
    const NvEncInputFrame *input_frame = encoder->GetNextInputFrame();

    NvEncoderCuda::CopyToDeviceFrame(
        info->cu_context,
        mpp_buffer_get_ptr(input),
        0, 
        reinterpret_cast<CUdeviceptr>(input_frame->inputPtr),
        (int)input_frame->pitch,
        encoder->GetEncodeWidth(),
        encoder->GetEncodeHeight(),
        CU_MEMORYTYPE_HOST, 
        input_frame->bufferFormat,
        input_frame->chromaOffsets,
        input_frame->numChromaPlanes,
        false,
        info->in_stream
    );

#endif /* _VPU_STUB_ */

    return MPP_OK;
}

MPP_RET hal_h264e_nv_wait(void *hal, HalTaskInfo *task)
{
    H264eHalContext *ctx = (H264eHalContext *)hal;
    HalNvEncInfo *info = (HalNvEncInfo *)ctx->extra_info;

#ifdef _VPU_STUB_
    // in container, wait result from queue
    size_t offset = sizeof(NV_ENC_ENCODE_OUT_PARAMS);

#else /* !_VPU_STUB_ */

    // in host, wait nvenc response
    MppBuffer   output = task->enc.output;

    // mpp_log("wait w:%u v:%u\n", ctx->cfg->prep.hor_stride, task->enc.valid);
    void *pbuf = mpp_buffer_get_ptr(output);

    std::vector<NV_ENC_OUTPUT_PTR> packets;
    NvEncoderOutputInVidMemCuda *encoder = static_cast<NvEncoderOutputInVidMemCuda *>(info->encoder);
    encoder->EncodeFrame(packets);

    uint32_t bsize = 0;
    if (packets.size() > 0) {
        // only use first frame
        NV_ENC_OUTPUT_PTR ptr = packets[0];
        cuMemcpyDtoH(pbuf, (CUdeviceptr)ptr, encoder->GetOutputBufferSize());
        NV_ENC_ENCODE_OUT_PARAMS *eparm = reinterpret_cast<NV_ENC_ENCODE_OUT_PARAMS *>(pbuf);
        bsize = eparm->bitstreamSizeInBytes;
    }

    if (bsize > 0) {
        IOInterruptCB  *int_cb;
        h264e_feedback *feedback = &ctx->feedback;
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

#endif /* _VPU_STUB_ */

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
    HalNvEncInfo *info = reinterpret_cast<HalNvEncInfo *>(ctx->extra_info);
    
    (void)cmd;
    (void)arg;

    switch (cmd) {
    case MPP_ENC_SET_PREP_CFG:
        if (info) {
            MppEncCfgSet *ecfg = ctx->set;
            // x264_param_t *param = &info->param;
            // param->i_width  = ecfg->prep.width;
            // param->i_height = ecfg->prep.height;
        }
        break;
    
    case MPP_ENC_SET_RC_CFG:
        if (info) {
            MppEncCfgSet *ecfg = ctx->set;
            // x264_param_t *param = &info->param;
            // param->i_fps_den = ecfg->rc.fps_in_denorm;
            // param->i_fps_num = ecfg->rc.fps_in_num;
        }
        break;

    case MPP_ENC_SET_CODEC_CFG:
        if (info) {
            MppEncCfgSet *ecfg = ctx->set;

            // TODO
            (void)ecfg;
            // ecfg->codec.h264.qp_init
            // ecfg->codec.h264.profile
            // ecfg->codec.h264.level

            ret = reinit_nvenc(ecfg, info);
        }
        break;

    default:
        // had handled in hal_h264e_stub_control
        break;
    } // (cmd)

    return ret;
}
