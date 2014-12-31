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
#ifndef AZIMUTH_STATE_MUSIC_H_
#define AZIMUTH_STATE_MUSIC_H_

#include "azimuth/util/audio.h"
#include "azimuth/util/music.h"

/*===========================================================================*/

// The number of different music keys there are, not counting AZ_MUS_NOTHING:
#define AZ_NUM_MUSIC_KEYS 17

typedef enum {
  AZ_MUS_NOTHING = 0,
  AZ_MUS_COLONY_1,
  AZ_MUS_FILIMUN_ZONE,
  AZ_MUS_CNIDAM_ZONE,
  AZ_MUS_NANDIAR_ZONE,
  AZ_MUS_VOQUAN_ZONE,
  AZ_MUS_BARRAG_ZONE,
  AZ_MUS_SARVARI_ZONE,
  AZ_MUS_CORE_ZONE,
  AZ_MUS_ZENITH_CORE,
  AZ_MUS_BOSS_1,
  AZ_MUS_BOSS_2,
  AZ_MUS_SUSPENSE,
  AZ_MUS_ESCAPE,
  AZ_MUS_UPGRADE,
  AZ_MUS_COLONY_2,
  AZ_MUS_BOSS_3,
  AZ_MUS_TITLE
} az_music_key_t;

/*===========================================================================*/

void az_get_drum_kit(int *num_drums_out, const az_sound_data_t **drums_out);

bool az_init_music_datas(const char *resource_dir);

// Indicate that we would like to change which music is playing.
void az_change_music(az_soundboard_t *soundboard, az_music_key_t music_key);

// Indicate that we would like to stop the current music (without playing
// something else next), by fading out for the given amount of time.
void az_stop_music(az_soundboard_t *soundboard, double fade_out_seconds);

/*===========================================================================*/

#endif // AZIMUTH_STATE_MUSIC_H_
