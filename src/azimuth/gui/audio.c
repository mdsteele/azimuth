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

#include "azimuth/gui/audio.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>

#include "azimuth/system/resource.h"
#include "azimuth/util/audio.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/random.h"
#include "azimuth/util/vector.h" // for AZ_TWO_PI

/*===========================================================================*/
// Constants:

// We use 16-bit stereo at 22050 samples/sec and a buffer size of 4096 samples.
#define AUDIO_FORMAT AUDIO_S16SYS
#define AUDIO_CHANNELS 2
#define AUDIO_RATE 22050
#define AUDIO_BUFFERSIZE 4096

// How many sound effects we can play simultaneously:
#define NUM_MIXER_CHANNELS 16

/*===========================================================================*/
// Music:

static struct {
  const char *filename;
  Mix_Music *music; // may be NULL if music failed to load
} music_entries[] = {
  [AZ_MUS_TITLE] = { .filename = "title-screen.mp3" },
  [AZ_MUS_CNIDAM_ZONE] = { .filename = "cnidam-zone.mp3" },
  [AZ_MUS_ZENITH_CORE] = { .filename = "zenith-core.mp3" }
};

static void load_all_music(void) {
  const char *resource_dir = az_get_resource_directory();
  char path_buffer[strlen(resource_dir) + 50u];
  AZ_ARRAY_LOOP(entry, music_entries) {
    assert(entry->filename != NULL);
    assert(strlen(entry->filename) > 0);
    sprintf(path_buffer, "%s/music/%s", resource_dir, entry->filename);
    entry->music = Mix_LoadMUS(path_buffer);
    if (entry->music == NULL) {
      fprintf(stderr, "Warning: failed to load %s\n", entry->filename);
    }
  }
}

static void free_all_music(void) {
  AZ_ARRAY_LOOP(entry, music_entries) {
    if (entry->music != NULL) {
      Mix_FreeMusic(entry->music);
      entry->music = NULL;
    }
  }
}

static void tick_music(const az_soundboard_t *soundboard) {
  static enum { STOPPED, FADING_OUT, PLAYING } music_status = STOPPED;
  static az_music_key_t current_music = AZ_MUS_TITLE;
  static bool music_pending = false;
  static az_music_key_t next_music = AZ_MUS_TITLE;

  // If nothing is playing right now (e.g. because we finished fading out, or
  // because the last music we started failed to load at program startup), set
  // us to STOPPED status.
  if (!Mix_PlayingMusic()) music_status = STOPPED;

  // If desired, change which music will play next.
  int fade_out_millis = -1;
  switch (soundboard->music_action) {
    case AZ_MUSA_NOTHING: break;
    case AZ_MUSA_CHANGE:
      next_music = soundboard->next_music;
      music_pending = true;
      if (current_music != next_music || music_status != PLAYING) {
        fade_out_millis = 500;
      }
      break;
    case AZ_MUSA_STOP:
      assert(soundboard->music_fade_out_millis >= 0);
      music_pending = false;
      fade_out_millis = soundboard->music_fade_out_millis;
      break;
  }
  if (fade_out_millis >= 0 && music_status == PLAYING) {
    if (fade_out_millis > 0) {
      Mix_FadeOutMusic(fade_out_millis);
      music_status = FADING_OUT;
    } else {
      Mix_HaltMusic();
      music_status = STOPPED;
    }
  }

  // If we're ready to start the next music, do so.
  if (music_pending && music_status == STOPPED) {
    current_music = next_music;
    music_pending = false;
    Mix_Music *music_data = music_entries[current_music].music;
    if (music_data != NULL && Mix_PlayMusic(music_data, -1) == 0) {
      music_status = PLAYING;
    }
  }
}

/*===========================================================================*/
// Sound effects:

// This section of the file contains a modified version of the sound and WAV
// file generation code from sfxr (http://www.drpetter.se/project_sfxr.html),
// written by Tomas Pettersson ("DrPetter").  sfxr is made available under the
// MIT/Expat license, reproduced here:

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

