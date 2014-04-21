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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "azimuth/util/audio.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/music.h"
#include "azimuth/util/warning.h"

/*===========================================================================*/

static const char *music_filenames[] = {
  [AZ_MUS_COLONY_ZONE] = "music01.txt",
  [AZ_MUS_FILIMUN_ZONE] = NULL,
  [AZ_MUS_CNIDAM_ZONE] = NULL,
  [AZ_MUS_NANDIAR_ZONE] = NULL,
  [AZ_MUS_VOQUAN_ZONE] = NULL,
  [AZ_MUS_BARRAG_ZONE] = NULL,
  [AZ_MUS_SARVARI_ZONE] = NULL,
  [AZ_MUS_CORE_ZONE] = NULL,
  [AZ_MUS_ZENITH_CORE] = NULL,
  [AZ_MUS_BOSS1] = NULL,
  [AZ_MUS_BOSS2] = NULL,
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
  music_data_initialized = true;
  atexit(destroy_music_datas);
  const size_t dirlen = strlen(resource_dir);
  char path_buffer[dirlen + 30u];
  for (int i = 0; i < AZ_ARRAY_SIZE(music_filenames); ++i) {
    const char *filename = music_filenames[i];
    if (filename == NULL) continue;
    assert(strlen(filename) <= 20u);
    sprintf(path_buffer, "%s/music/%s", resource_dir, filename);
    if (!az_parse_music_from_file(path_buffer, &music_datas[i])) {
      AZ_WARNING_ALWAYS("Failed to load music from %s\n", path_buffer);
      return false;
    }
  }
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
