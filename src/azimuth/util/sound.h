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
#ifndef AZIMUTH_UTIL_SOUND_H_
#define AZIMUTH_UTIL_SOUND_H_

#include <stddef.h>
#include <stdint.h>

/*===========================================================================*/

// Audio samples per second:
#define AZ_AUDIO_RATE 22050

typedef enum {
  AZ_NOISE_WAVE,
  AZ_SAWTOOTH_WAVE,
  AZ_SINE_WAVE,
  AZ_SQUARE_WAVE,
  AZ_TRIANGLE_WAVE,
  AZ_WOBBLE_WAVE
} az_sound_wave_kind_t;

typedef struct {
  az_sound_wave_kind_t wave_kind;
  float env_attack, env_sustain, env_punch, env_decay;
  float start_freq, freq_limit, freq_slide, freq_delta_slide;
  float vibrato_depth, vibrato_speed;
  float arp_mod, arp_speed;
  float square_duty, duty_sweep;
  float repeat_speed;
  float phaser_offset, phaser_sweep;
  float lpf_cutoff, lpf_ramp, lpf_resonance;
  float hpf_cutoff, hpf_ramp;
  float volume_adjust; // -1.0 to 1.0
} az_sound_spec_t;

typedef struct {
  size_t num_samples;
  int16_t *samples;
} az_sound_data_t;

void az_create_sound_data(const az_sound_spec_t *spec, az_sound_data_t *data);

void az_destroy_sound_data(az_sound_data_t *data);

/*===========================================================================*/

#endif // AZIMUTH_UTIL_SOUND_H_
