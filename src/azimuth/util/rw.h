/*=============================================================================
| Copyright 2012 Matthew D. Steele <mdsteele@alum.mit.edu>                    |
|                                                                             |
| This file is part of Azimuth.                                               |
|                                                                             |
| Azimuth is free software: you can redistribute it and/or modify it under    |
| the terms of the GNU General Public License as published by the Free        |
| Software Foundation, either version 3 of the License, or (at your option)   |
| any later version.                                                          |
|                                                                             |
| Azimuth is distributed in the hope that it will be useful, but WITHOUT      |
| ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       |
| FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for   |
| more details.                                                               |
|                                                                             |
| You should have received a copy of the GNU General Public License along     |
| with Azimuth.  If not, see <http://www.gnu.org/licenses/>.                  |
=============================================================================*/

#pragma once
#ifndef AZIMUTH_UTIL_RW_H_
#define AZIMUTH_UTIL_RW_H_

#include <stdbool.h>
#include <stdio.h>

/*===========================================================================*/

typedef enum {
  AZ_RW_CLOSED = 0,
  AZ_RW_STREAM, // a file that should not be closed
  AZ_RW_FILE,   // a file that should be closed
  AZ_RW_STRING, // a char array
} az_rw_type_t;

typedef struct {
  az_rw_type_t type;
  union {
    FILE *file;
    struct {
      const char *buffer;
      size_t size;
      size_t position;
    } string;
  } data;
} az_reader_t;

typedef struct {
  az_rw_type_t type;
  union {
    FILE *file;
    struct {
      char *buffer;
      size_t size;
      size_t position;
    } string;
  } data;
} az_writer_t;

typedef union {
  fpos_t file_pos;
  size_t string_pos;
} az_rw_pos_t;

/*===========================================================================*/

// Construtors:
void az_stream_reader(FILE *stream, az_reader_t *reader);
void az_stdin_reader(az_reader_t *reader);
bool az_file_reader(const char *path, az_reader_t *reader);
void az_charbuf_reader(const char *buffer, size_t size, az_reader_t *reader);
void az_cstring_reader(const char *str, az_reader_t *reader);

// Get/set position:
bool az_rgetpos(az_reader_t *reader, az_rw_pos_t *pos);
bool az_rsetpos(az_reader_t *reader, const az_rw_pos_t *pos);

// Read:
int az_rgetc(az_reader_t *reader);
int az_rscanf(az_reader_t *reader, const char *format, ...)
  __attribute__((__format__(__scanf__,2,3)));

// Close:
void az_rclose(az_reader_t *reader);

/*===========================================================================*/

// Construtors:
void az_stream_writer(FILE *stream, az_writer_t *writer);
void az_stdout_writer(az_writer_t *writer);
void az_stderr_writer(az_writer_t *writer);
bool az_file_writer(const char *path, az_writer_t *writer);
void az_charbuf_writer(char *buffer, size_t size, az_writer_t *writer);

// Write:
bool az_wprintf(az_writer_t *writer, const char *format, ...)
  __attribute__((__format__(__printf__,2,3)));

// Close:
void az_wclose(az_writer_t *writer);

/*===========================================================================*/

#endif // AZIMUTH_UTIL_RW_H_
