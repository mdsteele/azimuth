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

#include "zfxr/state.h"

#include <math.h>

#include "azimuth/util/audio.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/random.h"
#include "azimuth/util/sound.h"

/*===========================================================================*/

void az_init_zfxr_state(az_zfxr_state_t *state) {
  AZ_ZERO_OBJECT(state);
  state->sound_spec.wave_kind = AZ_TRIANGLE_WAVE;
  state->sound_spec.env_decay = 0.375;
  state->sound_spec.start_freq = 0.25;
  state->sound_spec.freq_slide = 0.25;
  az_create_sound_data(&state->sound_spec, &state->sound_data);
  state->request_play = true;
}

void az_tick_zfxr_state(az_zfxr_state_t *state, double time) {
  if (state->request_play) {
    if (state->ready_to_play) {
      az_destroy_sound_data(&state->sound_data);
      az_create_sound_data(&state->sound_spec, &state->sound_data);
      az_persist_sound_data(&state->soundboard, &state->sound_data);
      state->request_play = false;
      state->ready_to_play = false;
    } else {
      az_reset_sound_data(&state->soundboard, &state->sound_data);
      state->ready_to_play = true;
    }
  } else {
    az_persist_sound_data(&state->soundboard, &state->sound_data);
  }
}

/*===========================================================================*/

// The randomizers in this section of the file are adapted from sfxr
// (http://www.drpetter.se/project_sfxr.html), written by Tomas Pettersson
// ("DrPetter").  sfxr is made available under the MIT/Expat license,
// reproduced here:

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

static void reset_spec(az_zfxr_state_t *state) {
  az_sound_spec_t *spec = &state->sound_spec;
  AZ_ZERO_OBJECT(spec);
  spec->wave_kind = AZ_SQUARE_WAVE;
  spec->env_sustain = 0.3f;
  spec->env_decay = 0.4f;
  spec->start_freq = 0.3f;
  state->request_play = true;
}

void az_zfxr_random_pickup(az_zfxr_state_t *state) {
  reset_spec(state);
  az_sound_spec_t *spec = &state->sound_spec;
  spec->start_freq = az_random(0.4, 0.9);
  spec->env_sustain = az_random(0, 0.1);
  spec->env_decay = az_random(0.1, 0.5);
  spec->env_punch = az_random(0.3, 0.6);
  if (az_random(0, 1) < 0.5) {
    spec->arp_mod = az_random(0.2, 0.6);
    spec->arp_speed = az_random(0.5, 0.7);
  }
}

void az_zfxr_random_laser(az_zfxr_state_t *state) {
  reset_spec(state);
  az_sound_spec_t *spec = &state->sound_spec;
  static const az_sound_wave_kind_t kinds[] = {
    AZ_SAWTOOTH_WAVE, AZ_SINE_WAVE, AZ_SQUARE_WAVE, AZ_TRIANGLE_WAVE,
    AZ_WOBBLE_WAVE
  };
  spec->wave_kind = kinds[az_randint(0, AZ_ARRAY_SIZE(kinds) - 1)];
  if (az_randint(0, 2) == 0) {
    spec->start_freq = az_random(0.3, 0.9);
    spec->freq_limit = az_random(0, 0.1);
    spec->freq_slide = az_random(-0.65, -0.35);
  } else {
    spec->start_freq = az_random(0.5, 1.0);
    spec->freq_limit =
      fmax(0.2, spec->start_freq - az_random(0.2, 0.8));
    spec->freq_slide = az_random(-0.35, -0.15);
  }
  if (spec->wave_kind == AZ_SQUARE_WAVE) {
    if (az_randint(0, 1) == 0) {
      spec->square_duty = az_random(0, 0.5);
      spec->duty_sweep = az_random(0, 0.2);
    } else {
      spec->square_duty = az_random(0.4, 0.9);
      spec->duty_sweep = az_random(-0.7, 0);
    }
  }
  spec->env_sustain = az_random(0.1, 0.3);
  spec->env_decay = az_random(0, 0.4);
  if (az_randint(0, 1) == 0) {
    spec->env_punch = az_random(0, 0.3);
  }
  if (az_randint(0, 2) == 0) {
    spec->phaser_offset = az_random(0, 0.2);
    spec->phaser_sweep = az_random(-0.2, 0);
  }
  if (az_randint(0, 1) == 0) {
    spec->hpf_cutoff = az_random(0, 0.3);
  }
}

