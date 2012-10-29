/*
  tgareader.c: TARGA image reader for VapourSynth

  This file is a part of vstgareader

  Copyright (C) 2012  Oka Motofumi

  Author: Oka Motofumi (chikuzen.mo at gmail dot com)

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with Libav; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "tga.h"
#include "VapourSynth.h"

#define VS_TGAR_VERSION "0.1.0"

#ifdef _MSC_VER
#pragma warning(disable:4996)
#define snprintf _snprintf
#endif

typedef struct {
    const char **src_files;
    unsigned char *read_buff;
    VSVideoInfo vi;
} tga_hnd_t;

typedef struct {
    const VSMap *in;
    VSMap *out;
    VSCore *core;
    const VSAPI *vsapi;
} vs_args_t;


static inline uint32_t VS_CC
bitor8to32(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3)
{
    return ((uint32_t)b0 << 24) | ((uint32_t)b1 << 16) |
           ((uint32_t)b2 << 8) | (uint32_t)b3;
}


static void VS_CC
write_frame_bgr24(tga_hnd_t *th, VSFrameRef *dst, const VSAPI *vsapi)
{
    typedef struct {
        uint8_t c[12];
    } bgr24_t;

    uint8_t *srcp_orig = th->read_buff;
    int row_size = (th->vi.width + 3) >> 2;
    int height = th->vi.height;
    int src_stride = th->vi.width * 3;

    uint32_t *dstp_b = (uint32_t *)vsapi->getWritePtr(dst, 2);
    uint32_t *dstp_g = (uint32_t *)vsapi->getWritePtr(dst, 1);
    uint32_t *dstp_r = (uint32_t *)vsapi->getWritePtr(dst, 0);
    int dst_stride = vsapi->getStride(dst, 0) >> 2;

    for (int y = 0; y < height; y++) {
        bgr24_t *srcp = (bgr24_t *)(srcp_orig + y * src_stride);
        for (int x = 0; x < row_size; x++) {
            dstp_b[x] = bitor8to32(srcp[x].c[9], srcp[x].c[6],
                                   srcp[x].c[3], srcp[x].c[0]);
            dstp_g[x] = bitor8to32(srcp[x].c[10], srcp[x].c[7],
                                   srcp[x].c[4], srcp[x].c[1]);
            dstp_r[x] = bitor8to32(srcp[x].c[11], srcp[x].c[8],
                                   srcp[x].c[5], srcp[x].c[2]);
        }
        dstp_b += dst_stride;
        dstp_g += dst_stride;
        dstp_r += dst_stride;
    }
}


static void VS_CC
write_frame_bgra(tga_hnd_t *th, VSFrameRef *dst, const VSAPI *vsapi)
{
    typedef struct {
        uint8_t c[16];
    } bgra_t;

    uint8_t *srcp_orig = th->read_buff;
    int row_size = (th->vi.width + 3) >> 2;
    int height = th->vi.height;
    int src_stride = th->vi.width * 4;

    uint32_t *dstp_b = (uint32_t *)vsapi->getWritePtr(dst, 2);
    uint32_t *dstp_g = (uint32_t *)vsapi->getWritePtr(dst, 1);
    uint32_t *dstp_r = (uint32_t *)vsapi->getWritePtr(dst, 0);
    int dst_stride = vsapi->getStride(dst, 0) >> 2;

    for (int y = 0; y < height; y++) {
        bgra_t *srcp = (bgra_t *)(srcp_orig + y * src_stride);
        for (int x = 0; x < row_size; x++) {
            dstp_b[x] = bitor8to32(srcp[x].c[12], srcp[x].c[8],
                                   srcp[x].c[4],  srcp[x].c[0]);
            dstp_g[x] = bitor8to32(srcp[x].c[13], srcp[x].c[9],
                                   srcp[x].c[5],  srcp[x].c[1]);
            dstp_r[x] = bitor8to32(srcp[x].c[14], srcp[x].c[10],
                                   srcp[x].c[6],  srcp[x].c[2]);
        }
        dstp_b += dst_stride;
        dstp_g += dst_stride;
        dstp_r += dst_stride;
    }
}


static const VSFrameRef * VS_CC
tga_get_frame(int n, int activation_reason, void **instance_data,
              void **frame_data, VSFrameContext *frame_ctx, VSCore *core,
              const VSAPI *vsapi)
{
    if (activation_reason != arInitial) {
        return NULL;
    }

    tga_hnd_t *th = (tga_hnd_t *)*instance_data;

    int frame_number = n;
    if (n >= th->vi.numFrames) {
        frame_number = th->vi.numFrames - 1;
    }

    tga_t *tga = tga_open(th->src_files[frame_number]);
    if (!tga ||
        tga_read_metadata(tga) != TGA_OK ||
        tga_read_all_scanlines(tga, th->read_buff) != TGA_OK) {
        return NULL;
    }
    int mode = tga->depth;
    tga_close(tga);

    VSFrameRef *dst = vsapi->newVideoFrame(th->vi.format, th->vi.width,
                                           th->vi.height, NULL, core);

    VSMap *props = vsapi->getFramePropsRW(dst);
    vsapi->propSetInt(props, "_DurationNum", th->vi.fpsDen, 0);
    vsapi->propSetInt(props, "_DurationDen", th->vi.fpsNum, 0);

    if (mode == 24) {
        write_frame_bgr24(th, dst, vsapi);
    } else {
        write_frame_bgra(th, dst, vsapi);
    }

    return dst;
}


static void close_handler(tga_hnd_t *th)
{
    if (!th) {
        return;
    }
    free(th->read_buff);
    free(th->src_files);
    free(th);
}


static void VS_CC
vs_init(VSMap *in, VSMap *out, void **instance_data, VSNode *node,
        VSCore *core, const VSAPI *vsapi)
{
    tga_hnd_t *th = (tga_hnd_t *)*instance_data;
    vsapi->setVideoInfo(&th->vi, node);
}


static void VS_CC
vs_close(void *instance_data, VSCore *core, const VSAPI *vsapi)
{
    tga_hnd_t *th = (tga_hnd_t *)instance_data;
    close_handler(th);
}


static const char * VS_CC
check_srcs(tga_hnd_t *th, int n)
{

    tga_t *tga = tga_open(th->src_files[n]);
    if (!tga) {
        return "failed to open file";
    }
    tga_retcode_t ret = tga_read_metadata(tga);
    if (ret != TGA_OK) {
        tga_close(tga);
        return tga_get_error_string(ret);
    }

    if (tga->width != th->vi.width || tga->height != th->vi.height) {
        if (n > 0) {
            return "found a file which has diffrent resolution from first file";
        }
        th->vi.width = tga->width;
        th->vi.height = tga->height;
    }

    return NULL;
}


static void VS_CC
set_args_int64(int64_t *p, int default_value, const char *arg, vs_args_t *va)
{
    int err;
    *p = va->vsapi->propGetInt(va->in, arg, 0, &err);
    if (err) {
        *p = default_value;
    }
}


#define RET_IF_ERR(cond, ...) \
{\
    if (cond) {\
        close_handler(th);\
        snprintf(msg, 240, __VA_ARGS__);\
        vsapi->setError(out, msg_buff);\
        return;\
    }\
}

static void VS_CC
create_reader(const VSMap *in, VSMap *out, void *user_data, VSCore *core,
              const VSAPI *vsapi)
{
    char msg_buff[256] = "tgar: ";
    char *msg = msg_buff + strlen(msg_buff);
    vs_args_t va = {in, out, core, vsapi};

    tga_hnd_t *th = (tga_hnd_t *)calloc(sizeof(tga_hnd_t), 1);
    RET_IF_ERR(!th, "failed to create handler");

    int num_srcs = vsapi->propNumElements(in, "files");
    RET_IF_ERR(num_srcs < 1, "no source file");
    th->vi.numFrames = num_srcs;

    th->src_files = (const char **)calloc(sizeof(char *), num_srcs);
    RET_IF_ERR(!th->src_files, "failed to allocate array of src files");

    int err;
    for (int i = 0; i < num_srcs; i++) {
        th->src_files[i] = vsapi->propGetData(in, "files", i, &err);
        RET_IF_ERR(err || strlen(th->src_files[i]) == 0,
                   "zero length file name was found");
    }

    for (int i = 0; i < num_srcs; i++) {
        const char *cs = check_srcs(th, i);
        RET_IF_ERR(cs, "file %d: %s", i, cs);
    }

    th->vi.format = vsapi->getFormatPreset(pfRGB24, core);
    set_args_int64(&th->vi.fpsNum, 24, "fpsnum", &va);
    set_args_int64(&th->vi.fpsDen, 1, "fpsden", &va);

    th->read_buff = (uint8_t *)malloc(th->vi.width * th->vi.height * 4 + 32);
    RET_IF_ERR(!th->read_buff, "failed to allocate read buffer");

    const VSNodeRef *node =
        vsapi->createFilter(in, out, "Read", vs_init, tga_get_frame, vs_close,
                            fmSerial, 0, th, core);

    vsapi->propSetNode(out, "clip", node, 0);
}


VS_EXTERNAL_API(void) VapourSynthPluginInit(
    VSConfigPlugin f_config, VSRegisterFunction f_register, VSPlugin *plugin)
{
    f_config("chikuzen.does.not.have.his.own.domain.tgar", "tgar",
             "TARGA image reader for VapourSynth " VS_TGAR_VERSION,
             VAPOURSYNTH_API_VERSION, 1, plugin);
    f_register("Read", "files:data[];fpsnum:int:opt;fpsden:int:opt",
               create_reader, NULL, plugin);
}