// Rather than store our sound effects as resource files, we generate them from
// scratch at program startup.  For each sound effect, the table below stores
// the sfxr parameters used to generate the sound; then in init_all_sounds(),
// for each entry in the table we call synth_sound_wav() to run the sfxr
// algorithm and generate an in-memory WAV file, which we then pass to the SDL
// mixer to read and convert into a Mix_Chunk.

typedef struct {
  // Input parameters to the sfxr algorithm.  These correspond directly to the
  // sliders in sfxr, and each have ranges of [0,1] or [-1,1].  Two minor
  // adjustments have been made: first, lpf_cutoff here is 1 minus its usual
  // value in sfxr (this is so that we can leave it zero most of the time,
  // rather than constantly having to explicitly specify it as 1.0), and
  // second, we use a volume range of -1 to 1 instead of 0 to 1 (again so that
  // we can usually leave it at zero, rather than explicitly specifying 0.5).
  enum { SQUARE, SAWTOOTH, TRIANGLE, SINE, NOISE } wave_kind;
  float env_attack, env_sustain, env_punch, env_decay;
  float start_freq, freq_limit, freq_slide, freq_delta_slide;
  float vibrato_depth, vibrato_speed;
  float arp_mod, arp_speed;
  float square_duty, duty_sweep;
  float repeat_speed;
  float phaser_offset, phaser_sweep;
  float lpf_cutoff, lpf_ramp, lpf_resonance; // cutoff is 1 - value from sfxr
  float hpf_cutoff, hpf_ramp;
  float volume_adjust; // -1.0 to 1.0
  // SDL mixer chunk object, produced by running the sfxr algorithm:
  Mix_Chunk *chunk;
} az_sound_entry_t;

