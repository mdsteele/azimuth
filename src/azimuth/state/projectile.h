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
#ifndef AZIMUTH_STATE_PROJECTILE_H_
#define AZIMUTH_STATE_PROJECTILE_H_

#include <stdbool.h>

#include "azimuth/state/player.h" // for az_damage_flags_t
#include "azimuth/state/uid.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

typedef enum {
  AZ_PROJ_NOTHING = 0,
  // Ship projectiles:
  AZ_PROJ_GUN_NORMAL,
  AZ_PROJ_GUN_CHARGED_NORMAL,
  AZ_PROJ_GUN_FREEZE,
  AZ_PROJ_GUN_CHARGED_FREEZE,
  AZ_PROJ_GUN_HOMING,
  AZ_PROJ_GUN_CHARGED_HOMING,
  AZ_PROJ_GUN_PHASE,
  AZ_PROJ_GUN_CHARGED_PHASE,
  AZ_PROJ_GUN_BURST,
  AZ_PROJ_GUN_SHRAPNEL,
  AZ_PROJ_GUN_PIERCE,
  AZ_PROJ_GUN_CHARGED_PIERCE,
  AZ_PROJ_GUN_CHARGED_BEAM,
  AZ_PROJ_GUN_FREEZE_HOMING,
  AZ_PROJ_GUN_FREEZE_PHASE,
  AZ_PROJ_GUN_FREEZE_BURST,
  AZ_PROJ_GUN_FREEZE_SHRAPNEL,
  AZ_PROJ_GUN_FREEZE_PIERCE,
  AZ_PROJ_GUN_HOMING_PHASE,
  AZ_PROJ_GUN_HOMING_BURST,
  AZ_PROJ_GUN_HOMING_SHRAPNEL,
  AZ_PROJ_GUN_HOMING_PIERCE,
  AZ_PROJ_GUN_PHASE_BURST,
  AZ_PROJ_GUN_PHASE_PIERCE,
  AZ_PROJ_GUN_BURST_PIERCE,
  AZ_PROJ_GUN_PIERCE_SHRAPNEL,
  AZ_PROJ_ROCKET,
  AZ_PROJ_HYPER_ROCKET,
  AZ_PROJ_MISSILE_FREEZE,
  AZ_PROJ_MISSILE_BARRAGE,
  AZ_PROJ_MISSILE_TRIPLE,
  AZ_PROJ_MISSILE_HOMING,
  AZ_PROJ_MISSILE_PHASE,
  AZ_PROJ_MISSILE_BURST,
  AZ_PROJ_MISSILE_PIERCE,
  AZ_PROJ_MISSILE_BEAM,
  AZ_PROJ_BOMB,
  AZ_PROJ_MEGA_BOMB,
  // Baddie-only projectiles:
  AZ_PROJ_FIREBALL_FAST,
  AZ_PROJ_FIREBALL_SLOW,
  AZ_PROJ_LASER_PULSE,
  AZ_PROJ_NUCLEAR_EXPLOSION,
  AZ_PROJ_OTH_HOMING,
  AZ_PROJ_OTH_ROCKET,
  AZ_PROJ_OTH_SPRAY,
  AZ_PROJ_SPARK,
  AZ_PROJ_SPINE,
  AZ_PROJ_STINGER
} az_proj_kind_t;

// Bitset of flags dictating special projectile behavior:
typedef uint_fast8_t az_proj_flags_t;
// NO_HIT: projectile never impacts anything (just goes until it expires)
#define AZ_PROJF_NO_HIT   ((az_proj_flags_t)(1u << 0))
// PHASED: projectile passes through walls
#define AZ_PROJF_PHASED   ((az_proj_flags_t)(1u << 1))
// PIERCING: projectile pierces through multiple targets
#define AZ_PROJF_PIERCING ((az_proj_flags_t)(1u << 2))

typedef struct {
  double speed;
  double lifetime; // how long the projectile lasts, in seconds
  double impact_damage; // how much damage the projectile deals on impact
  double splash_damage; // how much damage the explosion deals
  double splash_radius; // radius of explosion (zero for most projectiles)
  double impact_shake; // how much we shake the camera on impact
  double homing_rate; // homing turn rate in radians per second
  az_proj_kind_t shrapnel_kind; // if AZ_PROJ_NOTHING, this proj doesn't burst
  az_damage_flags_t damage_kind; // 0 is interpreted as normal damage
  az_proj_flags_t properties;
} az_proj_data_t;

typedef struct {
  az_proj_kind_t kind; // if AZ_PROJ_NOTHING, this projectile is not present
  const az_proj_data_t *data;
  bool fired_by_enemy; // if true, this projectile can hit the ship
  az_vector_t position;
  az_vector_t velocity;
  double angle;
  double power; // damage multiplier
  double age; // seconds
  int param; // the meaning of this is projectile-kind-specific
  // For projectiles that can hit multiple baddies (e.g. those from the PIERCE
  // gun), this records the UID of the last baddie hit.  The projectile cannot
  // immediately hit this same baddie again.  This can also be AZ_SHIP_UID if
  // it last hit the ship.
  az_uid_t last_hit_uid;
} az_projectile_t;

// Set reasonable initial field values for a projectile of the given kind,
// fired from the given position at the given angle.
void az_init_projectile(az_projectile_t *proj, az_proj_kind_t kind,
                        bool fired_by_enemy, az_vector_t position,
                        double angle, double power);

/*===========================================================================*/

#endif // AZIMUTH_STATE_PROJECTILE_H_