void az_zfxr_random_explosion(az_zfxr_state_t *state) {
  reset_spec(state);
  az_sound_spec_t *spec = &state->sound_spec;
  spec->wave_kind = AZ_NOISE_WAVE;
  if (az_randint(0, 1) == 0) {
    spec->start_freq = az_random(0.1, 0.5);
    spec->freq_slide = az_random(-0.1, 0.3);
  } else {
    spec->start_freq = az_random(0.2, 0.9);
    spec->freq_slide = az_random(-0.4, -0.2);
  }
  spec->start_freq *= spec->start_freq;
  if (az_randint(0, 4) == 0) spec->freq_slide = 0;
  if (az_randint(0, 2) == 0) {
    spec->repeat_speed = az_random(0.3, 0.8);
  }
  spec->env_sustain = az_random(0.1, 0.4);
  spec->env_punch = az_random(0.2, 0.8);
  spec->env_decay = az_random(0, 0.5);
  if (az_randint(0, 1) == 0) {
    spec->phaser_offset = az_random(-0.3, 0.6);
    spec->phaser_sweep = az_random(-0.3, 0);
  }
  if (az_randint(0, 1) == 0) {
    spec->vibrato_depth = az_random(0, 0.7);
    spec->vibrato_speed = az_random(0, 0.6);
  }
  if (az_randint(0, 2) == 0) {
    spec->arp_mod = az_random(-0.8, 0.8);
    spec->arp_speed = az_random(0.6, 0.9);
  }
}

void az_zfxr_random_powerup(az_zfxr_state_t *state) {
  reset_spec(state);
  az_sound_spec_t *spec = &state->sound_spec;
  if (az_randint(0, 1) == 0) {
    spec->wave_kind = AZ_SAWTOOTH_WAVE;
  } else {
    spec->wave_kind = AZ_SQUARE_WAVE;
    spec->square_duty = az_random(0, 0.6);
  }
  spec->start_freq = az_random(0.2, 0.5);
  if (az_randint(0, 1) == 0) {
    spec->freq_slide = az_random(0.1, 0.5);
    spec->repeat_speed = az_random(0.4, 0.8);
  } else {
    spec->freq_slide = az_random(0.05, 0.25);
    if (az_randint(0, 1) == 0) {
      spec->vibrato_depth = az_random(0, 0.7);
      spec->vibrato_speed = az_random(0, 0.6);
    }
  }
  spec->env_sustain = az_random(0, 0.4);
  spec->env_decay = az_random(0.1, 0.5);
}

void az_zfxr_random_hurt(az_zfxr_state_t *state) {
  reset_spec(state);
  az_sound_spec_t *spec = &state->sound_spec;
  static const az_sound_wave_kind_t kinds[] = {
    AZ_NOISE_WAVE, AZ_SAWTOOTH_WAVE, AZ_SQUARE_WAVE
  };
  spec->wave_kind = kinds[az_randint(0, AZ_ARRAY_SIZE(kinds) - 1)];
  if (spec->wave_kind == AZ_SQUARE_WAVE) {
    spec->square_duty = az_random(0, 0.6);
  }
  spec->start_freq = az_random(0.2, 0.8);
  spec->freq_slide = az_random(-0.3, 0.1);
  spec->env_sustain = az_random(0, 0.1);
  spec->env_decay = az_random(0.1, 0.3);
  if (az_randint(0, 1) == 0) {
    spec->hpf_cutoff = az_random(0, 0.3);
  }
}

