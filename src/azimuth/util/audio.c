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

#include "azimuth/util/audio.h"

#include <assert.h>
#include <stdbool.h>

#include "azimuth/util/misc.h"
#include "azimuth/util/warning.h"

/*===========================================================================*/

void az_change_music_data(az_soundboard_t *soundboard, const az_music_t *music,
                          double fade_out_seconds) {
  soundboard->change_music = true;
  soundboard->next_music = music;
  soundboard->music_fade_out_seconds = fade_out_seconds;
}

void az_change_music_flag(az_soundboard_t *soundboard, int flag) {
  if (soundboard->change_music) {
    soundboard->change_next_music_flag = true;
    soundboard->new_next_music_flag = flag;
  } else {
    soundboard->change_current_music_flag = true;
    soundboard->new_current_music_flag = flag;
  }
}

void az_play_sound_data(az_soundboard_t *soundboard,
                        const az_sound_data_t *sound_data) {
  if (sound_data == NULL) return;
  if (soundboard->num_oneshots < AZ_ARRAY_SIZE(soundboard->oneshots)) {
    // Don't start the same sound more than once in the same frame.
    for (int i = 0; i < soundboard->num_oneshots; ++i) {
      if (soundboard->oneshots[i] == sound_data) return;
    }
    soundboard->oneshots[soundboard->num_oneshots] = sound_data;
    ++soundboard->num_oneshots;
  }
}

static void persist_sound_internal(
    az_soundboard_t *soundboard, const az_sound_data_t *sound_data,
    bool play, bool loop, bool reset) {
  if (sound_data == NULL) return;
  int index;
  for (index = 0; index < soundboard->num_persists; ++index) {
    if (soundboard->persists[index].sound_data == sound_data) goto merge;
  }
  if (soundboard->num_persists == AZ_ARRAY_SIZE(soundboard->persists)) {
    AZ_WARNING_ONCE("No room to persist sound\n");
    return;
  }
  assert(index == soundboard->num_persists);
  assert(soundboard->num_persists < AZ_ARRAY_SIZE(soundboard->persists));
  ++soundboard->num_persists;
  soundboard->persists[index].sound_data = sound_data;
 merge:
  assert(soundboard->persists[index].sound_data == sound_data);
  soundboard->persists[index].play |= play;
  soundboard->persists[index].loop |= loop;
  soundboard->persists[index].reset |= reset;
}

void az_loop_sound_data(az_soundboard_t *soundboard,
                        const az_sound_data_t *sound_data) {
  persist_sound_internal(soundboard, sound_data, true, true, false);
}

void az_persist_sound_data(az_soundboard_t *soundboard,
                           const az_sound_data_t *sound_data) {
  persist_sound_internal(soundboard, sound_data, true, false, false);
}

void az_hold_sound_data(az_soundboard_t *soundboard,
                        const az_sound_data_t *sound_data) {
  persist_sound_internal(soundboard, sound_data, false, false, false);
}

void az_reset_sound_data(az_soundboard_t *soundboard,
                         const az_sound_data_t *sound_data) {
  persist_sound_internal(soundboard, sound_data, false, false, true);
}

/*===========================================================================*/
