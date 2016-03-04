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

#include "azimuth/util/rw.h"

#include <stdarg.h>
#include <string.h>

#include "azimuth/util/misc.h"

/*===========================================================================*/

void az_stream_reader(FILE *stream, az_reader_t *reader) {
  reader->type = AZ_RW_STREAM;
  reader->data.file = stream;
}

void az_stdin_reader(az_reader_t *reader) {
  az_stream_reader(stdin, reader);
}

bool az_file_reader(const char *path, az_reader_t *reader) {
  FILE *file = fopen(path, "r");
  if (file == NULL) {
    reader->type = AZ_RW_CLOSED;
    return false;
  }
  reader->type = AZ_RW_FILE;
  reader->data.file = file;
  return true;
}

void az_charbuf_reader(const char *buffer, size_t size, az_reader_t *reader) {
  FILE *file = tmpfile();
  if (file == NULL || fwrite(buffer, sizeof(char), size, file) < size) {
    AZ_FATAL("Failed to create charbuf reader.\n");
  }
  rewind(file);
  reader->type = AZ_RW_FILE;
  reader->data.file = file;
}

void az_cstring_reader(const char *str, az_reader_t *reader) {
  az_charbuf_reader(str, strlen(str), reader);
}

bool az_rgetpos(az_reader_t *reader, az_rw_pos_t *pos) {
  bool success = false;
  switch (reader->type) {
    case AZ_RW_CLOSED: break;
    case AZ_RW_STREAM:
    case AZ_RW_FILE:
      success = (fgetpos(reader->data.file, &pos->file_pos) == 0);
      break;
    case AZ_RW_STRING:
      pos->string_pos = reader->data.string.position;
      success = true;
      break;
  }
  return success;
}

bool az_rsetpos(az_reader_t *reader, const az_rw_pos_t *pos) {
  bool success = false;
  switch (reader->type) {
    case AZ_RW_CLOSED: break;
    case AZ_RW_STREAM:
    case AZ_RW_FILE:
      success = (fsetpos(reader->data.file, &pos->file_pos) == 0);
      break;
    case AZ_RW_STRING:
      if (pos->string_pos <= reader->data.string.size) {
        reader->data.string.position = pos->string_pos;
        success = true;
      }
      break;
  }
  return success;
}

int az_rgetc(az_reader_t *reader) {
  int ch = EOF;
  switch (reader->type) {
    case AZ_RW_CLOSED: break;
    case AZ_RW_STREAM:
    case AZ_RW_FILE:
      ch = fgetc(reader->data.file);
      break;
    case AZ_RW_STRING:
      if (reader->data.string.position < reader->data.string.size) {
        ch = reader->data.string.buffer[reader->data.string.position++];
      }
      break;
  }
  return ch;
}

int az_rscanf(az_reader_t *reader, const char *format, ...) {
  int result = -1;
  switch (reader->type) {
    case AZ_RW_CLOSED: break;
    case AZ_RW_STREAM:
    case AZ_RW_FILE: {
      va_list args;
      va_start(args, format);
      result = vfscanf(reader->data.file, format, args);
      va_end(args);
    } break;
    case AZ_RW_STRING: break;  // TODO: Not implemented.
  }
  return result;
}

void az_rclose(az_reader_t *reader) {
  switch (reader->type) {
    case AZ_RW_CLOSED: return;
    case AZ_RW_STREAM: break;
    case AZ_RW_FILE:
      fclose(reader->data.file);
      break;
    case AZ_RW_STRING: break;
  }
  reader->type = AZ_RW_CLOSED;
}

/*===========================================================================*/

void az_stream_writer(FILE *stream, az_writer_t *writer) {
  writer->type = AZ_RW_STREAM;
  writer->data.file = stream;
}

void az_stdout_writer(az_writer_t *writer) {
  az_stream_writer(stdout, writer);
}

void az_stderr_writer(az_writer_t *writer) {
  az_stream_writer(stderr, writer);
}

bool az_file_writer(const char *path, az_writer_t *writer) {
  FILE *file = fopen(path, "w");
  if (file == NULL) {
    writer->type = AZ_RW_CLOSED;
    return false;
  }
  writer->type = AZ_RW_FILE;
  writer->data.file = file;
  return true;
}

void az_charbuf_writer(char *buffer, size_t size, az_writer_t *writer) {
  writer->type = AZ_RW_STRING;
  writer->data.string.buffer = buffer;
  writer->data.string.size = size;
  writer->data.string.position = 0;
}

bool az_wprintf(az_writer_t *writer, const char *format, ...) {
  bool success = false;
  switch (writer->type) {
    case AZ_RW_CLOSED: break;
    case AZ_RW_STREAM:
    case AZ_RW_FILE: {
      va_list args;
      va_start(args, format);
      success = (0 <= vfprintf(writer->data.file, format, args));
      va_end(args);
    } break;
    case AZ_RW_STRING: {
      va_list args;
      va_start(args, format);
      const int result = vsnprintf(
          writer->data.string.buffer + writer->data.string.position,
          writer->data.string.size - writer->data.string.position,
          format, args);
      va_end(args);
      if (result >= 0 && writer->data.string.position + (size_t)result <
          writer->data.string.size) {
        writer->data.string.position += result;
        success = true;
      }
    } break;
  }
  return success;
}

void az_wclose(az_writer_t *writer) {
  switch (writer->type) {
    case AZ_RW_CLOSED: return;
    case AZ_RW_STREAM: break;
    case AZ_RW_FILE:
      fclose(writer->data.file);
      break;
    case AZ_RW_STRING:
      if (writer->data.string.position < writer->data.string.size) {
        writer->data.string.buffer[writer->data.string.position] = '\0';
      }
      break;
  }
  writer->type = AZ_RW_CLOSED;
}

/*===========================================================================*/