void az_zfxr_random_jump(az_zfxr_state_t *state) {
  reset_spec(state);
  az_sound_spec_t *spec = &state->sound_spec;
  spec->wave_kind = AZ_SQUARE_WAVE;
  spec->square_duty = az_random(0, 0.6);
  spec->start_freq = az_random(0.3, 0.6);
  spec->freq_slide = az_random(0.1, 0.3);
  spec->env_sustain = az_random(0.1, 0.4);
  spec->env_sustain = az_random(0.1, 0.3);
  if (az_randint(0, 1) == 0) {
    spec->hpf_cutoff = az_random(0, 0.3);
  }
  if (az_randint(0, 1) == 0) {
    spec->lpf_cutoff = az_random(0, 0.6);
  }
}

void az_zfxr_random_blip(az_zfxr_state_t *state) {
  reset_spec(state);
  az_sound_spec_t *spec = &state->sound_spec;
  static const az_sound_wave_kind_t kinds[] = {
    AZ_SAWTOOTH_WAVE, AZ_SQUARE_WAVE
  };
  spec->wave_kind = kinds[az_randint(0, AZ_ARRAY_SIZE(kinds) - 1)];
  if (spec->wave_kind == AZ_SQUARE_WAVE) {
    spec->square_duty = az_random(0, 0.6);
  }
  spec->start_freq = az_random(0.2, 0.6);
  spec->env_sustain = az_random(0.1, 0.2);
  spec->env_decay = az_random(0, 0.2);
  spec->hpf_cutoff = 0.1f;
}

void az_zfxr_randomize(az_zfxr_state_t *state) {
  reset_spec(state);
  az_sound_spec_t *spec = &state->sound_spec;
  static const az_sound_wave_kind_t kinds[] = {
    AZ_NOISE_WAVE, AZ_SAWTOOTH_WAVE, AZ_SINE_WAVE, AZ_SQUARE_WAVE,
    AZ_TRIANGLE_WAVE, AZ_WOBBLE_WAVE
  };
  spec->wave_kind = kinds[az_randint(0, AZ_ARRAY_SIZE(kinds) - 1)];
  if (spec->wave_kind == AZ_SQUARE_WAVE) {
    spec->square_duty = az_random(0, 1);
    spec->duty_sweep = pow(az_random(0, 1), 3);
  }
  spec->env_attack = pow(az_random(0, 1), 3);
  spec->env_sustain = pow(az_random(0, 1), 2);
  spec->env_punch = pow(az_random(0, 0.8), 2);
  spec->env_decay = az_random(0, 1);
  if (spec->env_attack + spec->env_sustain + spec->env_decay < 0.2f) {
    spec->env_sustain += az_random(0.2, 0.5);
    spec->env_decay += az_random(0.2, 0.5);
  }
  spec->start_freq = pow(az_random(0, 1), 2);
  spec->freq_slide = pow(az_random(-1, 1), 5);
  if (spec->start_freq > 0.7f && spec->freq_slide > 0.2f) {
    spec->freq_slide = -spec->freq_slide;
  }
  if (spec->start_freq < 0.2f && spec->freq_slide < -0.05f) {
    spec->freq_slide = -spec->freq_slide;
  }
  spec->freq_delta_slide = pow(az_random(-1, 1), 3);
  spec->vibrato_depth = pow(az_random(0, 1), 3);
  spec->vibrato_speed = az_random(0, 1);
  spec->arp_mod = az_random(-1, 1);
  spec->arp_speed = az_random(0, 1);
  spec->repeat_speed = az_random(0, 1);
  spec->phaser_offset = pow(az_random(-1, 1), 3);
  spec->phaser_sweep = pow(az_random(-1, 1), 3);
  spec->lpf_cutoff = pow(az_random(0, 1), 3);
  spec->lpf_ramp = pow(az_random(-1, 1), 3);
  if (spec->lpf_cutoff < 0.1f && spec->lpf_ramp < -0.05f) {
    spec->lpf_ramp = -spec->lpf_ramp;
  }
  spec->lpf_resonance = az_random(-1, 1);
  spec->hpf_cutoff = pow(az_random(0, 1), 5);
  spec->hpf_ramp = pow(az_random(-1, 1), 5);
}

/*===========================================================================*/