static az_sound_entry_t sound_entries[] = {
  [AZ_SND_BEAM_FREEZE] = {
    .wave_kind = TRIANGLE,
    .env_sustain = 1.0, .start_freq = 0.3943662,
    .vibrato_depth = 0.1056338, .vibrato_speed = 0.57042253,
    .square_duty = 0.35298
  },
  [AZ_SND_BEAM_NORMAL] = {
    .wave_kind = SQUARE,
    .env_sustain = 0.1971831, .start_freq = 0.2943662,
    .vibrato_depth = 0.1056338, .vibrato_speed = 0.57042253,
    .square_duty = 0.35298
  },
  [AZ_SND_BEAM_PHASE] = {
    .wave_kind = SQUARE,
    .env_sustain = 0.1971831, .start_freq = 0.3443662,
    .vibrato_depth = 0.1056338, .vibrato_speed = 0.57042253,
    .square_duty = 0.35298
  },
  [AZ_SND_BEAM_PIERCE] = {
    .wave_kind = SAWTOOTH,
    .env_sustain = 0.1971831, .start_freq = 0.3943662,
    .vibrato_depth = 0.1056338, .vibrato_speed = 0.57042253,
    .square_duty = 0.35298
  },
  [AZ_SND_BLINK_MEGA_BOMB] = {
    .wave_kind = SAWTOOTH,
    .env_sustain = 0.119718313217,
    .env_decay = 0.0704225376248,
    .start_freq = 0.41549295187,
    .square_duty = 0.19812,
  },
  [AZ_SND_CHARGED_GUN] = {
    .wave_kind = SAWTOOTH,
    .env_sustain = 0.323943674564,
    .start_freq = 0.232394367456, .freq_slide = 0.004733738,
    .vibrato_depth = 0.33098590374, .vibrato_speed = 0.33098590374,
    .arp_speed = -0.4154, .square_duty = -0.8286, .phaser_offset = -0.7323943,
    .volume_adjust = -0.462962989807
  },
  [AZ_SND_CHARGED_ORDNANCE] = {
    .wave_kind = TRIANGLE,
    .env_sustain = 0.6,
    .start_freq = 0.202394367456, .freq_slide = 0.004733738,
    .vibrato_depth = 0.33098590374, .vibrato_speed = 0.25,
    .arp_speed = -0.4154, .square_duty = -0.8286, .phaser_offset = -0.7323943,
    .volume_adjust = -0.462962989807
  },
  [AZ_SND_CHARGING_GUN] = {
    .wave_kind = SQUARE,
    .env_attack = 1.0, .env_sustain = 0.34507, .env_decay = 0.246478870511,
    .start_freq = 0.1197183, .freq_slide = 0.118591,
    .freq_delta_slide = 0.056338,
    .square_duty = 0.40368, .duty_sweep = 0.0140844583511
  },
  [AZ_SND_CHARGING_ORDNANCE] = {
    .wave_kind = TRIANGLE,
    .env_attack = 1.0, .env_sustain = 0.34507, .env_decay = 0.246478870511,
    .start_freq = 0.0897183, .freq_slide = 0.118591,
    .freq_delta_slide = 0.056338,
    .square_duty = 0.40368, .duty_sweep = 0.0140844583511
  },
  [AZ_SND_DOOR_CLOSE] = {
    .wave_kind = SINE,
    .env_sustain = 0.352112680674, .env_decay = 0.12622,
    .start_freq = 0.401408463717, .freq_slide = -0.197183,
    .square_duty = 0.53694, .volume_adjust = -0.5
  },
  [AZ_SND_DOOR_OPEN] = {
    .wave_kind = SINE,
    .env_sustain = 0.352112680674, .env_decay = 0.12622,
    .start_freq = 0.246478870511, .freq_slide = 0.197183,
    .square_duty = 0.53694, .volume_adjust = -0.5
  },
  [AZ_SND_DROP_BOMB] = {
    .wave_kind = SINE,
    .env_sustain = 0.15671, .env_decay = 0.00384,
    .start_freq = 0.380281686783, .square_duty = 0.36672,
    .hpf_cutoff = 0.1, .volume_adjust = -0.314814834595
  },
  [AZ_SND_EXPLODE_BOMB] = {
    .wave_kind = NOISE,
    .env_sustain = 0.32941, .env_punch = 0.572, .env_decay = 0.23305,
    .start_freq = 0.1548265, .freq_slide = -0.36946,
    .vibrato_depth = 0.63161, .vibrato_speed = 0.38466,
    .phaser_offset = 0.12768, .phaser_sweep = -0.28872
  },
  [AZ_SND_EXPLODE_HYPER_ROCKET] = {
    .wave_kind = NOISE,
    .env_sustain = 0.31483, .env_punch = 0.26024, .env_decay = 0.4112,
    .start_freq = 0.161971837282, .repeat_speed = 0.3807,
    .volume_adjust = 0.333333435059
  },
  [AZ_SND_EXPLODE_MEGA_BOMB] = {
    .wave_kind = NOISE,
    .env_sustain = 0.4366197, .env_punch = 0.443, .env_decay = 0.3309859,
    .start_freq = 0.161395, .freq_slide = -0.0140845179558,
    .arp_mod = -0.5244799, .arp_speed = 0.74647885561, .repeat_speed = 0.59345
  },
  [AZ_SND_EXPLODE_ROCKET] = {
    .wave_kind = NOISE,
    .env_sustain = 0.29941, .env_punch = 0.69464, .env_decay = 0.3665,
    .start_freq = 0.6008111, .freq_slide = -0.25694,
    .repeat_speed = 0.7266001, .volume_adjust = 0.222222213745
  },
  [AZ_SND_EXPLODE_SHIP] = {
    .wave_kind = NOISE,
    .env_sustain = 0.852112650, .env_punch = 1.0, .env_decay = 0.767605662,
    .start_freq = 0.112676054239, .freq_slide = -0.0985915660858,
    .vibrato_depth = 0.281690150499, .vibrato_speed = 0.12924,
  },
  [AZ_SND_FIRE_GUN_FREEZE] = {
    .wave_kind = SINE,
    .env_sustain = 0.13848, .env_punch = 0.12036, .env_decay = 0.21536,
    .start_freq = 0.9788732, .freq_limit = 0.32917, .freq_slide = -0.22554,
    .square_duty = 0.46895, .duty_sweep = 0.1842,
    .hpf_cutoff = 0.007470001
  },
  [AZ_SND_FIRE_GUN_NORMAL] = {
    .wave_kind = SINE,
    .env_sustain = 0.10346, .env_punch = 0.16836, .env_decay = 0.18956,
    .start_freq = 0.89034, .freq_limit = 0.08224, .freq_slide = -0.61922,
    .square_duty = 0.2347, .duty_sweep = 0.0176,
    .hpf_cutoff = 0.25965
  },
  [AZ_SND_FIRE_GUN_PIERCE] = {
    .wave_kind = SAWTOOTH,
    .env_sustain = 0.23076, .env_decay = 0.05284,
    .start_freq = 0.7278, .freq_limit = 0.29536, .freq_slide = -0.29848,
    .square_duty = 0.46265, .duty_sweep = -0.18305
  },
  [AZ_SND_FIRE_ROCKET] = {
    .wave_kind = NOISE,
    .env_sustain = 0.39907, .env_punch = 0.65582, .env_decay = 0.39085,
    .start_freq = 0.7881, .freq_slide = -0.30088,
    .vibrato_depth = 0.59703, .vibrato_speed = 0.03828,
    .phaser_offset = -0.09300001, .phaser_sweep = -0.19305,
    .volume_adjust = -0.42592590332
  },
  [AZ_SND_FIRE_HYPER_ROCKET] = {
    .wave_kind = NOISE,
    .env_sustain = 0.3, .env_punch = 0.25, .env_decay = 0.5,
    .start_freq = 0.2, .vibrato_depth = 0.5, .vibrato_speed = 0.75,
    .repeat_speed = 0.55,
    .phaser_offset = 0.25, .phaser_sweep = -0.25
  },
  [AZ_SND_HIT_WALL] = {
    .wave_kind = NOISE,
    .env_sustain = 0.03245, .env_decay = 0.10518,
    .start_freq = 0.43334, .freq_slide = -0.57352
  },
  [AZ_SND_PICKUP_ORDNANCE] = {
    .wave_kind = SQUARE,
    .env_sustain = 0.17572, .env_decay = 0.2746479,
    .start_freq = 0.26918, .freq_slide = 0.25928,
    .square_duty = 0.01332, .repeat_speed = 0.69412
  },
  [AZ_SND_PICKUP_SHIELDS] = {
    .wave_kind = SQUARE,
    .env_sustain = 0.1376, .env_decay = 0.288732379675,
    .start_freq = 0.225352108479, .freq_slide = 0.2832,
    .repeat_speed = 0.542253494263
  }
};

