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
#ifndef AZIMUTH_UTIL_AUDIO_H_
#define AZIMUTH_UTIL_AUDIO_H_

#include <stdbool.h>

/*===========================================================================*/

typedef enum {
  AZ_MUS_TITLE,
  AZ_MUS_CNIDAM_ZONE,
  AZ_MUS_ZENITH_CORE
} az_music_key_t;

typedef enum {
  AZ_SND_BEAM_FREEZE,
  AZ_SND_BEAM_NORMAL,
  AZ_SND_BEAM_PHASE,
  AZ_SND_BEAM_PIERCE,
  AZ_SND_BLINK_MEGA_BOMB,
  AZ_SND_CHARGED_GUN,
  AZ_SND_CHARGING_GUN,
  AZ_SND_DOOR_CLOSE,
  AZ_SND_DOOR_OPEN,
  AZ_SND_DROP_BOMB,
  AZ_SND_EXPLODE_BOMB,
  AZ_SND_EXPLODE_HYPER_ROCKET,
  AZ_SND_EXPLODE_MEGA_BOMB,
  AZ_SND_EXPLODE_ROCKET,
  AZ_SND_EXPLODE_SHIP,
  AZ_SND_FIRE_GUN_FREEZE,
  AZ_SND_FIRE_GUN_NORMAL,
  AZ_SND_FIRE_GUN_PIERCE,
  AZ_SND_FIRE_ROCKET,
  AZ_SND_FIRE_HYPER_ROCKET,
  AZ_SND_HIT_WALL,
  AZ_SND_PICKUP_ORDNANCE,
  AZ_SND_PICKUP_SHIELDS
} az_sound_key_t;

// A soundboard keeps track of what sounds/music we want to play next.  It will
// be periodically read and flushed by our audio system.  To initialize it,
// simply zero it with a memset.  One should not manipulate the fields of this
// struct directly; instead, use the various functions below.
typedef struct {
  enum { AZ_MUSA_NOTHING = 0, AZ_MUSA_CHANGE, AZ_MUSA_STOP } music_action;
  az_music_key_t next_music;
  int music_fade_out_millis;
  int num_oneshots;
  az_sound_key_t oneshots[10];
  int num_persists;
  struct { az_sound_key_t sound; bool loop; } persists[10];
} az_soundboard_t;

/*===========================================================================*/

// Indicate that we would like to change which music is playing.
void az_change_music(az_soundboard_t *soundboard, az_music_key_t music);

// Indicate that we would like to stop the current music (without playing
// something else next), by fading out for the given amount of time.
void az_stop_music(az_soundboard_t *soundboard, double fade_out_seconds);

// Indicate that we should play the given sound (once).  The sound will not
// loop, and cannot be cancelled once started.
void az_play_sound(az_soundboard_t *soundboard, az_sound_key_t sound);

// Indicate that we should start playing, or continue to play, the given sound.
// To keep the sound going, we must call this function every frame with the
// same sound, otherwise the sound will stop.  As long as we keep calling this
// function, the sound will continue to loop.
void az_loop_sound(az_soundboard_t *soundboard, az_sound_key_t sound);

// Indicate that we should start playing, or continue to play, the given sound.
// To keep the sound going, we must call this function every frame with the
// same sound, otherwise the sound will stop.  The sound will play only once,
// and won't restart until we stop calling this function for at least one frame
// before calling it again.
void az_persist_sound(az_soundboard_t *soundboard, az_sound_key_t sound);

/*===========================================================================*/

#endif // AZIMUTH_UTIL_AUDIO_H_
