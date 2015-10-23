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
#ifndef AZIMUTH_STATE_SOUND_H_
#define AZIMUTH_STATE_SOUND_H_

#include "azimuth/util/audio.h"
#include "azimuth/util/sound.h"

/*===========================================================================*/

// The number of different sound keys there are, not counting AZ_SND_NOTHING:
#define AZ_NUM_SOUND_KEYS 117

typedef enum {
  AZ_SND_NOTHING = 0,
  // Sounds used in scripts:
  AZ_SND_DRILLING,
  AZ_SND_SWITCH_ACTIVATE,
  AZ_SND_SWITCH_CONFIRM,
  AZ_SND_ALARM,
  AZ_SND_MINOR_UPGRADE,
  AZ_SND_LIGHTS_FLICKER,
  AZ_SND_LIGHTS_ON,
  AZ_SND_GLASS_BREAK,
  // Sounds not used in scripts:
  AZ_SND_BEAM_FREEZE,
  AZ_SND_BEAM_NORMAL,
  AZ_SND_BEAM_PHASE,
  AZ_SND_BEAM_PIERCE,
  AZ_SND_BLINK_MEGA_BOMB,
  AZ_SND_BOSS_EXPLODE,
  AZ_SND_BOSS_SHAKE,
  AZ_SND_BOUNCE_FIREBALL,
  AZ_SND_CHARGED_GUN,
  AZ_SND_CHARGED_MISSILE_BEAM,
  AZ_SND_CHARGED_ORDNANCE,
  AZ_SND_CHARGING_GUN,
  AZ_SND_CHARGING_ORDNANCE,
  AZ_SND_CLOAK_BEGIN,
  AZ_SND_CLOAK_END,
  AZ_SND_CORE_BEAM_CHARGE,
  AZ_SND_CORE_BEAM_FIRE,
  AZ_SND_CPLUS_ACTIVE,
  AZ_SND_CPLUS_CHARGED,
  AZ_SND_CPLUS_IMPACT,
  AZ_SND_CPLUS_READY,
  AZ_SND_DOOR_CLOSE,
  AZ_SND_DOOR_OPEN,
  AZ_SND_DROP_BOMB,
  AZ_SND_ELECTRICITY,
  AZ_SND_ENTER_ATMOSPHERE,
  AZ_SND_ERUPTION,
  AZ_SND_EXPLODE_BOMB,
  AZ_SND_EXPLODE_FIREBALL_LARGE,
  AZ_SND_EXPLODE_FIREBALL_SMALL,
  AZ_SND_EXPLODE_HYPER_ROCKET,
  AZ_SND_EXPLODE_MEGA_BOMB,
  AZ_SND_EXPLODE_ROCKET,
  AZ_SND_EXPLODE_SHIP,
  AZ_SND_FIRE_FIREBALL,
  AZ_SND_FIRE_GRAVITY_TORPEDO,
  AZ_SND_FIRE_GUN_CHARGED_BEAM,
  AZ_SND_FIRE_GUN_CHARGED_FREEZE,
  AZ_SND_FIRE_GUN_CHARGED_NORMAL,
  AZ_SND_FIRE_GUN_CHARGED_PHASE,
  AZ_SND_FIRE_GUN_CHARGED_PIERCE,
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
  AZ_SND_FREEZE_BADDIE,
  AZ_SND_HEAT_DAMAGE,
  AZ_SND_HIT_ARMOR,
  AZ_SND_HIT_WALL,
  AZ_SND_HURT_BOUNCER,
  AZ_SND_HURT_CRAWLER,
  AZ_SND_HURT_FISH,
  AZ_SND_HURT_KILOFUGE,
  AZ_SND_HURT_NOCTURNE,
  AZ_SND_HURT_OTH,
  AZ_SND_HURT_PLANT,
  AZ_SND_HURT_ROCKWYRM,
  AZ_SND_HURT_SHIP,
  AZ_SND_HURT_SHIP_SLIGHTLY,
  AZ_SND_HURT_SWOOPER,
  AZ_SND_HURT_TURRET,
  AZ_SND_HURT_ZIPPER,
  AZ_SND_KILL_ATOM,
  AZ_SND_KILL_BOUNCER,
  AZ_SND_KILL_DRAGONFLY,
  AZ_SND_KILL_FIRE_CRAWLER,
  AZ_SND_KILL_FISH,
  AZ_SND_KILL_OTH,
  AZ_SND_KILL_PLANT,
  AZ_SND_KILL_SWOOPER,
  AZ_SND_KILL_TURRET,
  AZ_SND_KLAXON_COUNTDOWN,
  AZ_SND_KLAXON_COUNTDOWN_LOW,
  AZ_SND_KLAXON_SHIELDS_LOW,
  AZ_SND_KLAXON_SHIELDS_VERY_LOW,
  AZ_SND_LAUNCH_OTH_RAZORS,
  AZ_SND_MAGBEEST_LEG_STOMP,
  AZ_SND_MAGBEEST_MAGNET,
  AZ_SND_MAGBEEST_MAGNET_CHARGE,
  AZ_SND_MAGBEEST_TUNNELLING,
  AZ_SND_MENU_CLICK,
  AZ_SND_MENU_HOVER,
  AZ_SND_METAL_CLINK,
  AZ_SND_NPS_PORTAL,
  AZ_SND_ORDN_ACTIVATE,
  AZ_SND_ORDN_DEACTIVATE,
  AZ_SND_ORION_BOOSTER,
  AZ_SND_PICKUP_ORDNANCE,
  AZ_SND_PICKUP_SHIELDS,
  AZ_SND_PISTON_MOVEMENT,
  AZ_SND_PLANET_DEFORM,
  AZ_SND_PLANET_EXPLODE,
  AZ_SND_PRISMATIC_CHARGE,
  AZ_SND_PRISMATIC_FIRE,
  AZ_SND_REACTIVE_ARMOR,
  AZ_SND_ROCKWYRM_SCREAM,
  AZ_SND_SONIC_SCREECH,
  AZ_SND_SPLASH,
  AZ_SND_THRUST,
  AZ_SND_TRACTOR_BEAM,
  AZ_SND_USE_COMM_CONSOLE,
  AZ_SND_USE_REFILL_CONSOLE,
  AZ_SND_USE_SAVE_CONSOLE,
} az_sound_key_t;

/*===========================================================================*/

void az_init_sound_datas(void);

// Indicate that we should play the given sound (once).  The sound will not
// loop, and cannot be cancelled or paused once started.
void az_play_sound(az_soundboard_t *soundboard, az_sound_key_t sound);

// Indicate that we should start playing, or continue to play, the given sound.
// To keep the sound going, we must call this function every frame with the
// same sound, otherwise the sound will stop.  As long as we keep calling this
// function, the sound will continue to loop.
void az_loop_sound(az_soundboard_t *soundboard, az_sound_key_t sound);

void az_loop_sound_with_volume(
    az_soundboard_t *soundboard, az_sound_key_t sound, float volume);

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

#endif // AZIMUTH_STATE_SOUND_H_
