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
#include <string.h> // for memset

#include "azimuth/util/misc.h"

/*===========================================================================*/

static const az_proj_data_t proj_data[] = {
  [AZ_PROJ_GUN_NORMAL] = {
    .speed = 600.0,
    .lifetime = 2.0,
    .impact_damage = 1.0
  },
  [AZ_PROJ_GUN_CHARGED_NORMAL] = {
    .speed = 800.0,
    .lifetime = 2.0,
    .impact_damage = 5.0,
    .damage_kind = AZ_DMGF_NORMAL | AZ_DMGF_CHARGED
  },
  [AZ_PROJ_GUN_FREEZE] = {
    .speed = 600.0,
    .lifetime = 2.0,
    .impact_damage = 1.0,
    .damage_kind = AZ_DMGF_NORMAL | AZ_DMGF_FREEZE
  },
  [AZ_PROJ_GUN_CHARGED_FREEZE] = {
    .speed = 850.0,
    .lifetime = 2.0,
    .impact_damage = 5.0,
    .damage_kind = AZ_DMGF_NORMAL | AZ_DMGF_CHARGED | AZ_DMGF_FREEZE
  },
  [AZ_PROJ_GUN_FREEZE_TRIPLE] = {
    .speed = 600.0,
    .lifetime = 2.0,
    .impact_damage = 0.8,
    .damage_kind = AZ_DMGF_NORMAL | AZ_DMGF_FREEZE
  },
  [AZ_PROJ_GUN_TRIPLE] = {
    .speed = 600.0,
    .lifetime = 2.0,
    .impact_damage = 0.8
  },
  [AZ_PROJ_GUN_CHARGED_TRIPLE] = {
    .speed = 800.0,
    .lifetime = 2.0,
    .impact_damage = 4.0,
    .damage_kind = AZ_DMGF_NORMAL | AZ_DMGF_CHARGED
  },
  [AZ_PROJ_GUN_HOMING] = {
    .speed = 500.0,
    .lifetime = 3.0,
    .impact_damage = 0.5,
    .properties = AZ_PROJF_HOMING
  },
  [AZ_PROJ_GUN_CHARGED_HOMING] = {
    .speed = 500.0,
    .lifetime = 5.0,
    .impact_damage = 2.5,
    .damage_kind = AZ_DMGF_NORMAL | AZ_DMGF_CHARGED,
    .properties = AZ_PROJF_HOMING
  },
  [AZ_PROJ_GUN_FREEZE_HOMING] = {
    .speed = 500.0,
    .lifetime = 3.0,
    .impact_damage = 0.5,
    .damage_kind = AZ_DMGF_NORMAL | AZ_DMGF_FREEZE,
    .properties = AZ_PROJF_HOMING
  },
  [AZ_PROJ_GUN_TRIPLE_HOMING] = {
    .speed = 500.0,
    .lifetime = 3.0,
    .impact_damage = 0.4,
    .properties = AZ_PROJF_HOMING
  },
  [AZ_PROJ_GUN_PHASE] = {
    .speed = 600.0,
    .lifetime = 0.5,
    .impact_damage = 0.1,
    .properties = AZ_PROJF_PHASED
  },
  [AZ_PROJ_GUN_CHARGED_PHASE] = {
    .speed = 800.0,
    .lifetime = 3.0,
    .impact_damage = 10.0,
    .damage_kind = AZ_DMGF_NORMAL | AZ_DMGF_CHARGED,
    .properties = AZ_PROJF_PHASED | AZ_PROJF_PIERCING
  },
  [AZ_PROJ_GUN_FREEZE_PHASE] = {
    .speed = 600.0,
    .lifetime = 0.5,
    .impact_damage = 0.1,
    .damage_kind = AZ_DMGF_NORMAL | AZ_DMGF_FREEZE,
    .properties = AZ_PROJF_PHASED
  },
  [AZ_PROJ_GUN_TRIPLE_PHASE] = {
    .speed = 600.0,
    .lifetime = 0.5,
    .impact_damage = 0.08,
    .properties = AZ_PROJF_PHASED
  },
  [AZ_PROJ_GUN_HOMING_PHASE] = {
    .speed = 600.0,
    .lifetime = 0.5,
    .impact_damage = 0.05,
    .properties = AZ_PROJF_HOMING | AZ_PROJF_PHASED
  },
  [AZ_PROJ_GUN_PHASE_BURST] = {
    .speed = 600.0,
    .lifetime = 0.5,
    .impact_damage = 0.1,
    .shrapnel_kind = AZ_PROJ_GUN_PHASE,
    .properties = AZ_PROJF_PHASED
  },
  [AZ_PROJ_GUN_PHASE_PIERCE] = {
    .speed = 600.0,
    .lifetime = 0.5,
    .impact_damage = 0.2,
    .damage_kind = AZ_DMGF_NORMAL | AZ_DMGF_PIERCE,
    .properties = AZ_PROJF_PHASED | AZ_PROJF_PIERCING
  },
  [AZ_PROJ_GUN_BURST] = {
    .speed = 900.0,
    .lifetime = 2.0,
    .impact_damage = 1.5,
    .shrapnel_kind = AZ_PROJ_GUN_SHRAPNEL
  },
  [AZ_PROJ_GUN_FREEZE_BURST] = {
    .speed = 900.0,
    .lifetime = 2.0,
    .impact_damage = 1.0,
    .shrapnel_kind = AZ_PROJ_GUN_FREEZE_SHRAPNEL,
    .damage_kind = AZ_DMGF_NORMAL | AZ_DMGF_FREEZE
  },
  [AZ_PROJ_GUN_TRIPLE_BURST] = {
    .speed = 900.0,
    .lifetime = 2.0,
    .impact_damage = 1.0,
    .shrapnel_kind = AZ_PROJ_GUN_SHRAPNEL
  },
  [AZ_PROJ_GUN_HOMING_BURST] = {
    .speed = 900.0,
    .lifetime = 2.0,
    .impact_damage = 1.5,
    .shrapnel_kind = AZ_PROJ_GUN_HOMING_SHRAPNEL
  },
  [AZ_PROJ_GUN_BURST_PIERCE] = {
    .speed = 900.0,
    .lifetime = 2.0,
    .impact_damage = 3.0,
    .shrapnel_kind = AZ_PROJ_GUN_SHRAPNEL,
    .damage_kind = AZ_DMGF_NORMAL | AZ_DMGF_PIERCE,
    .properties = AZ_PROJF_PIERCING
  },
  [AZ_PROJ_GUN_SHRAPNEL] = {
    .speed = 500.0,
    .lifetime = 0.8,
    .impact_damage = 0.5
  },
  [AZ_PROJ_GUN_FREEZE_SHRAPNEL] = {
    .speed = 500.0,
    .lifetime = 0.8,
    .impact_damage = 0.5,
    .damage_kind = AZ_DMGF_NORMAL | AZ_DMGF_FREEZE
  },
  [AZ_PROJ_GUN_HOMING_SHRAPNEL] = {
    .speed = 400.0,
    .lifetime = 1.1,
    .impact_damage = 0.5,
    .properties = AZ_PROJF_HOMING
  },
  [AZ_PROJ_GUN_PIERCE] = {
    .speed = 700.0,
    .lifetime = 2.0,
    .impact_damage = 2.0,
    .damage_kind = AZ_DMGF_NORMAL | AZ_DMGF_PIERCE,
    .properties = AZ_PROJF_PIERCING
  },
  [AZ_PROJ_GUN_CHARGED_PIERCE] = {
    .speed = 800.0,
    .lifetime = 2.0,
    .impact_damage = 10.0,
    .damage_kind = AZ_DMGF_NORMAL | AZ_DMGF_CHARGED | AZ_DMGF_PIERCE,
    .properties = AZ_PROJF_PIERCING
  },
  [AZ_PROJ_GUN_FREEZE_PIERCE] = {
    .speed = 700.0,
    .lifetime = 2.0,
    .impact_damage = 2.0,
    .damage_kind = AZ_DMGF_NORMAL | AZ_DMGF_FREEZE | AZ_DMGF_PIERCE,
    .properties = AZ_PROJF_PIERCING
  },
  [AZ_PROJ_GUN_TRIPLE_PIERCE] = {
    .speed = 700.0,
    .lifetime = 2.0,
    .impact_damage = 1.6,
    .damage_kind = AZ_DMGF_NORMAL | AZ_DMGF_PIERCE,
    .properties = AZ_PROJF_PIERCING
  },
  [AZ_PROJ_GUN_HOMING_PIERCE] = {
    .speed = 600.0,
    .lifetime = 3.0,
    .impact_damage = 1.0,
    .damage_kind = AZ_DMGF_NORMAL | AZ_DMGF_PIERCE,
    .properties = AZ_PROJF_HOMING | AZ_PROJF_PIERCING
  },
  [AZ_PROJ_ROCKET] = {
    .speed = 1000.0,
    .lifetime = 3.0,
    .impact_damage = 5.0,
    .splash_damage = 5.0,
    .splash_radius = 25.0,
    .impact_shake = 0.75,
    .damage_kind = AZ_DMGF_ROCKET
  },
  [AZ_PROJ_HYPER_ROCKET] = {
    .speed = 1500.0,
    .lifetime = 3.0,
    .impact_damage = 25.0,
    .splash_damage = 25.0,
    .splash_radius = 40.0,
    .impact_shake = 4.0,
    .damage_kind = AZ_DMGF_HYPER_ROCKET | AZ_DMGF_ROCKET
  },
  [AZ_PROJ_MISSILE_FREEZE] = {
    .speed = 1000.0,
    .lifetime = 3.0,
    .impact_damage = 25.0,
    .splash_damage = 3.0,
    .splash_radius = 200.0,
    .impact_shake = 4.0,
    .damage_kind = AZ_DMGF_FREEZE | AZ_DMGF_ROCKET
  },
  [AZ_PROJ_MISSILE_BARRAGE] = {
    .lifetime = 0.27,
    .properties = AZ_PROJF_NO_HIT
  },
  [AZ_PROJ_MISSILE_TRIPLE] = {
    .speed = 1000.0,
    .lifetime = 3.0,
    .impact_damage = 5.0,
    .splash_damage = 5.0,
    .splash_radius = 25.0,
    .impact_shake = 0.75,
    .damage_kind = AZ_DMGF_ROCKET
  },
  [AZ_PROJ_MISSILE_HOMING] = {
    .speed = 800.0,
    .lifetime = 3.0,
    .impact_damage = 4.0,
    .splash_damage = 4.0,
    .splash_radius = 20.0,
    .impact_shake = 0.75,
    .damage_kind = AZ_DMGF_ROCKET,
    .properties = AZ_PROJF_HOMING
  },
  [AZ_PROJ_MISSILE_PHASE] = {
    .speed = 1000.0,
    .lifetime = 3.0,
    .impact_damage = 18.0,
    .splash_damage = 18.0,
    .splash_radius = 40.0,
    .impact_shake = 4.0,
    .damage_kind = AZ_DMGF_HYPER_ROCKET | AZ_DMGF_ROCKET,
    .properties = AZ_PROJF_PHASED
  },
  [AZ_PROJ_MISSILE_BURST] = {
    .speed = 800.0,
    .lifetime = 3.0,
    .properties = AZ_PROJF_NO_HIT
  },
  [AZ_PROJ_MISSILE_PIERCE] = {
    .speed = 1500.0,
    .lifetime = 3.0,
    .impact_damage = 18.0,
    .splash_damage = 18.0,
    .splash_radius = 35.0,
    .impact_shake = 4.0,
    .damage_kind = AZ_DMGF_HYPER_ROCKET | AZ_DMGF_ROCKET,
    .properties = AZ_PROJF_PIERCING
  },
  [AZ_PROJ_BOMB] = {
    .lifetime = 1.5,
    .splash_damage = 25.0,
    .splash_radius = 60.0,
    .impact_shake = 1.5,
    .damage_kind = AZ_DMGF_BOMB
  },
  [AZ_PROJ_MEGA_BOMB] = {
    .lifetime = 3.0,
    .splash_damage = 75.0,
    .splash_radius = 200.0,
    .impact_shake = 3.0,
    .damage_kind = AZ_DMGF_MEGA_BOMB | AZ_DMGF_BOMB,
    .properties = AZ_PROJF_NO_HIT
  },
  [AZ_PROJ_FIREBALL_FAST] = {
    .speed = 550.0,
    .lifetime = 2.0,
    .impact_damage = 10.0
  },
  [AZ_PROJ_FIREBALL_SLOW] = {
    .speed = 260.0,
    .lifetime = 2.0,
    .impact_damage = 10.0
  },
  [AZ_PROJ_SPINE] = {
    .speed = 200.0,
    .lifetime = 4.0,
    .impact_damage = 8.0
  }
};

void az_init_projectile(az_projectile_t *proj, az_proj_kind_t kind,
                        bool fired_by_enemy, az_vector_t position,
                        double angle) {
  assert(kind != AZ_PROJ_NOTHING);
  memset(proj, 0, sizeof(*proj));
  proj->kind = kind;
  const int data_index = (int)kind;
  assert(0 <= data_index && data_index < AZ_ARRAY_SIZE(proj_data));
  proj->data = &proj_data[data_index];
  proj->fired_by_enemy = fired_by_enemy;
  proj->position = position;
  proj->velocity = az_vpolar(proj->data->speed, angle);
  proj->angle = angle;
  proj->last_hit_uid = AZ_NULL_UID;
}

/*===========================================================================*/
