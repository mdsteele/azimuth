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

#include "azimuth/state/music.h"

#include <assert.h>
#include <stdlib.h>

#include "azimuth/util/audio.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/music.h"
#include "azimuth/util/string.h"
#include "azimuth/util/warning.h"

/*===========================================================================*/

static bool drums_initialized = false;

static const az_sound_spec_t drum_specs[] = {
  // Base drum:
  [0] = {
    .wave_kind = AZ_WOBBLE_WAVE,
    .env_sustain = 0.163306, .env_punch = 0.315, .env_decay = 0.295,
    .start_freq = 0.175, .freq_delta_slide = -0.713252,
    .arp_mod = -0.548859, .arp_speed = 0.160983,
    .repeat_speed = 0.555, .phaser_sweep = -0.35, .volume_adjust = 0.76
  },
  // Alternate base drum:
  [1] = {
    .wave_kind = AZ_WOBBLE_WAVE,
    .env_sustain = 0.163306, .env_punch = 0.9, .env_decay = 0.295,
    .start_freq = 0.18, .freq_delta_slide = -0.713252,
    .arp_mod = -0.548859, .arp_speed = 0.160983,
    .repeat_speed = 0.65, .phaser_sweep = -0.05, .volume_adjust = 0.76
  },
  // Clap:
  [2] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_decay = 0.31, .start_freq = 0.24, .freq_slide = 0.25,
    .phaser_offset = 0.17, .volume_adjust = -0.25
  },
  // Tom-tom 1:
  [3] = {
    .wave_kind = AZ_SINE_WAVE,
    .env_sustain = 0.06, .env_punch = 1.0, .env_decay = 0.375,
    .start_freq = 0.175, .freq_slide = -0.17, .freq_delta_slide = 0.05,
    .volume_adjust = 0.5
  },
  // Tom-tom 2:
  [4] = {
    .wave_kind = AZ_SINE_WAVE,
    .env_sustain = 0.06, .env_punch = 1.0, .env_decay = 0.375,
    .start_freq = 0.2, .freq_slide = -0.17, .freq_delta_slide = 0.05,
    .volume_adjust = 0.5
  },
  // Tom-tom 3:
  [5] = {
    .wave_kind = AZ_SINE_WAVE,
    .env_sustain = 0.06, .env_punch = 1.0, .env_decay = 0.375,
    .start_freq = 0.230, .freq_slide = -0.17, .freq_delta_slide = 0.05,
    .volume_adjust = 0.5
  },
  // Tom-tom 4:
  [6] = {
    .wave_kind = AZ_SINE_WAVE,
    .env_sustain = 0.06, .env_punch = 1.0, .env_decay = 0.375,
    .start_freq = 0.255, .freq_slide = -0.17, .freq_delta_slide = 0.05,
    .volume_adjust = 0.5
  }
};

static az_sound_data_t drum_datas[AZ_ARRAY_SIZE(drum_specs)];

static void destroy_drums(void) {
  AZ_ARRAY_LOOP(data, drum_datas) az_destroy_sound_data(data);
}

void az_get_drum_kit(int *num_drums_out, const az_sound_data_t **drums_out) {
  if (!drums_initialized) {
    for (int i = 0; i < AZ_ARRAY_SIZE(drum_specs); ++i) {
      az_create_sound_data(&drum_specs[i], &drum_datas[i]);
    }
    atexit(destroy_drums);
    drums_initialized = true;
  }
  *num_drums_out = AZ_ARRAY_SIZE(drum_datas);
  *drums_out = drum_datas;
}

/*===========================================================================*/

static const char *music_filenames[] = {
  [AZ_MUS_COLONY_ZONE] = "music01.txt",
  [AZ_MUS_FILIMUN_ZONE] = NULL,
  [AZ_MUS_CNIDAM_ZONE] = NULL,
  [AZ_MUS_NANDIAR_ZONE] = "music04.txt",
  [AZ_MUS_VOQUAN_ZONE] = NULL,
  [AZ_MUS_BARRAG_ZONE] = "music06.txt",
  [AZ_MUS_SARVARI_ZONE] = NULL,
  [AZ_MUS_CORE_ZONE] = "music08.txt",
  [AZ_MUS_ZENITH_CORE] = NULL,
  [AZ_MUS_BOSS1] = "music10.txt",
  [AZ_MUS_BOSS2] = "music11.txt",
  [AZ_MUS_SUSPENSE] = "music12.txt",
  [AZ_MUS_TITLE] = NULL
};

AZ_STATIC_ASSERT(AZ_ARRAY_SIZE(music_filenames) == AZ_NUM_MUSIC_KEYS + 1);

static az_music_t music_datas[AZ_ARRAY_SIZE(music_filenames)];

static bool music_data_initialized = false;

static void destroy_music_datas(void) {
  AZ_ARRAY_LOOP(music, music_datas) az_destroy_music(music);
}

bool az_init_music_datas(const char *resource_dir) {
  assert(!music_data_initialized);
  assert(resource_dir != NULL);
  // Initialize drum kit:
  int num_drums = 0;
  const az_sound_data_t *drums = NULL;
  az_get_drum_kit(&num_drums, &drums);
  // Initialize music:
  for (int i = 0; i < AZ_ARRAY_SIZE(music_filenames); ++i) {
    const char *filename = music_filenames[i];
    if (filename == NULL) continue;
    char *music_path = az_strprintf("%s/music/%s", resource_dir, filename);
    if (!az_parse_music_from_file(music_path, num_drums, drums,
                                  &music_datas[i])) {
      AZ_WARNING_ALWAYS("Failed to load music from %s\n", music_path);
      free(music_path);
      destroy_music_datas();
      return false;
    } else free(music_path);
  }
  atexit(destroy_music_datas);
  music_data_initialized = true;
  return true;
}

static const az_music_t *music_data_for_key(az_music_key_t music_key) {
  assert(music_data_initialized);
  const int music_index = (int)music_key;
  assert(music_index >= 0);
  assert(music_index < AZ_ARRAY_SIZE(music_datas));
  const az_music_t *music = &music_datas[music_index];
  if (music->spec_length == 0) return NULL;
  return music;
}

/*===========================================================================*/

void az_change_music(az_soundboard_t *soundboard, az_music_key_t music_key) {
  az_change_music_data(soundboard, music_data_for_key(music_key), 0.5);
}

void az_stop_music(az_soundboard_t *soundboard, double fade_out_seconds) {
  az_change_music_data(soundboard, NULL, fade_out_seconds);
}

/*===========================================================================*/
