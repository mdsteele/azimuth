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

#include "azimuth/util/sound.h"

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h"

#pragma GCC diagnostic error "-Wconversion"

/*===========================================================================*/
// Synthesizer:

// This section of the file contains a modified version of the sound generation
// code from sfxr (http://www.drpetter.se/project_sfxr.html), written by Tomas
// Pettersson ("DrPetter").  sfxr is made available under the MIT/Expat
// license, reproduced here:

  /*******************************************************************\
  * sfxr                                                              *
  * Copyright (c) 2007 Tomas Pettersson                               *
  *                                                                   *
  * Permission is hereby granted, free of charge, to any person       *
  * obtaining a copy of this software and associated documentation    *
  * files (the "Software"), to deal in the Software without           *
  * restriction, including without limitation the rights to use,      *
  * copy, modify, merge, publish, distribute, sublicense, and/or sell *
  * copies of the Software, and to permit persons to whom the         *
  * Software is furnished to do so, subject to the following          *
  * conditions:                                                       *
  *                                                                   *
  * The above copyright notice and this permission notice shall be    *
  * included in all copies or substantial portions of the Software.   *
  *                                                                   *
  * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,   *
  * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES   *
  * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND          *
  * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT       *
  * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,      *
  * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING      *
  * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR     *
  * OTHER DEALINGS IN THE SOFTWARE.                                   *
  \*******************************************************************/

// Many thanks to DrPetter for developing sfxr, and for releasing it as Free
// Software.

static struct {
  int phase;
  double fperiod, fmaxperiod, fslide, fdslide;
  int period;
  float square_duty, square_slide;
  int env_stage;
  int env_time;
  int env_length[3];
  float env_vol;
  float fphase, fdphase;
  int iphase;
  float phaser_buffer[1024];
  int ipp;
  float noise_buffer[32];
  float fltp, fltdp, fltw, fltw_d;
  float fltdmp, fltphp, flthp, flthp_d;
  float vib_phase, vib_speed, vib_amp;
  int rep_time, rep_limit;
  int arp_time, arp_limit;
  double arp_mod;
  size_t num_samples;
  int16_t samples[128 * 1024];
} synth;

// Fill each entry in synth.noise_buffer with a random float from -1 to 1.
static void refill_noise_buffer(void) {
  // Xorshift RNG (see http://en.wikipedia.org/wiki/Xorshift)
  static uint32_t x = 123456789;
  static uint32_t y = 362436069;
  static uint32_t z = 521288629;
  static uint32_t w = 88675123;
  for (int i = 0; i < 32; ++i) {
    const uint32_t t = x ^ (x << 11);
    x = y; y = z; z = w;
    w = w ^ (w >> 19) ^ t ^ (t >> 8);
    synth.noise_buffer[i] = (float)((w * 4.656612874161595e-10) - 1.0);
  }
}

