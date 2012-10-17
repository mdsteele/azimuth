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
#include "azimuth/util/misc.h"
#include "azimuth/util/random.h"

/*===========================================================================*/

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
  assert(!proj->data->phased);
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

static void tick_projectile(az_space_state_t *state, az_projectile_t *proj,
                            double time) {
  // Age the projectile, and remove it if it is expired.
  proj->age += time;
  if (proj->age > proj->data->lifetime) {
    proj->kind = AZ_PROJ_NOTHING;
    return;
  }

  // Move the projectile by its velocity:
  const az_vector_t start = proj->position;
  proj->position = az_vadd(proj->position, az_vmul(proj->velocity, time));

  // Check if the projectile hits anything.  If it was fired by an enemy, it
  // can hit the ship:
  if (proj->fired_by_enemy) {
    az_vector_t hit_at;
    if (proj->last_hit_uid != AZ_SHIP_UID &&
        az_ray_hits_ship(&state->ship, start, az_vsub(proj->position, start),
                         &hit_at)) {
      proj->position = hit_at;
      state->ship.player.shields -= proj->data->damage;
      on_projectile_hit_target(state, proj, NULL);
      return;
    }
  }
  // Or, if the projectile was fired by the ship, it can hit baddies:
  else {
    az_vector_t hit_at = proj->position;
    az_baddie_t *hit_baddie = NULL;
    AZ_ARRAY_LOOP(baddie, state->baddies) {
      if (baddie->kind == AZ_BAD_NOTHING) continue;
      if (baddie->uid == proj->last_hit_uid) continue;
      if (az_ray_hits_baddie(baddie, start, az_vsub(hit_at, start),
                             &hit_at, NULL)) {
        hit_baddie = baddie;
      }
    }
    if (hit_baddie != NULL) {
      proj->position = hit_at;
      on_projectile_hit_target(state, proj, hit_baddie);
      hit_baddie->health -= proj->data->damage;
      if (hit_baddie->health <= 0.0) {
        hit_baddie->kind = AZ_BAD_NOTHING;
        az_try_add_pickup(state, AZ_PUP_LARGE_SHIELDS, hit_baddie->position);
      }
      return;
    }
  }
  // If it didn't hit the ship or a baddie already, the projectile can hit
  // walls (unless this kind of projectile passes through walls):
  if (!proj->data->phased) {
    az_vector_t hit_at = proj->position;
    bool did_hit = false;
    AZ_ARRAY_LOOP(wall, state->walls) {
      if (wall->kind == AZ_WALL_NOTHING) continue;
      if (az_ray_hits_wall(wall, start, az_vsub(hit_at, start),
                           &hit_at, NULL)) {
        did_hit = true;
      }
    }
    if (did_hit) {
      proj->position = hit_at;
      on_projectile_hit_wall(state, proj);
    }
  }

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

void az_tick_projectiles(az_space_state_t *state, double time) {
  AZ_ARRAY_LOOP(proj, state->projectiles) {
    if (proj->kind == AZ_PROJ_NOTHING) continue;
    tick_projectile(state, proj, time);
  }
}

/*===========================================================================*/
