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
#include <SDL_mixer/SDL_mixer.h>

#include "azimuth/system/resource.h"
#include "azimuth/util/audio.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/random.h"
#include "azimuth/util/vector.h"
#include "azimuth/util/warning.h"

/*===========================================================================*/
// Constants:

// We use 16-bit stereo at 22050 samples/sec and a buffer size of 1024 samples.
#define AUDIO_FORMAT AUDIO_S16SYS
#define AUDIO_CHANNELS 2
#define AUDIO_RATE 22050
#define AUDIO_BUFFERSIZE 1024

// How many sound effects we can play simultaneously:
#define NUM_MIXER_CHANNELS 16

/*===========================================================================*/
// Music:

static struct {
  const char *filename;
  Mix_Music *music; // may be NULL if music failed to load
} music_entries[] = {
  [AZ_MUS_TITLE] = { .filename = "title-screen.ogg" },
  [AZ_MUS_COLONY_ZONE] = { .filename = "colony-zone.mp3" },
  [AZ_MUS_FILIMUN_ZONE] = { .filename = "filimun-zone.ogg" },
  [AZ_MUS_CNIDAM_ZONE] = { .filename = "cnidam-zone.mp3" },
  [AZ_MUS_NANDIAR_ZONE] = { .filename = "nandiar-zone.mp3" },
  [AZ_MUS_VOQUAN_ZONE] = { .filename = "voquan-zone.ogg" },
  [AZ_MUS_BARRAG_ZONE] = { .filename = "barrag-zone.ogg" },
  [AZ_MUS_SARVARI_ZONE] = { .filename = "sarvari-zone.ogg" },
  [AZ_MUS_CORE_ZONE] = { .filename = "core-zone.ogg" },
  [AZ_MUS_ZENITH_CORE] = { .filename = "zenith-core.mp3" },
  [AZ_MUS_BOSS1] = { .filename = "boss1.ogg" },
  [AZ_MUS_BOSS2] = { .filename = "boss2.ogg" }
};

AZ_STATIC_ASSERT(AZ_ARRAY_SIZE(music_entries) == AZ_NUM_MUSIC_KEYS);