// Reset the sfxr synth.  This code is taken directly from sfxr, with only
// minor changes.
static void reset_synth(const az_sound_spec_t *spec, bool restart) {
  if (!restart) synth.phase = 0;
  synth.fperiod = 100.0 / (spec->start_freq * spec->start_freq + 0.001);
  synth.period = (int)synth.fperiod;
  synth.fmaxperiod = 100.0 / (spec->freq_limit * spec->freq_limit + 0.001);
  synth.fslide = 1.0 - pow(spec->freq_slide, 3) * 0.01;
  synth.fdslide = -pow(spec->freq_delta_slide, 3) * 0.000001;
  synth.square_duty = 0.5f - spec->square_duty * 0.5f;
  synth.square_slide = -spec->duty_sweep * 0.00005f;
  if (spec->arp_mod >= 0.0f) {
    synth.arp_mod = 1.0 - pow(spec->arp_mod, 2) * 0.9;
  } else {
    synth.arp_mod = 1.0 + pow(spec->arp_mod, 2) * 10.0;
  }
  synth.arp_time = 0;
  synth.arp_limit = (int)(pow(1.0 - spec->arp_speed, 2) * 20000 + 32);
  if (spec->arp_speed == 1.0f) synth.arp_limit = 0;
  if (!restart) {
    // Reset filter:
    synth.fltp = 0.0f;
    synth.fltdp = 0.0f;
    synth.fltw = powf(1.0f - spec->lpf_cutoff, 3) * 0.1f;
    synth.fltw_d = 1.0f + spec->lpf_ramp * 0.0001f;
    synth.fltdmp = 5.0f / (1.0f + powf(spec->lpf_resonance, 2) * 20.0f) *
      (0.01f + synth.fltw);
    if (synth.fltdmp > 0.8f) synth.fltdmp = 0.8f;
    synth.fltphp = 0.0f;
    synth.flthp = powf(spec->hpf_cutoff, 2) * 0.1f;
    synth.flthp_d = 1.0f + spec->hpf_ramp * 0.0003f;
    // Reset vibrato:
    synth.vib_phase = 0.0f;
    synth.vib_speed = powf(spec->vibrato_speed, 2) * 0.01f;
    synth.vib_amp = spec->vibrato_depth * 0.5f;
    // Reset envelope:
    synth.env_vol = 0.0f;
    synth.env_stage = 0;
    synth.env_time = 0;
    synth.env_length[0] =
      (int)(spec->env_attack * spec->env_attack * 100000.0f);
    synth.env_length[1] =
      (int)(spec->env_sustain * spec->env_sustain * 100000.0f);
    synth.env_length[2] =
      (int)(spec->env_decay * spec->env_decay * 100000.0f);
    // Reset phaser:
    synth.fphase = powf(spec->phaser_offset, 2) * 1020.0f;
    if (spec->phaser_offset < 0.0f) synth.fphase = -synth.fphase;
    synth.fdphase = powf(spec->phaser_sweep, 2);
    if (spec->phaser_sweep < 0.0f) synth.fdphase = -synth.fdphase;
    synth.iphase = abs((int)synth.fphase);
    synth.ipp = 0;
    AZ_ZERO_ARRAY(synth.phaser_buffer);
    // Refill noise buffer:
    refill_noise_buffer();
    // Reset repeat:
    synth.rep_time = 0;
    synth.rep_limit =
      (int)(powf(1.0f - spec->repeat_speed, 2) * 20000 + 32);
    if (spec->repeat_speed == 0.0f) synth.rep_limit = 0;
  }
}

