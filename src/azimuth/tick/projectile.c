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

#include "azimuth/tick/projectile.h"

#include <assert.h>
#include <stdlib.h> // for NULL

#include "azimuth/state/projectile.h"
#include "azimuth/state/space.h"
#include "azimuth/state/uid.h"
#include "azimuth/tick/pickup.h" // for az_add_random_pickup
#include "azimuth/util/misc.h"
#include "azimuth/util/random.h"

/*===========================================================================*/

typedef enum {
  AZ_IMP_NOTHING = 0,
  AZ_IMP_BADDIE,
  AZ_IMP_DOOR,
  AZ_IMP_SHIP,
  AZ_IMP_WALL
} az_impact_type_t;

typedef struct {
  az_impact_type_t type;
  union {
    az_baddie_t *baddie;
    az_door_t *door;
    az_wall_t *wall;
  } target;
} az_impact_target_t;


static void on_projectile_impact(az_space_state_t *state,
                                 az_projectile_t *proj) {
  az_particle_t *particle;
  if (az_insert_particle(state, &particle)) {
    particle->kind = AZ_PAR_BOOM;
    particle->color = (az_color_t){255, 255, 255, 255};
    particle->position = proj->position;
    particle->velocity = AZ_VZERO;
    particle->lifetime = 0.3;
    particle->param1 = 10;
  }
  for (int i = 0; i < 5; ++i) {
    if (az_insert_particle(state, &particle)) {
      particle->kind = AZ_PAR_SPECK;
      particle->color = AZ_WHITE;
      particle->position = proj->position;
      particle->velocity = az_vpolar(20.0 + 50.0 * az_random(),
                                     az_random() * AZ_TWO_PI);
      particle->angle = 0.0;
      particle->lifetime = 1.0;
    } else break;
  }
}

static void on_projectile_hit_wall(az_space_state_t *state,
                                   az_projectile_t *proj) {
  proj->kind = AZ_PROJ_NOTHING;
  on_projectile_impact(state, proj);
}

// Called when a projectile hits a baddie or the ship.  If the projectile is
// hitting the ship, baddie will be NULL.
static void on_projectile_hit_target(az_space_state_t *state,
                                     az_projectile_t *proj,
                                     az_baddie_t *baddie) {
  if (proj->data->piercing) {
    proj->last_hit_uid = (baddie == NULL ? AZ_SHIP_UID : baddie->uid);
  } else {
    proj->kind = AZ_PROJ_NOTHING;
  }
  on_projectile_impact(state, proj);
}

static void projectile_special_logic(az_space_state_t *state,
                                     az_projectile_t *proj,
                                     double time) {
  // The projectile still hasn't hit anything.  Apply kind-specific logic to
  // the projectile (e.g. homing projectiles will home in).
  switch (proj->kind) {
    case AZ_PROJ_BOMB:
      //maybe_detonate_bomb(state, proj);
      break;
    // TODO: implement logic for other special projectile kinds here
    default: break;
  }
}

static void tick_projectile(az_space_state_t *state, az_projectile_t *proj,
                            double time) {
  // Age the projectile, and remove it if it is expired.
  proj->age += time;
  if (proj->age > proj->data->lifetime) {
    proj->kind = AZ_PROJ_NOTHING;
    return;
  }

  // Figure out what, if anything, the projectile hits:
  const az_vector_t start = proj->position;
  az_vector_t delta = az_vmul(proj->velocity, time);
  az_impact_target_t impact = {.type = AZ_IMP_NOTHING};
  // Walls:
  if (!proj->data->phased) {
    AZ_ARRAY_LOOP(wall, state->walls) {
      if (wall->kind == AZ_WALL_NOTHING) continue;
      az_vector_t hit_at;
      if (az_ray_hits_wall(wall, start, delta, &hit_at, NULL)) {
        impact.type = AZ_IMP_WALL;
        impact.target.wall = wall;
        delta = az_vsub(hit_at, start);
      }
    }
  }
  // Doors:
  AZ_ARRAY_LOOP(door, state->doors) {
    az_vector_t hit_at;
    if (az_ray_hits_door(door, start, delta, &hit_at, NULL)) {
      impact.type = AZ_IMP_DOOR;
      impact.target.door = door;
      delta = az_vsub(hit_at, start);
    }
  }
  // Ship:
  if (proj->fired_by_enemy && proj->last_hit_uid != AZ_SHIP_UID) {
    az_vector_t hit_at;
    if (az_ray_hits_ship(&state->ship, start, delta, &hit_at)) {
      impact.type = AZ_IMP_SHIP;
      delta = az_vsub(hit_at, start);
    }
  }
  // Baddies:
  if (!proj->fired_by_enemy) {
    AZ_ARRAY_LOOP(baddie, state->baddies) {
      if (baddie->kind == AZ_BAD_NOTHING) continue;
      if (baddie->uid == proj->last_hit_uid) continue;
      az_vector_t hit_at;
      if (az_ray_hits_baddie(baddie, start, delta, &hit_at, NULL)) {
        impact.type = AZ_IMP_BADDIE;
        impact.target.baddie = baddie;
        delta = az_vsub(hit_at, start);
      }
    }
  }

  // Move the projectile:
  proj->position = az_vadd(start, delta);

  // Resolve the impact (if any):
  switch (impact.type) {
    case AZ_IMP_NOTHING:
      projectile_special_logic(state, proj, time);
      break;
    case AZ_IMP_BADDIE:
      on_projectile_hit_target(state, proj, impact.target.baddie);
      impact.target.baddie->health -= proj->data->damage;
      if (impact.target.baddie->health <= 0.0) {
        impact.target.baddie->kind = AZ_BAD_NOTHING;
        az_add_random_pickup(state,
                             impact.target.baddie->data->potential_pickups,
                             impact.target.baddie->position);
      }
      break;
    case AZ_IMP_DOOR:
      if (!proj->fired_by_enemy) {
        // TODO: Test kind of projectile against kind of door.
        impact.target.door->is_open = true;
      }
      on_projectile_hit_wall(state, proj);
      break;
    case AZ_IMP_SHIP:
      state->ship.player.shields -= proj->data->damage;
      on_projectile_hit_target(state, proj, NULL);
      break;
    case AZ_IMP_WALL:
      on_projectile_hit_wall(state, proj);
      break;
  }
}

void az_tick_projectiles(az_space_state_t *state, double time) {
  AZ_ARRAY_LOOP(proj, state->projectiles) {
    if (proj->kind == AZ_PROJ_NOTHING) continue;
    tick_projectile(state, proj, time);
  }
}

/*===========================================================================*/