static struct {
  // Fields for maintaining sfxr synth state:
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
  // Fields for constructing WAV files:
  char wav_buffer[128 * 1024]; // buffer for storing in-memory WAV file
  char *wav_chunk_size;  // points to where in wav_buffer we write chunk size
  char *wav_data_size;   // points to where in wav_buffer we write data size
  char *wav_data_start;  // points to where in wav_buffer we write actual data
  size_t wav_buffer_len; // stores total size of WAV file
} synth;

// Copy a NUL-terminated string to the buffer and advance the pointer.
static void write_string(const char *str, char **ptr) {
  const size_t length = strlen(str);
  memcpy(*ptr, str, length);
  *ptr += length;
}

// Write a uint16 to the buffer in little-endian order and advance the pointer.
static void write_uint16le(uint16_t value, char **ptr) {
  (*ptr)[0] = value & 0xff;
  (*ptr)[1] = (value >> 8) & 0xff;
  *ptr += 2;
}

// Write a uint32 to the buffer in little-endian order and advance the pointer.
static void write_uint32le(uint32_t value, char **ptr) {
  (*ptr)[0] = value & 0xff;
  (*ptr)[1] = (value >> 8) & 0xff;
  (*ptr)[2] = (value >> 16) & 0xff;
  (*ptr)[3] = (value >> 24) & 0xff;
  *ptr += 4;
}