// Generate the given sound effect and populate the synth.samples array.
// This code is taken directly from sfxr, with only minor changes.
static void synth_sound(const az_sound_spec_t *spec) {
  synth.num_samples = 0;
  reset_synth(spec, false);
  float filesample = 0.0f;
  int fileacc = 0;
  bool finished = false;

  while (synth.num_samples < AZ_ARRAY_SIZE(synth.samples) && !finished) {
    ++synth.rep_time;
    if (synth.rep_limit != 0 && synth.rep_time >= synth.rep_limit) {
      synth.rep_time = 0;
      reset_synth(spec, true);
    }

    // frequency envelopes/arpeggios
    ++synth.arp_time;
    if (synth.arp_limit != 0 && synth.arp_time >= synth.arp_limit) {
      synth.arp_limit = 0;
      synth.fperiod *= synth.arp_mod;
    }
    synth.fslide += synth.fdslide;
    synth.fperiod *= synth.fslide;
    if (synth.fperiod > synth.fmaxperiod) {
      synth.fperiod = synth.fmaxperiod;
      if (spec->freq_limit > 0.0f) finished = true;
    }
    float rfperiod = (float)synth.fperiod;
    if (synth.vib_amp > 0.0f) {
      synth.vib_phase += synth.vib_speed;
      rfperiod =
        (float)(synth.fperiod * (1.0 + sin(synth.vib_phase) * synth.vib_amp));
    }
    synth.period = (int)rfperiod;
    if (synth.period < 8) synth.period = 8;
    synth.square_duty += synth.square_slide;
    if (synth.square_duty < 0.0f) synth.square_duty = 0.0f;
    if (synth.square_duty > 0.5f) synth.square_duty = 0.5f;
    // volume envelope
    synth.env_time++;
    if (synth.env_time > synth.env_length[synth.env_stage]) {
      synth.env_time = 0;
      ++synth.env_stage;
      if (synth.env_stage == 3) finished = true;
    }
    if (synth.env_stage == 0) {
      assert(synth.env_length[0] > 0);
      synth.env_vol = (float)synth.env_time / synth.env_length[0];
    }
    if (synth.env_stage == 1) {
      synth.env_vol = 1.0f;
      if (synth.env_length[1] > 0) {
        synth.env_vol +=
          powf(1.0f - (float)synth.env_time / synth.env_length[1], 1.0f) *
          2.0f * spec->env_punch;
      }
    }
    if (synth.env_stage == 2) {
      synth.env_vol = (synth.env_length[2] > 0 ?
                       1.0f - (float)synth.env_time / synth.env_length[2] :
                       1.0f);
    }

    // phaser step
    synth.fphase += synth.fdphase;
    synth.iphase = abs((int)synth.fphase);
    if (synth.iphase > 1023) synth.iphase = 1023;

    if (synth.flthp_d != 0.0f) {
      synth.flthp *= synth.flthp_d;
      if (synth.flthp < 0.00001f) synth.flthp=0.00001f;
      if (synth.flthp > 0.1f) synth.flthp=0.1f;
    }

    float ssample = 0.0f;
    for (int si = 0; si < 8; ++si) { // 8x supersampling
      float sample = 0.0f;
      synth.phase++;
      if (synth.phase >= synth.period) {
        synth.phase %= synth.period;
        if (spec->wave_kind == AZ_NOISE_WAVE) {
          refill_noise_buffer();
        }
      }
      // base waveform
      assert(synth.period > 0);
      float fp = (float)synth.phase / synth.period;
      switch (spec->wave_kind) {
        case AZ_NOISE_WAVE:
          sample = synth.noise_buffer[synth.phase * 32 / synth.period];
          break;
        case AZ_SAWTOOTH_WAVE:
          sample = 1.0f - fp * 2.0f;
          break;
        case AZ_SINE_WAVE:
          sample = (float)sin(fp * AZ_TWO_PI);
          break;
        case AZ_SQUARE_WAVE:
          sample = (fp < synth.square_duty ? 0.5f : -0.5f);
          break;
        case AZ_TRIANGLE_WAVE:
          sample = 4.0f * fabsf(fp - 0.5f) - 1.0f;
          break;
        case AZ_WOBBLE_WAVE:
          sample = (float)(0.5 * (cos(fp * AZ_TWO_PI) +
                                  sin(2.0 * fp * AZ_TWO_PI)));
          break;
      }
      // lp filter
      float pp = synth.fltp;
      synth.fltw *= synth.fltw_d;
      if (synth.fltw < 0.0f) synth.fltw = 0.0f;
      if (synth.fltw > 0.1f) synth.fltw = 0.1f;
      if (spec->lpf_cutoff != 0.0f) {
        synth.fltdp += (sample - synth.fltp) * synth.fltw;
        synth.fltdp -= synth.fltdp * synth.fltdmp;
      } else {
        synth.fltp = sample;
        synth.fltdp = 0.0f;
      }
      synth.fltp += synth.fltdp;
      // hp filter
      synth.fltphp += synth.fltp - pp;
      synth.fltphp -= synth.fltphp * synth.flthp;
      sample = synth.fltphp;
      // phaser
      synth.phaser_buffer[synth.ipp & 1023] = sample;
      sample += synth.phaser_buffer[(synth.ipp - synth.iphase + 1024) & 1023];
      synth.ipp = (synth.ipp + 1) & 1023;
      // final accumulation and envelope application
      ssample += sample * synth.env_vol;
    }
    const float master_vol = 0.05f;
    ssample = ssample / 8 * master_vol;

    ssample *= 1.0f + spec->volume_adjust;

    ssample *= 4.0f; // arbitrary gain to get reasonable output volume...
    if (ssample > 1.0f) ssample = 1.0f;
    if (ssample < -1.0f) ssample = -1.0f;
    filesample += ssample;
    ++fileacc;
    if (fileacc == 2) {
      filesample /= fileacc;
      fileacc = 0;
      synth.samples[synth.num_samples++] = (int16_t)(filesample * 32000);
      filesample = 0.0f;
    }
  }

  // Trim unneeded zeros off the end.
  while (synth.num_samples > 0 &&
         synth.samples[synth.num_samples - 1] == 0) {
    --synth.num_samples;
  }
}

/*===========================================================================*/

void az_create_sound_data(const az_sound_spec_t *spec, az_sound_data_t *data) {
  assert(spec != NULL);
  assert(data != NULL);
  synth_sound(spec);
  data->num_samples = synth.num_samples;
  data->samples = AZ_ALLOC(synth.num_samples, int16_t);
  memcpy(data->samples, synth.samples, synth.num_samples * sizeof(int16_t));
}

void az_destroy_sound_data(az_sound_data_t *data) {
  assert(data != NULL);
  free(data->samples);
  AZ_ZERO_OBJECT(data);
}

/*===========================================================================*/
