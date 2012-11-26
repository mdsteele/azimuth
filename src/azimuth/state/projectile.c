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

#include "azimuth/state/projectile.h"

#include <assert.h>
#include <stdbool.h>

#include "azimuth/util/misc.h"

/*===========================================================================*/

static const az_proj_data_t proj_data[] = {
  [AZ_PROJ_GUN_NORMAL] = {
    .speed = 600.0,
    .lifetime = 1.0,
    .impact_damage = 1.0
  },
  [AZ_PROJ_GUN_CHARGED_NORMAL] = {
    .speed = 800.0,
    .lifetime = 1.0,
    .impact_damage = 5.0
  },
  [AZ_PROJ_GUN_TRIPLE] = {
    .speed = 600.0,
    .lifetime = 1.0,
    .impact_damage = 0.8
  },
  [AZ_PROJ_GUN_CHARGED_TRIPLE] = {
    .speed = 800.0,
    .lifetime = 1.0,
    .impact_damage = 4.0
  },
  [AZ_PROJ_GUN_HOMING] = {
    .speed = 500.0,
    .lifetime = 3.0,
    .impact_damage = 0.5,
    .homing = true
  },
  [AZ_PROJ_GUN_CHARGED_HOMING] = {
    .speed = 500.0,
    .lifetime = 5.0,
    .impact_damage = 2.5,
    .homing = true
  },
  [AZ_PROJ_GUN_TRIPLE_HOMING] = {
    .speed = 500.0,
    .lifetime = 3.0,
    .impact_damage = 0.4,
    .homing = true
  },
  [AZ_PROJ_GUN_PHASE] = {
    .speed = 600.0,
    .lifetime = 1.0,
    .impact_damage = 1.0,
    .phased = true
  },
  [AZ_PROJ_GUN_PIERCE] = {
    .speed = 700.0,
    .lifetime = 1.0,
    .impact_damage = 2.0,
    .piercing = true
  },
  [AZ_PROJ_GUN_TRIPLE_PIERCE] = {
    .speed = 700.0,
    .lifetime = 1.0,
    .impact_damage = 1.6,
    .piercing = true
  },
  [AZ_PROJ_GUN_HOMING_PIERCE] = {
    .speed = 600.0,
    .lifetime = 3.0,
    .impact_damage = 1.0,
    .homing = true,
    .piercing = true
  },
  [AZ_PROJ_ROCKET] = {
    .speed = 800.0,
    .lifetime = 1.0,
    .impact_damage = 5.0,
    .splash_damage = 5.0,
    .splash_radius = 20.0
  },
  [AZ_PROJ_BOMB] = {
    .speed = 0.0,
    .lifetime = 10.0,
    .splash_damage = 25.0,
    .splash_radius = 50.0
  },
  [AZ_PROJ_MEGA_BOMB] = {
    .speed = 0.0,
    .lifetime = 10.0,
    .splash_damage = 75.0,
    .splash_radius = 100.0
  },
};

void az_init_projectile(az_projectile_t *proj, az_proj_kind_t kind,
                        bool fired_by_enemy, az_vector_t position,
                        double angle) {
  assert(kind != AZ_PROJ_NOTHING);
  proj->kind = kind;
  const int data_index = (int)kind;
  assert(0 <= data_index && data_index < AZ_ARRAY_SIZE(proj_data));
  proj->data = &proj_data[data_index];
  proj->fired_by_enemy = fired_by_enemy;
  proj->position = position;
  proj->velocity = az_vpolar(proj->data->speed, angle);
  proj->age = 0.0;
  proj->last_hit_uid = AZ_NULL_UID;
}

/*===========================================================================*/