// Fill synth.wav_buffer with the common WAV header data, and initialize the
// synth.wav_* pointers so that we can fill those fields in for each WAV that
// we generate.
static void init_wav_buffer(void) {
  const int sample_rate = 22050; // 22.05 kHz
  const int sample_bytes = 2; // 16-bit samples
  const int num_channels = 1; // mono
  char *ptr = synth.wav_buffer;
  // File header:
  write_string("RIFF", &ptr); // chunk ID
  synth.wav_chunk_size = ptr;
  ptr += 4; // leave space for file size
  write_string("WAVE", &ptr); // chunk format
  // Subchunk 1 (fmt):
  write_string("fmt ", &ptr); // subchunk 1 ID
  write_uint32le(16, &ptr); // subchunk 1 size
  write_uint16le(1, &ptr); // audio format (1 = PCM, no compression)
  write_uint16le(num_channels, &ptr); // num channels (1 = mono)
  write_uint32le(sample_rate, &ptr); // sample rate
  write_uint32le(sample_rate * sample_bytes * num_channels, &ptr); // bytes/sec
  write_uint16le(sample_bytes * num_channels, &ptr); // block align
  write_uint16le(sample_bytes * 8, &ptr); // bits per sample
  // Subchunk 2 (data):
  write_string("data", &ptr); // subchunk 1 ID
  synth.wav_data_size = ptr;
  ptr += 4; // leave space for data size
  synth.wav_data_start = ptr;
}

// Reset the sfxr synth.  This code is taken directly from sfxr, with only
// minor changes.
static void reset_synth(const az_sound_entry_t *entry, bool restart) {
  if (!restart) synth.phase = 0;
  synth.fperiod = 100.0 / (entry->start_freq * entry->start_freq + 0.001);
  synth.period = (int)synth.fperiod;
  synth.fmaxperiod = 100.0 / (entry->freq_limit * entry->freq_limit + 0.001);
  synth.fslide = 1.0 - pow(entry->freq_slide, 3.0) * 0.01;
  synth.fdslide = -pow(entry->freq_delta_slide, 3.0) * 0.000001;
  synth.square_duty = 0.5f - entry->square_duty * 0.5f;
  synth.square_slide = -entry->duty_sweep * 0.00005f;
  if (entry->arp_mod >= 0.0f) {
    synth.arp_mod = 1.0 - pow(entry->arp_mod, 2.0) * 0.9;
  } else {
    synth.arp_mod = 1.0 + pow(entry->arp_mod, 2.0) * 10.0;
  }
  synth.arp_time = 0;
  synth.arp_limit = (int)(pow(1.0 - entry->arp_speed, 2.0) * 20000 + 32);
  if (entry->arp_speed == 1.0f) synth.arp_limit = 0;
  if (!restart) {
    // Reset filter:
    synth.fltp = 0.0f;
    synth.fltdp = 0.0f;
    synth.fltw = pow(1.0 - entry->lpf_cutoff, 3.0) * 0.1f;
    synth.fltw_d = 1.0f + entry->lpf_ramp * 0.0001f;
    synth.fltdmp = 5.0f / (1.0f + pow(entry->lpf_resonance, 2.0) * 20.0f) *
      (0.01f + synth.fltw);
    if (synth.fltdmp > 0.8f) synth.fltdmp = 0.8f;
    synth.fltphp = 0.0f;
    synth.flthp = pow(entry->hpf_cutoff, 2.0) * 0.1f;
    synth.flthp_d = 1.0 + entry->hpf_ramp * 0.0003f;
    // Reset vibrato:
    synth.vib_phase = 0.0f;
    synth.vib_speed = pow(entry->vibrato_speed, 2.0) * 0.01f;
    synth.vib_amp = entry->vibrato_depth * 0.5f;
    // Reset envelope:
    synth.env_vol = 0.0f;
    synth.env_stage = 0;
    synth.env_time = 0;
    synth.env_length[0] =
      (int)(entry->env_attack * entry->env_attack * 100000.0f);
    synth.env_length[1] =
      (int)(entry->env_sustain * entry->env_sustain * 100000.0f);
    synth.env_length[2] =
      (int)(entry->env_decay * entry->env_decay * 100000.0f);
    // Reset phaser:
    synth.fphase = pow(entry->phaser_offset, 2.0) * 1020.0f;
    if (entry->phaser_offset < 0.0f) synth.fphase = -synth.fphase;
    synth.fdphase = pow(entry->phaser_sweep, 2.0);
    if (entry->phaser_sweep < 0.0f) synth.fdphase = -synth.fdphase;
    synth.iphase = abs((int)synth.fphase);
    synth.ipp = 0;
    for (int i = 0; i < 1024; ++i)
      synth.phaser_buffer[i] = 0.0f;
    // Refill noise buffer:
    for (int i = 0; i < 32; ++i)
      synth.noise_buffer[i] = az_random(-1.0, 1.0);
    // Reset repeat:
    synth.rep_time = 0;
    synth.rep_limit =
      (int)(pow(1.0f - entry->repeat_speed, 2.0f) * 20000 + 32);
    if (entry->repeat_speed == 0.0f) synth.rep_limit = 0;
  }
}

