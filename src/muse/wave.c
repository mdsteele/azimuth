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

#include "azimuth/util/music.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "azimuth/util/misc.h"
#include "azimuth/util/music.h"
#include "azimuth/util/sound.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

static void write_int16(FILE *file, int16_t value) {
  fputc((value >> 0) & 0xff, file);
  fputc((value >> 8) & 0xff, file);
}

static void write_int32(FILE *file, int32_t value) {
  fputc((value >> 0) & 0xff, file);
  fputc((value >> 8) & 0xff, file);
  fputc((value >> 16) & 0xff, file);
  fputc((value >> 24) & 0xff, file);
}

void az_write_music_to_wav_file(FILE *file, az_music_synth_t *synth,
                                double duration) {
  const int num_samples = ceil(AZ_AUDIO_RATE * duration);
  const int bytes_per_sample = 2;
  const int bits_per_sample = 8 * bytes_per_sample;
  const int data_size = num_samples * bytes_per_sample;
  // Header:
  fputs("RIFF", file);
  write_int32(file, 36 * data_size);
  fputs("WAVE", file);
  // Format:
  fputs("fmt ", file);
  write_int32(file, 16); // Subchunk size
  write_int16(file, 1); // Audio format (1 = PCM)
  write_int16(file, 1); // Num channels
  write_int32(file, AZ_AUDIO_RATE); // Sample rate
  write_int32(file, AZ_AUDIO_RATE * bytes_per_sample); // Byte rate
  write_int16(file, bytes_per_sample); // Block align
  write_int16(file, bits_per_sample); // Bits per sample
  // Data:
  fputs("data", file);
  write_int32(file, data_size);
  int16_t buffer[1024];
  int samples_remaining = num_samples;
  while (samples_remaining > 0) {
    int samples_written = az_imin(samples_remaining, AZ_ARRAY_SIZE(buffer));
    az_synthesize_music(synth, buffer, samples_written);
    for (int i = 0; i < samples_written; ++i) {
      write_int16(file, buffer[i]);
    }
    samples_remaining -= samples_written;
  }
}

/*===========================================================================*/
