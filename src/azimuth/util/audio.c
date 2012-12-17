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

/*===========================================================================*/
