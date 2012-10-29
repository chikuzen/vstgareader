/*
  tga.c

  original author Matthias Brueckner (libtga)
  modified by Oka Motofumi

  Copyright (C) 2001-2002  Matthias Brueckner <matbrc@gmx.de>
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "tga.h"

#define TGA_HEADER_SIZE 18


static inline int get_image_data_offset(tga_t *tga)
{
    return TGA_HEADER_SIZE + tga->id_len;
}

static inline uint32_t get_scanline_size(tga_t *tga)
{
    return tga->width * tga->depth >> 3;
}

static inline int is_encoded_data(tga_t *tga)
{
    return tga->img_t == 10;
}

static inline int bitor_int(uint8_t a, uint8_t b)
{
    return ((int)a << 8) | ((int)b);
}


static tga_retcode_t tga_read_rle(tga_t *tga, uint8_t *buf)
{
    if (!tga || !buf) {
        return TGA_ERROR;
    }

    int repeat = 0;
    int direct = 0;
    int width = tga->width;
    int bytes = tga->depth >> 3;
    char sample[4];
    FILE *fd = tga->fd;

    for (int x = 0; x < width; x++) {
        if (repeat == 0 && direct == 0) {
            int head = getc(fd);
            if (head == EOF) {
                return TGA_ERROR;
            }
            if (head >= 128) {
                repeat = head - 127;
                if (fread(sample, bytes, 1, fd) < 1) {
                    return TGA_ERROR;
                }
            } else {
                direct = head + 1;
            }
        }
        if (repeat > 0) {
            for (int k = 0; k < bytes; k++) {
                buf[k] = sample[k];
            }
            repeat--;
        } else {
            if (fread(buf, bytes, 1, fd) < 1) {
                return TGA_ERROR;
            }
            --direct;
        }
        buf += bytes;
    }

    return TGA_OK;
}


tga_t *tga_open(const char *file)
{
    tga_t *tga = (tga_t *)calloc(sizeof(tga_t), 1);
    if (!tga) {
        return NULL;
    }

    tga->fd = fopen(file, "rb");
    if (!tga->fd) {
        free(tga);
        return NULL;
    }

    return tga;
}


void tga_close(tga_t *tga)
{
    if (tga) {
        fclose(tga->fd);
        free(tga);
    }
}


const char *tga_get_error_string(tga_retcode_t code)
{
    const struct {
        tga_retcode_t retcode;
        char *error_string;
    } table[] = {
        { TGA_OK,                 "Success"                },
        { TGA_ERROR,              "Error"                  },
        { TGA_SEEK_FAIL,          "Seek failed"            },
        { TGA_READ_FAIL,          "Read failed"            },
        { TGA_UNKNOWN_FORMAT,     "Unknown format"         },
        { TGA_UNSUPPORTED_FORMAT, "Unsupported format"     },
        { TGA_NO_IMAGE_DATA,      "File has no image data" },
        { code,                   "Unkown retcode"         }
    };

    int i = 0;
    while (table[i].retcode != code) i++;
    return table[i].error_string;
}


tga_retcode_t tga_read_metadata(tga_t *tga)
{
    if (!tga) {
        return TGA_ERROR;
    }

    if (fseek(tga->fd, 0, SEEK_SET) != 0) {
        return TGA_SEEK_FAIL;
    }

    uint8_t tmp[TGA_HEADER_SIZE] = { 0 };
    if (fread(tmp, TGA_HEADER_SIZE, 1, tga->fd) == 0) {
        return TGA_READ_FAIL;
    }

    if (tmp[1] != 0 && tmp[1] != 1) {
        return TGA_UNKNOWN_FORMAT;
    }

    if (tmp[2] == 0) {
        return TGA_NO_IMAGE_DATA;
    }

    if (tmp[2] != 1 && tmp[2] != 2 && tmp[2] != 3 &&
        tmp[2] != 9 && tmp[2] != 10 && tmp[2] != 11) {
        return TGA_UNKNOWN_FORMAT;
    }

    if (tmp[2] != 2 && tmp[2] != 10) {
        return TGA_UNSUPPORTED_FORMAT;
    }

    if (tmp[16] != 24 && tmp[16] != 32) {
        return TGA_UNSUPPORTED_FORMAT;
    }

    tga->id_len     = tmp[0];
    tga->img_t      = tmp[2];
    tga->width      = bitor_int(tmp[13], tmp[12]);
    tga->height     = bitor_int(tmp[15], tmp[14]);
    tga->depth      = tmp[16];

    return TGA_OK;
}


tga_retcode_t tga_read_all_scanlines(tga_t *tga, uint8_t *buf)
{
    if (!tga || !buf) {
        return TGA_ERROR;
    }

    if (fseek(tga->fd, get_image_data_offset(tga), SEEK_SET) != 0) {
        return TGA_SEEK_FAIL;
    }

    size_t sln_size = get_scanline_size(tga);
    size_t read;
    size_t lines = tga->height;
    if (is_encoded_data(tga)) {
        for (read = 0; read < lines; read++) {
            if (tga_read_rle(tga, buf + read * sln_size) != TGA_OK) {
                break;
            }
        }
    } else {
        read = fread(buf, sln_size, lines, tga->fd);
    }

    return read == lines ? TGA_OK : TGA_READ_FAIL;
}