static void load_all_music(void) {
  const char *resource_dir = az_get_resource_directory();
  char path_buffer[strlen(resource_dir) + 48u];
  AZ_ARRAY_LOOP(entry, music_entries) {
    assert(entry->filename != NULL);
    assert(strlen(entry->filename) > 0);
    assert(strlen(entry->filename) <= 40);
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
  enum { SQUARE, SAWTOOTH, TRIANGLE, SINE, WOBBLE, NOISE } wave_kind;
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
  [AZ_SND_CHARGED_MISSILE_BEAM] = {
    .wave_kind = WOBBLE,
    .env_decay = 0.563380300999,
    .start_freq = 0.373239427805, .freq_slide = 0.1549295187,
    .vibrato_depth = 0.760563373566, .vibrato_speed = 0.66197180748,
    .phaser_offset = 0.718309879303, .phaser_sweep = 0.436619758606,
    .lpf_cutoff = 0.6768399, .hpf_cutoff = 0.036
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
    .env_attack = 0.2, .env_sustain = 1.0,
    .start_freq = 0.0897183, .freq_slide = 0.118591,
    .freq_delta_slide = 0.056338,
    .square_duty = 0.40368, .duty_sweep = 0.0140844583511,
    .volume_adjust = -0.4
  },
  [AZ_SND_CPLUS_ACTIVE] = {
    .wave_kind = NOISE,
    .env_sustain = 0.2827, .env_punch = 0.61646, .env_decay = 1.0,
    .start_freq = 0.443661957979, .freq_delta_slide = -0.25352114439,
    .vibrato_depth = 0.27069, .vibrato_speed = 0.00366,
    .arp_mod = 0.253521084785, .arp_speed = 0.65505,
    .repeat_speed = 0.732394337654,
    .phaser_offset = -0.04278001, .phaser_sweep = -0.112676084042
  },
  [AZ_SND_CPLUS_CHARGED] = {
    .wave_kind = WOBBLE,
    .env_sustain = 0.6126761, .env_punch = 0.08549776, .env_decay = 0.204225,
    .start_freq = 0.84507, .freq_slide = 0.00128, .freq_delta_slide = 0.11465,
    .vibrato_depth = 0.4195169, .vibrato_speed = -0.39,
    .arp_mod = 0.1044, .arp_speed = -0.5518,
    .square_duty = 0.189, .duty_sweep = 0.01924883,
    .phaser_offset = -0.4636848, .phaser_sweep = 0.02882149,
    .lpf_cutoff = 0.5151533, .lpf_ramp = 0.4815216, .lpf_resonance = 0.312,
    .hpf_cutoff = 0.6101004, .hpf_ramp = -0.01561149, .volume_adjust = -0.222
  },
  [AZ_SND_CPLUS_IMPACT] = {
    .wave_kind = NOISE,
    .env_sustain = 0.33175, .env_punch = 0.53942, .env_decay = 0.788732409477,
    .start_freq = 0.1128691, .freq_slide = 0.05820001,
    .phaser_offset = 0.47598, .phaser_sweep = -0.25746
  },
  [AZ_SND_CPLUS_READY] = {
    .wave_kind = TRIANGLE,
    .env_sustain = 0.6, .env_punch = 0.1355418,
    .start_freq = 0.5106531, .freq_delta_slide = 0.5334117,
    .vibrato_depth = -0.0002874958, .vibrato_speed = -0.161,
    .arp_mod = 0.9934, .arp_speed = -0.331,
    .square_duty = 0.8678, .duty_sweep = 0.000924009, .repeat_speed = 0.663,
    .phaser_offset = 0.0006630559, .phaser_sweep = -0.3438828,
    .lpf_cutoff = 0.4502442, .lpf_resonance = -0.4142,
    .hpf_cutoff = 0.0008809352, .hpf_ramp = -0.2542205, .volume_adjust = -0.5
  },
  [AZ_SND_DOOR_CLOSE] = {
    .wave_kind = WOBBLE,
    .env_sustain = 0.352112680674, .env_decay = 0.12622,
    .start_freq = 0.401408463717, .freq_slide = -0.197183,
    .square_duty = 0.53694, .volume_adjust = -0.5
  },
  [AZ_SND_DOOR_OPEN] = {
    .wave_kind = WOBBLE,
    .env_sustain = 0.352112680674, .env_decay = 0.12622,
    .start_freq = 0.246478870511, .freq_slide = 0.197183,
    .square_duty = 0.53694, .volume_adjust = -0.5
  },
  [AZ_SND_DRILLING] = {
    .wave_kind = NOISE,
    .env_sustain = 0.74, .start_freq = 0.1056338,
    .vibrato_depth = 0.640845, .vibrato_speed = 0.56338,
    .volume_adjust = -0.5
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
    .vibrato_depth = 0.281690150499, .vibrato_speed = 0.12924
  },
  [AZ_SND_FIRE_GUN_CHARGED_BEAM] = {
    .wave_kind = SAWTOOTH,
    .env_sustain = 0.36394, .env_decay = 0.274647891521,
    .start_freq = 0.1338, .freq_slide = 0.2676, .freq_delta_slide = -0.140845,
    .phaser_offset = -0.323943674564, .phaser_sweep = 0.436619758606
  },
  [AZ_SND_FIRE_GUN_FREEZE] = {
    .wave_kind = WOBBLE,
    .env_sustain = 0.13848, .env_punch = 0.12036, .env_decay = 0.21536,
    .start_freq = 0.9788732, .freq_limit = 0.32917, .freq_slide = -0.22554,
    .square_duty = 0.46895, .duty_sweep = 0.1842,
    .hpf_cutoff = 0.007470001
  },
  [AZ_SND_FIRE_GUN_NORMAL] = {
    .wave_kind = SINE,
    .env_sustain = 0.10346, .env_punch = 0.16836, .env_decay = 0.18956,
    .start_freq = 0.89034, .freq_limit = 0.08224, .freq_slide = -0.61922,
    .square_duty = 0.2347, .duty_sweep = 0.0176, .hpf_cutoff = 0.25965
  },
  [AZ_SND_FIRE_GUN_PIERCE] = {
    .wave_kind = SAWTOOTH,
    .env_sustain = 0.23076, .env_decay = 0.05284,
    .start_freq = 0.7278, .freq_limit = 0.29536, .freq_slide = -0.29848,
    .square_duty = 0.46265, .duty_sweep = -0.18305
  },
  [AZ_SND_FIRE_HYPER_ROCKET] = {
    .wave_kind = NOISE,
    .env_sustain = 0.3, .env_punch = 0.25, .env_decay = 0.5,
    .start_freq = 0.2, .vibrato_depth = 0.5, .vibrato_speed = 0.75,
    .repeat_speed = 0.55, .phaser_offset = 0.25, .phaser_sweep = -0.25
  },
  [AZ_SND_FIRE_LASER_PULSE] = {
    .wave_kind = SQUARE,
    .env_sustain = 0.11852, .env_punch = 0.132, .env_decay = 0.183098584414,
    .start_freq = 0.535211265087, .freq_limit = 0.2, .freq_slide = -0.26748,
    .square_duty = 0.18045, .duty_sweep = 0.18344
  },
  [AZ_SND_FIRE_MISSILE_BEAM] = {
    .wave_kind = SAWTOOTH,
    .env_sustain = 0.03084, .env_decay = 0.66197180748,
    .start_freq = 0.929577469826, .freq_slide = -0.0140845179558,
    .vibrato_depth = 0.704225361347, .vibrato_speed = 0.739436626434,
    .phaser_offset = 0.352112650871, .phaser_sweep = 0.549295783043,
    .volume_adjust = 0.4
  },
  [AZ_SND_FIRE_OTH_ROCKET] = {
    .wave_kind = NOISE,
    .env_sustain = 0.605633795261, .env_punch = 0.160833105445,
    .env_decay = 0.732394337654, .start_freq = 0.345070421696,
    .freq_slide = -0.00747528579086, .freq_delta_slide = -0.00656020781025,
    .arp_mod = 0.704225301743, .arp_speed = 0.732394337654,
    .phaser_offset = -0.16901409626, .phaser_sweep = -0.563380241394,
    .lpf_cutoff = 0.795774638653, .lpf_ramp = -0.126760542393,
    .lpf_resonance = 0.718309879303, .volume_adjust = 0.6
  },
  [AZ_SND_FIRE_OTH_SPRAY] = {
    .wave_kind = SQUARE,
    .env_attack = 0.0563380271196, .env_sustain = 0.309859156609,
    .env_punch = 0.0774647891521, .env_decay = 0.598591566086,
    .start_freq = 0.387323945761, .freq_slide = 0.0563380718231,
    .freq_delta_slide = -0.08450704813,
    .vibrato_depth = 0.0774647891521, .vibrato_speed = 0.654929578304,
    .arp_mod = -0.140845060349,
    .square_duty = 0.570422530174, .duty_sweep = -0.281690120697,
    .repeat_speed = 0.612676084042, .phaser_sweep = 0.774647831917,
    .lpf_cutoff = 0.718309879303, .lpf_ramp = -0.0098278503865,
    .lpf_resonance = 0.366197168827
  },
  [AZ_SND_FIRE_ROCKET] = {
    .wave_kind = NOISE,
    .env_sustain = 0.39907, .env_punch = 0.65582, .env_decay = 0.39085,
    .start_freq = 0.7881, .freq_slide = -0.30088,
    .vibrato_depth = 0.59703, .vibrato_speed = 0.03828,
    .phaser_offset = -0.09300001, .phaser_sweep = -0.19305,
    .volume_adjust = -0.42592590332
  },
  [AZ_SND_FIRE_STINGER] = {
    .wave_kind = NOISE,
    .env_sustain = 0.2112676, .env_punch = 0.2112676, .env_decay = 0.28873238,
    .start_freq = 0.3653719, .freq_slide = 0.49295771122
  },
  [AZ_SND_HEAT_DAMAGE] = {
    .wave_kind = NOISE,
    .env_sustain = 0.605633795261, .freq_slide = 0.591549277306,
    .freq_delta_slide = -0.943661987782, .repeat_speed = 0.0352112688124,
    .phaser_offset = 0.309859156609, .phaser_sweep = -0.33802819252,
    .hpf_cutoff = 0.133802816272, .volume_adjust = -0.2
  },
  [AZ_SND_HIT_WALL] = {
    .wave_kind = NOISE,
    .env_sustain = 0.03245, .env_decay = 0.10518,
    .start_freq = 0.43334, .freq_slide = -0.57352
  },
  [AZ_SND_KILL_ATOM] = {
    .wave_kind = SQUARE,
    .env_sustain = 0.211267605424, .env_punch = 0.405718,
    .env_decay = 0.605633795261, .start_freq = 0.7841117,
    .freq_slide = -0.236357, .freq_delta_slide = -0.9898345,
    .vibrato_depth = 0.04728087, .vibrato_speed = -0.4434,
    .arp_mod = 0.6122, .arp_speed = 0.2002,
    .square_duty = -0.3186, .duty_sweep = -0.4922905,
    .repeat_speed = 0.4604,
    .phaser_offset = 0.6059253, .phaser_sweep = -0.3536932,
    .lpf_cutoff = 0.982626, .lpf_ramp = 0.2353423, .lpf_resonance = 0.8618,
    .volume_adjust = 0.8
  },
  [AZ_SND_KILL_DRAGONFLY] = {
    .wave_kind = SQUARE,
    .env_attack = 0.03548157, .env_sustain = 0.378881,
    .env_punch = 0.3252649, .env_decay = -0.36613,
    .start_freq = 0.05789758,
    .freq_slide = 0.5313399, .freq_delta_slide = -0.1058238,
    .vibrato_depth = 0.9644304, .vibrato_speed = 0.9172,
    .arp_mod = 0.00940001, .arp_speed = 0.281,
    .square_duty = -0.1706, .duty_sweep = 0.585956,
    .repeat_speed = 0.8049999,
    .phaser_offset = -0.5914348, .phaser_sweep = -0.8174004,
    .lpf_cutoff = 0.8984792, .lpf_ramp = 0.3468362, .lpf_resonance = -0.007,
    .hpf_cutoff = 0.1794088, .hpf_ramp = -0.1530325,
    .volume_adjust = 0.3
  },
  [AZ_SND_KILL_TURRET] = {
    .wave_kind = NOISE,
    .env_sustain = 0.10687, .env_punch = 0.24554, .env_decay = 0.463,
    .start_freq = 0.0972442, .freq_slide = 0.05804, .repeat_speed = 0.56475,
  },
  [AZ_SND_KLAXON] = {
    .wave_kind = SQUARE,
    .env_sustain = 0.4507, .start_freq = 0.32394,
    .freq_slide = 0.112676, .freq_delta_slide = -0.197183,
    .square_duty = 0.873239, .phaser_offset = 0.40845,
    .volume_adjust = -0.24
  },
  [AZ_SND_KLAXON_DIRE] = {
    .wave_kind = SQUARE,
    .env_sustain = 0.4507, .start_freq = 0.32394,
    .freq_slide = 0.212676, .freq_delta_slide = -0.197183,
    .square_duty = 0.873239, .phaser_offset = 0.40845,
    .volume_adjust = -0.19
  },
  [AZ_SND_LAUNCH_OTH_RAZORS] = {
    .wave_kind = NOISE,
    .env_attack = -0.0173337701708, .env_sustain = 0.450704216957,
    .env_punch = 0.0839145034552, .env_decay = 0.133802816272,
    .start_freq = 0.211267605424, .freq_delta_slide = -0.464788734913,
    .vibrato_speed = 0.725352108479,
    .arp_mod = -0.0140845179558, .arp_speed = -0.966199994087,
    .repeat_speed = 0.58450704813, .phaser_sweep = 0.563380241394,
    .lpf_cutoff = 0.781690120697, .lpf_ramp = -0.464788734913,
    .lpf_resonance = 0.274647891521
  },
  [AZ_SND_METAL_CLINK] = {
    .wave_kind = SAWTOOTH,
    .env_decay = 0.260563373566, .start_freq = 0.407042229176,
    .phaser_offset = -0.197183, .lpf_cutoff = 0.2253521, .lpf_resonance = 1.0
  },
  [AZ_SND_ORION_BOOSTER] = {
    .wave_kind = NOISE,
    .env_sustain = 0.338028, .env_punch = 0.4225352, .env_decay = 0.6,
    .start_freq = 0.190140843391, .freq_slide = 0.19992,
    .vibrato_depth = 0.39781, .vibrato_speed = 0.06258,
    .phaser_offset = 0.746478915215, .phaser_sweep = -0.0704225301743
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
  },
  [AZ_SND_SONIC_SCREECH] = {
    .wave_kind = SAWTOOTH,
    .env_sustain = 0.24708, .env_punch = 0.1941, .env_decay = 0.36444,
    .start_freq = 0.6959, .freq_limit = 0.28848,
    .freq_slide = -0.29102, .freq_delta_slide = 0.352112650871,
    .vibrato_depth = 0.316901415586, .vibrato_speed = 0.563380300999,
    .phaser_offset = 0.521126747131, .phaser_sweep = -0.239436626434,
    .hpf_cutoff = 0.394366204739
  },
  [AZ_SND_SPLASH] = {
    .wave_kind = NOISE,
    .env_attack = 0.302816897631, .env_decay = 0.464788734913,
    .start_freq = 0.84507, .freq_slide = 0.422535, .freq_delta_slide = -0.0986,
    .phaser_sweep = 1.0, .hpf_cutoff = 0.5
  },
  [AZ_SND_TRACTOR_BEAM] = {
    .wave_kind = WOBBLE, .env_sustain = 1.0, .start_freq = 0.18,
    .vibrato_depth = 0.2, .vibrato_speed = 0.3, .volume_adjust = -0.5
  }
};

AZ_STATIC_ASSERT(AZ_ARRAY_SIZE(sound_entries) == AZ_NUM_SOUND_KEYS);

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
        case WOBBLE:
          sample = (float)(0.5 * (cos(fp * AZ_TWO_PI) +
                                  sin(2.0 * fp * AZ_TWO_PI)));
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
    enum { INACTIVE = 0, PLAYING, PAUSED, FINISHED } status;
    az_sound_key_t sound;
    int channel;
  } persisting_sounds[NUM_MIXER_CHANNELS];

  // First, go through each of the active persisting_sounds and update their
  // status based on the soundboard.
  bool already_active[soundboard->num_persists];
  memset(already_active, 0, sizeof(bool) * soundboard->num_persists);
  AZ_ARRAY_LOOP(persisting, persisting_sounds) {
    if (persisting->status == INACTIVE) continue;
    // Check if this sound is in the soundboard's persists array.  If not, we
    // will implicitly halt and reset the sound.
    bool reset = true;
    for (int i = 0; i < soundboard->num_persists; ++i) {
      if (soundboard->persists[i].sound == persisting->sound) {
        // We only need to worry about play/pause right now if we're not
        // resetting the sound.  If reset and play are both true, we'll still
        // halt the sound here, and then start it again below.
        if (!soundboard->persists[i].reset) {
          reset = false;
          already_active[i] = true;
          // Resume or pause the sound as needed.
          if (soundboard->persists[i].play) {
            if (persisting->status == PAUSED) {
              Mix_Resume(persisting->channel);
              persisting->status = PLAYING;
            }
          } else {
            if (persisting->status == PLAYING) {
              Mix_Pause(persisting->channel);
              persisting->status = PAUSED;
            }
          }
        }
        break;
      }
    }
    // If the sound has finished playing on its own (meaning its channel is
    // available for use by another sound), mark it as FINISHED so we'll know
    // not to mess with that channel later if we need to halt/pause this sound.
    if (persisting->status == PLAYING && !Mix_Playing(persisting->channel)) {
      persisting->status = FINISHED;
    }
    // If we're supposed to reset this sound, halt it.
    if (reset) {
      if (persisting->status == PLAYING || persisting->status == PAUSED) {
        Mix_HaltChannel(persisting->channel);
      }
      persisting->status = INACTIVE;
    }
  }

  // Second, go through the soundboard and start playing any new persistent
  // sounds that need to be started.
  for (int i = 0; i < soundboard->num_persists; ++i) {
    if (already_active[i] || !soundboard->persists[i].play) continue;
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
        } else {
          AZ_WARNING_ONCE("Could not play persistent sound (%d)\n",
                          (int)sound);
        }
        break;
      }
    }
  }

  // Third, start playing any new one-shot sounds.
  for (int i = 0; i < soundboard->num_oneshots; ++i) {
    const az_sound_key_t sound = soundboard->oneshots[i];
    const int channel = Mix_PlayChannel(-1, sound_entries[sound].chunk, 0);
    if (channel < 0) {
      AZ_WARNING_ONCE("Could not play sound effect (%d)\n", (int)sound);
    }
  }
}

