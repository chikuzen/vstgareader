/*
  tga.h

  Author Oka Motofumi

  Copyright (C) 2012,  Oka Motofumi <chikuzen.mo at gmail dot com>

  This file is part of vstgareader

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

#ifndef VS_TGA_H
#define VS_TGA_H

#include <stdint.h>

typedef enum {
    TGA_OK,
    TGA_ERROR,
    TGA_SEEK_FAIL,
    TGA_READ_FAIL,
    TGA_UNKNOWN_FORMAT,
    TGA_UNSUPPORTED_FORMAT,
    TGA_NO_IMAGE_DATA
} tga_retcode_t;

typedef struct {
    FILE *fd;
    int id_len;
    int img_t;
    int width;
    int height;
    int depth;
} tga_t;


tga_t *tga_open(const char *file_name);

tga_retcode_t tga_read_metadata(tga_t *tga);

tga_retcode_t tga_read_all_scanlines(tga_t *tga, uint8_t *buf);

const char *tga_get_error_string(tga_retcode_t code);

void tga_close(tga_t *tga);

#endif /* VS_TGA_H */
