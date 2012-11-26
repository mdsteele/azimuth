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
#include <math.h> // for INFINITY
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

// Common projectile impact code, called by both on_projectile_hit_wall and
// on_projectile_hit_target.
static void on_projectile_impact(az_space_state_t *state,
                                 az_projectile_t *proj, az_vector_t normal) {
  const double radius = proj->data->splash_radius;
  // Deal splash damage, if applicable.
  if (radius > 0.0 && proj->data->splash_damage > 0.0) {
    if (proj->fired_by_enemy) {
      if (az_vwithin(state->ship.position, proj->position, radius)) {
        az_damage_ship(state, proj->data->splash_damage);
      }
    } else {
      AZ_ARRAY_LOOP(baddie, state->baddies) {
        if (baddie->kind == AZ_BAD_NOTHING) continue;
        if (az_vwithin(baddie->position, proj->position,
                       radius + baddie->data->bounding_radius)) {
          az_damage_baddie(state, baddie, proj->data->splash_damage);
        }
      }
    }
  }

  // If applicable, burst into shrapnel.
  if (proj->data->shrapnel_kind != AZ_PROJ_NOTHING) {
    const double mid_theta = az_vtheta(normal);
    for (int i = -2; i <= 2; ++i) {
      const double theta = mid_theta + 0.2 * AZ_PI * (i + az_random() - 0.5);
      az_projectile_t *shrapnel;
      if (az_insert_projectile(state, &shrapnel)) {
        az_init_projectile(shrapnel, proj->data->shrapnel_kind, false,
                           az_vadd(proj->position, az_vpolar(0.1, theta)),
                           theta);
        if (!shrapnel->data->homing) {
          shrapnel->velocity =
            az_vmul(shrapnel->velocity, 0.5 + 0.5 * az_random());
        }
      }
    }
  }

  // Add particles.
  az_particle_t *particle;
  if (az_insert_particle(state, &particle)) {
    particle->kind = AZ_PAR_BOOM;
    particle->color = (az_color_t){255, 255, 255, 255};
    particle->position = proj->position;
    particle->velocity = AZ_VZERO;
    particle->lifetime = 0.3;
    particle->param1 = (radius <= 0.0 ? 10.0 : radius);
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

// Called when a projectile hits a wall or a door.  Note that phased
// projectiles can't ever hit walls, but they _can_ hit doors.
static void on_projectile_hit_wall(az_space_state_t *state,
                                   az_projectile_t *proj, az_vector_t normal) {
  on_projectile_impact(state, proj, normal);
  proj->kind = AZ_PROJ_NOTHING;
}

// Called when a projectile hits a baddie or the ship.  If the projectile is
// hitting the ship, baddie will be NULL.
static void on_projectile_hit_target(az_space_state_t *state,
                                     az_projectile_t *proj,
                                     az_baddie_t *baddie, az_vector_t normal) {
  // Run common impact code:
  on_projectile_impact(state, proj, normal);
  // Deal impact damage:
  if (baddie == NULL) {
    az_damage_ship(state, proj->data->impact_damage);
  } else {
    az_damage_baddie(state, baddie, proj->data->impact_damage);
  }
  // Remove the projecitle (unless it's a piercing projectile).
  if (proj->data->piercing) {
    proj->last_hit_uid = (baddie == NULL ? AZ_SHIP_UID : baddie->uid);
  } else {
    proj->kind = AZ_PROJ_NOTHING;
  }
}

static void projectile_home_in(az_space_state_t *state,
                               az_projectile_t *proj,
                               double time) {
  assert(proj->data->homing);
  const double proj_angle = az_vtheta(proj->velocity);
  // First, figure out what position we're homing in on.
  az_vector_t goal = state->ship.position;
  if (!proj->fired_by_enemy) {
    double best_dist = INFINITY;
    bool found_target = false;
    AZ_ARRAY_LOOP(baddie, state->baddies) {
      if (baddie->kind == AZ_BAD_NOTHING) continue;
      if (baddie->uid == proj->last_hit_uid) continue;
      const double dist = az_vdist(baddie->position, proj->position) +
        fabs(az_mod2pi(az_vtheta(az_vsub(baddie->position, proj->position)) -
                       proj_angle)) * 100.0;
      if (dist < best_dist) {
        best_dist = dist;
        found_target = true;
        goal = baddie->position;
      }
    }
    if (!found_target) return;
  }
  // Now, home in on the goal position.
  const double turn_radians = time * 3.5;
  const double goal_angle = az_vtheta(az_vsub(goal, proj->position));
  const double angle_delta = az_mod2pi(proj_angle - goal_angle);
  const double new_angle =
    (angle_delta < 0.0 ?
     (-angle_delta <= turn_radians ? goal_angle : proj_angle + turn_radians) :
     (angle_delta <= turn_radians ? goal_angle : proj_angle - turn_radians));
  proj->velocity = az_vpolar(proj->data->speed, new_angle);
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
  az_vector_t normal = AZ_VZERO;
  // Walls:
  if (!proj->data->phased) {
    AZ_ARRAY_LOOP(wall, state->walls) {
      if (wall->kind == AZ_WALL_NOTHING) continue;
      az_vector_t hit_at;
      if (az_ray_hits_wall(wall, start, delta, &hit_at, &normal)) {
        impact.type = AZ_IMP_WALL;
        impact.target.wall = wall;
        delta = az_vsub(hit_at, start);
      }
    }
  }
  // Doors:
  AZ_ARRAY_LOOP(door, state->doors) {
    if (door->kind == AZ_DOOR_NOTHING) continue;
    az_vector_t hit_at;
    if (az_ray_hits_door(door, start, delta, &hit_at, &normal)) {
      impact.type = AZ_IMP_DOOR;
      impact.target.door = door;
      delta = az_vsub(hit_at, start);
    }
  }
  // Ship:
  if (proj->fired_by_enemy && proj->last_hit_uid != AZ_SHIP_UID &&
      az_ship_is_present(&state->ship)) {
    az_vector_t hit_at;
    if (az_ray_hits_ship(&state->ship, start, delta, &hit_at, &normal)) {
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
      if (az_ray_hits_baddie(baddie, start, delta, &hit_at, &normal)) {
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
      if (proj->data->homing) {
        projectile_home_in(state, proj, time);
      }
      projectile_special_logic(state, proj, time);
      break;
    case AZ_IMP_BADDIE:
      on_projectile_hit_target(state, proj, impact.target.baddie, normal);
      break;
    case AZ_IMP_DOOR:
      if (!proj->fired_by_enemy) {
        // TODO: Test kind of projectile against kind of door.
        impact.target.door->is_open = true;
      }
      on_projectile_hit_wall(state, proj, normal);
      break;
    case AZ_IMP_SHIP:
      on_projectile_hit_target(state, proj, NULL, normal);
      break;
    case AZ_IMP_WALL:
      on_projectile_hit_wall(state, proj, normal);
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