// Generate the given sound effect and populate synth.wav_buffer with an
// in-memory WAV file of the sound.  This code is taken directly from sfxr,
// with only minor changes.
static void synth_sound_wav(const az_sound_entry_t *entry) {
  const char *end = synth.wav_buffer + AZ_ARRAY_SIZE(synth.wav_buffer);
  char *ptr = synth.wav_data_start;
  reset_synth(entry, false);
  float filesample = 0.0f;
  int fileacc = 0;
  bool finished = false;

  while (ptr < end && !finished) {
    ++synth.rep_time;
    if (synth.rep_limit != 0 && synth.rep_time >= synth.rep_limit) {
      synth.rep_time = 0;
      reset_synth(entry, true);
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
      if (entry->freq_limit > 0.0f) finished = true;
    }
    float rfperiod = synth.fperiod;
    if (synth.vib_amp > 0.0f) {
      synth.vib_phase += synth.vib_speed;
      rfperiod = synth.fperiod * (1.0 + sin(synth.vib_phase) * synth.vib_amp);
    }
    synth.period = (int)rfperiod;
    if (synth.period < 8) synth.period = 8;
    synth.square_duty += synth.square_slide;
    if (synth.square_duty < 0.0f) synth.square_duty=0.0f;
    if (synth.square_duty > 0.5f) synth.square_duty=0.5f;
    // volume envelope
    synth.env_time++;
    if (synth.env_time > synth.env_length[synth.env_stage]) {
      synth.env_time = 0;
      ++synth.env_stage;
      if (synth.env_stage == 3) finished = true;
    }
    if (synth.env_stage == 0)
      synth.env_vol = (float)synth.env_time / synth.env_length[0];
    if (synth.env_stage == 1)
      synth.env_vol = 1.0f +
        pow(1.0f - (float)synth.env_time / synth.env_length[1], 1.0f) *
        2.0f * entry->env_punch;
    if (synth.env_stage==2)
      synth.env_vol = 1.0f - (float)synth.env_time / synth.env_length[2];

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
        if (entry->wave_kind == NOISE) {
          for (int i = 0; i < 32; ++i) {
            synth.noise_buffer[i] = az_random(-1.0, 1.0);
          }
        }
      }
      // base waveform
      float fp = (float)synth.phase / synth.period;
      switch (entry->wave_kind) {
        case SQUARE:
          sample = (fp < synth.square_duty ? 0.5f : -0.5f);
          break;
        case SAWTOOTH:
          sample = 1.0f - fp * 2.0f;
          break;
        case TRIANGLE:
          sample = 4.0f * fabs(fp - 0.5f) - 1.0f;
          break;
        case SINE:
          sample = (float)sin(fp * AZ_TWO_PI);
          break;
        case NOISE:
          sample = synth.noise_buffer[synth.phase * 32 / synth.period];
          break;
      }
      // lp filter
      float pp = synth.fltp;
      synth.fltw *= synth.fltw_d;
      if (synth.fltw < 0.0f) synth.fltw = 0.0f;
      if (synth.fltw > 0.1f) synth.fltw = 0.1f;
      if (entry->lpf_cutoff != 0.0f) {
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

    ssample *= 1.0f + entry->volume_adjust;

    ssample *= 4.0f; // arbitrary gain to get reasonable output volume...
    if (ssample > 1.0f) ssample = 1.0f;
    if (ssample < -1.0f) ssample = -1.0f;
    filesample += ssample;
    ++fileacc;
    if (fileacc == 2) {
      filesample /= fileacc;
      fileacc = 0;
      write_uint16le((uint16_t)(filesample * 32000), &ptr);
      filesample = 0.0f;
    }
  }

  // Fill in file and data sizes.
  const size_t data_size = ptr - synth.wav_data_start;
  ptr = synth.wav_data_size;
  write_uint32le(data_size, &ptr);
  assert(synth.wav_data_start > synth.wav_buffer);
  synth.wav_buffer_len = synth.wav_data_start - synth.wav_buffer + data_size;
  ptr = synth.wav_chunk_size;
  write_uint32le(synth.wav_buffer_len - 8, &ptr);
}

