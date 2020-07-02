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

#define MODULE_TAG "mpp_rvpu_prim"

#include <fcntl.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <string.h>
#include <unistd.h>

#include "mpp_device.h"
#include "mpp_common.h"
#include "mpp_mem.h"
#include "mpp_log.h"

#include "rvpu_primitive.h"

// anbox - multiplex with audio socket
#define DEF_RVPU_SOCKNAME       "/dev/anbox_audio"

#define CLIENT_TYPE_RVPU        0x02

#ifndef min
#define min(a,b)                (((a)<(b)) ? (a):(b))
#endif

const char *def_sockname = DEF_RVPU_SOCKNAME;

/**
 * RVPU channel client info
 */
typedef struct _client_info {
    unsigned char type;         /**< 0:playback 1:recording 2:screenshot(*) */
} ClientInfo;

void set_rvpu_sockname(const char *sname)
{
    def_sockname = sname;
}

int connect_remote()
{
    struct sockaddr_un addr;
    const char *srvname = def_sockname;
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
    strncpy(addr.sun_path, srvname, sizeof(addr.sun_path));
    len = offsetof(struct sockaddr_un, sun_path) + strlen(addr.sun_path);
    if (connect(fd, (struct sockaddr *)&addr, len) < 0) {
        close(fd);
        mpp_err("connect to '%s' with error: %s", addr.sun_path, strerror(errno));
        return MPP_ERR_OPEN_FILE;
    }

    if (fd > 0) {   // create a channel for streaming
        int nread = -1;
        ClientInfo cmd = { CLIENT_TYPE_RVPU }, rsp;
        mpp_log("connected to '%s'", addr.sun_path);
        if (write(fd, &cmd, sizeof(cmd)) > 0) {
            nread = read(fd, &rsp, sizeof(rsp));
        }
        if (nread < 0 || rsp.type != cmd.type) {
            mpp_err("invalid response type %d (%d)", rsp.type, sizeof(rsp));
        }
    }

    return fd;
}

void disconnect_remote(int fd)
{
    if (fd > 0) {
        send_remote(fd, RVPU_PRIM_DEINIT, NULL, 0);
        close(fd);
        fd = -1;
    }
}

MPP_RET send_remote(int fd, RVPU_PRIM_CODE prim, void *data, size_t datasiz)
{
    char buf[8 + MAX_CONTEXT_SIZE];
    size_t  len;
    ssize_t ret;
    
    if (fd <= 0)
        return MPP_NOK;

    if (datasiz > 0) {
        int n = sprintf(buf, "%d:%lu:", prim, datasiz);
        memcpy(&buf[n], data, datasiz);
        len = n + datasiz;
    } else {
        len = sprintf(buf, "%d:0:", prim);
    }

    ret = send(fd, buf, len, MSG_NOSIGNAL);
    return ret > 0 ? MPP_OK : MPP_NOK;
}

static size_t 
parse_token(const uint8_t *pch_begin, const uint8_t *pch_end, char *out, size_t outsiz)
{
    const uint8_t *pch;
    char *pout = out; 
    const char *outend = out + outsiz - 1;

    for (pch = pch_begin; pch < pch_end && pout < outend; pout++) {
        uint8_t ch = *pch++;
        if (ch == ':' || ch == '\0') {
            break;
        }
        *pout = ch;
    }
    *pout = '\0';
    
    // mpp_log("parse_token '%s' %d bytes", out, pch - pch_begin);
    return pch - pch_begin;
}

ssize_t parse_header(const uint8_t *data, const uint8_t *pend, RVPU_PRIM_CODE *prim, size_t *plen)
{
    const uint8_t *pch  = data;

    char t1[8], t2[8];
    pch += parse_token(pch, pend, &t1[0], sizeof(t1));
    pch += parse_token(pch, pend, &t2[0], sizeof(t2));
    mpp_assert(pch <= pend);
    *prim = (RVPU_PRIM_CODE)atoi(t1);
    *plen = atoi(t2);
    return pch - data;
}

ssize_t recv_remote(int fd, RVPU_PRIM_CODE *prim, void *pdata, size_t maxsize)
{
    uint8_t buf[8 + MAX_CONTEXT_SIZE];
    size_t  len;
    ssize_t ret;
    
    if (fd <= 0)
        return MPP_NOK;
    mpp_assert(prim != NULL);

    ret = recv(fd, buf, len, MSG_DONTWAIT);

    if (ret > 0) {
        const uint8_t *pch  = buf;
        const uint8_t *pend = buf + ret;
        pch += parse_header(pch, pend, prim, &len);
        if (len > 0 && pdata != NULL) {
            memcpy(pdata, pch, min(maxsize, len));
        }
        ret = len;
    }
    return ret;
}
