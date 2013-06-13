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

void az_change_music(az_soundboard_t *soundboard, az_music_key_t music) {
  soundboard->music_action = AZ_MUSA_CHANGE;
  soundboard->next_music = music;
}

void az_stop_music(az_soundboard_t *soundboard, double fade_out_seconds) {
  assert(fade_out_seconds >= 0.0);
  soundboard->music_action = AZ_MUSA_STOP;
  soundboard->music_fade_out_millis = (int)(1000.0 * fade_out_seconds);
}

void az_play_sound(az_soundboard_t *soundboard, az_sound_key_t sound) {
  if (soundboard->num_oneshots < AZ_ARRAY_SIZE(soundboard->oneshots)) {
    // Don't start the same sound more than once in the same frame.
    for (int i = 0; i < soundboard->num_oneshots; ++i) {
      if (soundboard->oneshots[i] == sound) return;
    }
    soundboard->oneshots[soundboard->num_oneshots] = sound;
    ++soundboard->num_oneshots;
  }
}

static void persist_sound_internal(
    az_soundboard_t *soundboard, az_sound_key_t sound,
    bool play, bool loop, bool reset) {
  int index;
  for (index = 0; index < soundboard->num_persists; ++index) {
    if (soundboard->persists[index].sound == sound) goto merge;
  }
  if (soundboard->num_persists == AZ_ARRAY_SIZE(soundboard->persists)) {
    AZ_WARNING_ONCE("No room to persist sound %d\n", (int)sound);
    return;
  }
  assert(index == soundboard->num_persists);
  assert(soundboard->num_persists < AZ_ARRAY_SIZE(soundboard->persists));
  ++soundboard->num_persists;
  soundboard->persists[index].sound = sound;
 merge:
  assert(soundboard->persists[index].sound == sound);
  soundboard->persists[index].play |= play;
  soundboard->persists[index].loop |= loop;
  soundboard->persists[index].reset |= reset;
}

void az_loop_sound(az_soundboard_t *soundboard, az_sound_key_t sound) {
  persist_sound_internal(soundboard, sound, true, true, false);
}

void az_persist_sound(az_soundboard_t *soundboard, az_sound_key_t sound) {
  persist_sound_internal(soundboard, sound, true, false, false);
}

void az_hold_sound(az_soundboard_t *soundboard, az_sound_key_t sound) {
  persist_sound_internal(soundboard, sound, false, false, false);
}

void az_reset_sound(az_soundboard_t *soundboard, az_sound_key_t sound) {
  persist_sound_internal(soundboard, sound, false, false, true);
}

/*===========================================================================*/