static void generate_all_sounds(void) {
  init_wav_buffer();
  AZ_ARRAY_LOOP(entry, sound_entries) {
    synth_sound_wav(entry);
    entry->chunk = Mix_LoadWAV_RW(SDL_RWFromMem(synth.wav_buffer,
                                                synth.wav_buffer_len), true);
    if (entry->chunk == NULL) {
      AZ_FATAL("Failed to generate sound chunk\n");
    }
  }
}

static void free_all_sounds(void) {
  AZ_ARRAY_LOOP(entry, sound_entries) {
    assert(entry->chunk != NULL);
    Mix_FreeChunk(entry->chunk);
    entry->chunk = NULL;
  }
}

static void tick_sounds(const az_soundboard_t *soundboard) {
  static struct {
    enum { INACTIVE = 0, PLAYING, FINISHED } status;
    az_sound_key_t sound;
    int channel;
  } persisting_sounds[NUM_MIXER_CHANNELS];

  // Clean up unpersisted sounds.
  bool done[soundboard->num_persists];
  memset(done, 0, sizeof(bool) * soundboard->num_persists);
  AZ_ARRAY_LOOP(persisting, persisting_sounds) {
    if (persisting->status == INACTIVE) continue;
    bool halt = true;
    for (int i = 0; i < soundboard->num_persists; ++i) {
      if (soundboard->persists[i].sound == persisting->sound) {
        halt = false;
        done[i] = true;
        break;
      }
    }
    if (halt) {
      if (persisting->status == PLAYING) {
        Mix_HaltChannel(persisting->channel);
      }
      persisting->status = INACTIVE;
    } else if (persisting->status == PLAYING &&
               !Mix_Playing(persisting->channel)) {
      persisting->status = FINISHED;
    }
  }

  // Play new persistent sounds.
  for (int i = 0; i < soundboard->num_persists; ++i) {
    if (done[i]) continue;
    AZ_ARRAY_LOOP(persisting, persisting_sounds) {
      if (persisting->status == INACTIVE) {
        const az_sound_key_t sound = soundboard->persists[i].sound;
        const int channel =
          Mix_PlayChannel(-1, sound_entries[sound].chunk,
                          (soundboard->persists[i].loop ? -1 : 0));
        if (channel >= 0) {
          persisting->status = PLAYING;
          persisting->sound = sound;
          persisting->channel = channel;
        }
        break;
      }
    }
  }

  // Play one-shot sounds.
  for (int i = 0; i < soundboard->num_oneshots; ++i) {
    Mix_PlayChannel(-1, sound_entries[soundboard->oneshots[i]].chunk, 0);
  }
}

/*===========================================================================*/
// Audio system:

static bool audio_mixer_initialized = false;

static void shut_down_audio_mixer(void) {
  free_all_sounds();
  free_all_music();
  Mix_CloseAudio();
}

void az_init_audio_mixer(void) {
  assert(!audio_mixer_initialized);
  // Initialize the SDL mixer.
  if (Mix_OpenAudio(AUDIO_RATE, AUDIO_FORMAT, AUDIO_CHANNELS,
                    AUDIO_BUFFERSIZE) != 0) {
    AZ_FATAL("Mix_OpenAudio failed.\n");
  }
  atexit(shut_down_audio_mixer);
  Mix_AllocateChannels(NUM_MIXER_CHANNELS);
  // Load our music and sound data.
  load_all_music();
  generate_all_sounds();
  audio_mixer_initialized = true;
}

void az_tick_audio_mixer(az_soundboard_t *soundboard) {
  assert(audio_mixer_initialized);
  tick_music(soundboard);
  tick_sounds(soundboard);
  memset(soundboard, 0, sizeof(*soundboard));
}

/*===========================================================================*/