/*===========================================================================*/
// Audio system:

static bool audio_mixer_initialized = false;
static bool audio_mixer_paused = false;

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

static int to_sdl_volume(float volume) {
  return az_imin(az_imax(0, (int)(volume * (float)MIX_MAX_VOLUME)),
                 MIX_MAX_VOLUME);
}

void az_set_global_music_volume(float volume) {
  assert(audio_mixer_initialized);
  assert(!audio_mixer_paused);
  Mix_VolumeMusic(to_sdl_volume(volume));
}

void az_set_global_sound_volume(float volume) {
  assert(audio_mixer_initialized);
  assert(!audio_mixer_paused);
  Mix_Volume(-1, to_sdl_volume(volume));
}

void az_tick_audio_mixer(az_soundboard_t *soundboard) {
  assert(audio_mixer_initialized);
  assert(!audio_mixer_paused);
  tick_music(soundboard);
  tick_sounds(soundboard);
  AZ_ZERO_OBJECT(soundboard);
}

void az_pause_all_audio(void) {
  if (!audio_mixer_initialized) return;
  assert(!audio_mixer_paused);
  Mix_Pause(-1); // pause all (non-music) channels
  Mix_PauseMusic();
  audio_mixer_paused = true;
}

void az_unpause_all_audio(void) {
  if (!audio_mixer_initialized) return;
  assert(audio_mixer_paused);
  Mix_Resume(-1); // unpause all (non-music) channels
  Mix_ResumeMusic();
  audio_mixer_paused = false;
}

/*===========================================================================*/
