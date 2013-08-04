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

// The number of different music keys there are:
#define AZ_NUM_MUSIC_KEYS 12

typedef enum {
  AZ_MUS_TITLE,
  AZ_MUS_COLONY_ZONE,
  AZ_MUS_FILIMUN_ZONE,
  AZ_MUS_CNIDAM_ZONE,
  AZ_MUS_NANDIAR_ZONE,
  AZ_MUS_VOQUAN_ZONE,
  AZ_MUS_BARRAG_ZONE,
  AZ_MUS_SARVARI_ZONE,
  AZ_MUS_CORE_ZONE,
  AZ_MUS_ZENITH_CORE,
  AZ_MUS_BOSS1,
  AZ_MUS_BOSS2
} az_music_key_t;

// The number of different sound keys there are:
#define AZ_NUM_SOUND_KEYS 48

typedef enum {
  // Sounds used in scripts:
  AZ_SND_DRILLING = 0,
  // Sounds not used in scripts:
  AZ_SND_BEAM_FREEZE,
  AZ_SND_BEAM_NORMAL,
  AZ_SND_BEAM_PHASE,
  AZ_SND_BEAM_PIERCE,
  AZ_SND_BLINK_MEGA_BOMB,
  AZ_SND_CHARGED_GUN,
  AZ_SND_CHARGED_MISSILE_BEAM,
  AZ_SND_CHARGED_ORDNANCE,
  AZ_SND_CHARGING_GUN,
  AZ_SND_CHARGING_ORDNANCE,
  AZ_SND_CPLUS_ACTIVE,
  AZ_SND_CPLUS_CHARGED,
  AZ_SND_CPLUS_IMPACT,
  AZ_SND_CPLUS_READY,
  AZ_SND_DOOR_CLOSE,
  AZ_SND_DOOR_OPEN,
  AZ_SND_DROP_BOMB,
  AZ_SND_EXPLODE_BOMB,
  AZ_SND_EXPLODE_HYPER_ROCKET,
  AZ_SND_EXPLODE_MEGA_BOMB,
  AZ_SND_EXPLODE_ROCKET,
  AZ_SND_EXPLODE_SHIP,
  AZ_SND_FIRE_GUN_CHARGED_BEAM,
  AZ_SND_FIRE_GUN_FREEZE,
  AZ_SND_FIRE_GUN_NORMAL,
  AZ_SND_FIRE_GUN_PIERCE,
  AZ_SND_FIRE_HYPER_ROCKET,
  AZ_SND_FIRE_LASER_PULSE,
  AZ_SND_FIRE_MISSILE_BEAM,
  AZ_SND_FIRE_OTH_ROCKET,
  AZ_SND_FIRE_OTH_SPRAY,
  AZ_SND_FIRE_ROCKET,
  AZ_SND_FIRE_STINGER,
  AZ_SND_HEAT_DAMAGE,
  AZ_SND_HIT_WALL,
  AZ_SND_KILL_ATOM,
  AZ_SND_KILL_DRAGONFLY,
  AZ_SND_KILL_TURRET,
  AZ_SND_KLAXON,
  AZ_SND_KLAXON_DIRE,
  AZ_SND_LAUNCH_OTH_RAZORS,
  AZ_SND_METAL_CLINK,
  AZ_SND_ORION_BOOSTER,
  AZ_SND_PICKUP_ORDNANCE,
  AZ_SND_PICKUP_SHIELDS,
  AZ_SND_SPLASH,
  AZ_SND_TRACTOR_BEAM
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
  struct {
    az_sound_key_t sound;
    bool play;
    bool loop;
    bool reset;
  } persists[10];
} az_soundboard_t;

/*===========================================================================*/

// Indicate that we would like to change which music is playing.
void az_change_music(az_soundboard_t *soundboard, az_music_key_t music);

// Indicate that we would like to stop the current music (without playing
// something else next), by fading out for the given amount of time.
void az_stop_music(az_soundboard_t *soundboard, double fade_out_seconds);

// Indicate that we should play the given sound (once).  The sound will not
// loop, and cannot be cancelled or paused once started.
void az_play_sound(az_soundboard_t *soundboard, az_sound_key_t sound);

// Indicate that we should start playing, or continue to play, the given sound.
// To keep the sound going, we must call this function every frame with the
// same sound, otherwise the sound will stop.  As long as we keep calling this
// function, the sound will continue to loop.
void az_loop_sound(az_soundboard_t *soundboard, az_sound_key_t sound);

// Indicate that we should start playing, or continue to play, the given sound.
// To keep the sound going, we must call this function every frame with the
// same sound, otherwise the sound will stop.  The sound will play only once,
// and won't restart until we either stop calling this function for at least
// one frame before calling it again, or we call az_reset_sound.
void az_persist_sound(az_soundboard_t *soundboard, az_sound_key_t sound);

// Indicate that, if the given persisted or looped sound is currently playing,
// we should pause it for this frame.  To keep the sound from resetting, we
// must call this function every frame with the same sound; the sound will
// resume once we start calling az_persist_sound or az_loop_sound again.
void az_hold_sound(az_soundboard_t *soundboard, az_sound_key_t sound);

// Indicate that the given persisted or looped sound should restart from the
// beginning, even if we also call az_persist_sound or az_loop_sound this frame
// to keep the sound going.
void az_reset_sound(az_soundboard_t *soundboard, az_sound_key_t sound);

/*===========================================================================*/

#endif // AZIMUTH_UTIL_AUDIO_H_
