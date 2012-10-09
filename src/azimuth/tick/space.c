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

#include "azimuth/tick/space.h"

#include <assert.h>
#include <math.h> // for pow
#include <stdbool.h>
#include <stdlib.h> // for NULL

#include "azimuth/state/space.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

#define PICKUP_MAX_AGE 10.0
#define PICKUP_COLLECTION_RANGE 20.0
#define TURN_RATE 5.0
#define THRUST_ACCEL 500.0

static void tick_particles(az_space_state_t *state,
                           double time_seconds) {
  AZ_ARRAY_LOOP(particle, state->particles) {
    if (particle->kind == AZ_PAR_NOTHING) continue;
    particle->age += time_seconds;
    if (particle->age >= particle->lifetime) {
      particle->kind = AZ_PAR_NOTHING;
    }
  }
}

static void tick_pickups(az_space_state_t *state,
                         double time_seconds) {
  az_player_t *player = &state->ship.player;
  AZ_ARRAY_LOOP(pickup, state->pickups) {
    if (pickup->kind == AZ_PUP_NOTHING) continue;
    pickup->age += time_seconds;
    if (az_vwithin(pickup->position, state->ship.position,
                   PICKUP_COLLECTION_RANGE)) {
      switch (pickup->kind) {
        case AZ_PUP_ROCKETS:
          player->rockets = az_imin(player->max_rockets, player->rockets +
                                    AZ_ROCKETS_PER_PICKUP);
          break;
        case AZ_PUP_BOMBS:
          player->bombs = az_imin(player->max_bombs, player->bombs +
                                  AZ_BOMBS_PER_PICKUP);
          break;
        case AZ_PUP_SMALL_SHIELDS:
          player->shields = az_imin(player->max_shields, player->shields +
                                    AZ_SHIELDS_PER_SMALL_PICKUP);
          break;
        case AZ_PUP_MEDIUM_SHIELDS:
          player->shields = az_imin(player->max_shields, player->shields +
                                    AZ_SHIELDS_PER_MEDIUM_PICKUP);
          break;
        case AZ_PUP_LARGE_SHIELDS:
          player->shields = az_imin(player->max_shields, player->shields +
                                    AZ_SHIELDS_PER_LARGE_PICKUP);
          break;
        default: assert(false);
      }
      pickup->kind = AZ_PUP_NOTHING;
    } else if (pickup->age >= PICKUP_MAX_AGE) {
      pickup->kind = AZ_PUP_NOTHING;
    }
  }
}

static void tick_ship(az_space_state_t *state, double time_seconds) {
  az_ship_t *ship = &state->ship;
  az_player_t *player = &ship->player;
  const az_controls_t *controls = &ship->controls;
  const bool has_lateral = az_has_upgrade(player, AZ_UPG_LATERAL_THRUSTERS);
  const double impulse = THRUST_ACCEL * time_seconds;

  // Recharge energy:
  const double charge_rate = 75.0 +
    (az_has_upgrade(player, AZ_UPG_FUSION_REACTOR) ? 75.0 : 0.0) +
    (az_has_upgrade(player, AZ_UPG_QUANTUM_REACTOR) ? 125.0 : 0.0);
  player->energy = az_dmin(player->max_energy,
                           player->energy + charge_rate * time_seconds);

  // Apply velocity:
  ship->position = az_vadd(ship->position,
                           az_vmul(ship->velocity, time_seconds));

  // Apply gravity:
  const double planetoid_radius = 100000.0; // pixels
  const double surface_gravity = 1.6; // pixels per second^2
  // Gravity works as follows.  By the spherical shell theorem, the force of
  // gravity on the ship (while inside the planetoid) varies linearly with the
  // distance from the planetoid's center, so the change in velocity is
  // time_seconds * surface_gravity * vnorm(position) / planetoid_radius.
  // However, we should then multiply this scalar by the unit vector pointing
  // from the ship towards the core, which is -position/vnorm(position).  The
  // vnorm(position) factors cancel and we end up with what we have here.
  ship->velocity = az_vadd(ship->velocity,
                           az_vmul(ship->position, -time_seconds *
                                   (surface_gravity / planetoid_radius)));

  // Turning left:
  if (controls->left && !controls->right) {
    if (!controls->util) {
      ship->angle = az_mod2pi(ship->angle + TURN_RATE * time_seconds);
    } else if (has_lateral) {
      ship->velocity = az_vadd(ship->velocity,
                               az_vpolar(impulse / 2,
                                         ship->angle - AZ_HALF_PI));
    }
  }

  // Turning right:
  if (controls->right && !controls->left) {
    if (!controls->util) {
      ship->angle = az_mod2pi(ship->angle - TURN_RATE * time_seconds);
    } else if (has_lateral) {
      ship->velocity = az_vadd(ship->velocity,
                               az_vpolar(impulse / 2,
                                         ship->angle + AZ_HALF_PI));
    }
  }

  // Forward thrust:
  if (controls->up && !controls->down) {
    if (!controls->util) {
      ship->velocity = az_vadd(ship->velocity,
                               az_vpolar(impulse, ship->angle));
    } else if (has_lateral) {
      ship->velocity = az_vadd(ship->velocity,
                               az_vpolar(-impulse/2, ship->angle));
    }
  }

  // Retro thrusters:
  if (controls->down && !controls->up &&
      az_has_upgrade(player, AZ_UPG_RETRO_THRUSTERS)) {
    const double speed = az_vnorm(ship->velocity);
    if (speed <= impulse) {
      ship->velocity = AZ_VZERO;
    } else {
      ship->velocity = az_vmul(ship->velocity, (speed - impulse) / speed);
    }
  }

  // Apply drag:
  const double base_max_speed =
    400.0 * (az_has_upgrade(player, AZ_UPG_DYNAMIC_ARMOR) ? 1.5 : 1.0);
  const double drag_coeff = THRUST_ACCEL / (base_max_speed * base_max_speed);
  const az_vector_t drag_force =
    az_vmul(ship->velocity, -drag_coeff * az_vnorm(ship->velocity));
  ship->velocity = az_vadd(ship->velocity, az_vmul(drag_force, time_seconds));

  // Deactivate tractor beam:
  if (!controls->util) {
    ship->tractor_beam.active = false;
  }
  // Activate tractor beam:
  if (controls->util && controls->fire1 && !ship->tractor_beam.active) {
    az_node_t *closest_node = NULL;
    double best_distance = 300.0;
    AZ_ARRAY_LOOP(node, state->nodes) {
      if (node->kind != AZ_NODE_NOTHING) {
        const double dist = az_vnorm(az_vsub(node->position, ship->position));
        if (dist <= best_distance) {
          closest_node = node;
          best_distance = dist;
        }
      }
    }
    if (closest_node != NULL) {
      ship->tractor_beam.active = true;
      ship->tractor_beam.node_uid = closest_node->uid;
      ship->tractor_beam.distance = best_distance;
    }
  }
  // TODO: Apply tractor beam's velocity changes
}

static void tick_timer(az_timer_t *timer, double time_seconds) {
  if (timer->active_for < 0.0) return;
  if (timer->active_for < 10.0) timer->active_for += time_seconds;
  timer->time_remaining = az_dmax(0.0, timer->time_remaining - time_seconds);
}

static void tick_camera(az_vector_t *camera, az_vector_t towards,
                        double time_seconds) {
  const double tracking_base = 0.00003; // smaller = faster tracking
  const az_vector_t difference = az_vsub(towards, *camera);
  const az_vector_t change =
    az_vmul(difference, 1.0 - pow(tracking_base, time_seconds));
  *camera = az_vadd(*camera, change);
}

void az_tick_space_state(az_space_state_t *state, double time_seconds) {
  ++state->clock;

  tick_particles(state, time_seconds);
  tick_pickups(state, time_seconds);

  AZ_ARRAY_LOOP(proj, state->projectiles) {
    if (proj->kind == AZ_PROJ_NOTHING) continue;
    proj->age += time_seconds;
    if (proj->age > 1.0) {
      proj->kind = AZ_PROJ_NOTHING;
      continue;
    }

    proj->position = az_vadd(proj->position,
                             az_vmul(proj->velocity, time_seconds));
    // TODO: don't do this loop if the proj was fired by a baddie
    AZ_ARRAY_LOOP(baddie, state->baddies) {
      if (baddie->kind == AZ_BAD_NOTHING) continue;
      if (az_vwithin(baddie->position, proj->position, 20.0)) {
        az_particle_t *particle;
        if (az_insert_particle(state, &particle)) {
          particle->kind = AZ_PAR_BOOM;
          particle->color = (az_color_t){255, 255, 255, 255};
          particle->position = proj->position;
          particle->velocity = AZ_VZERO;
          particle->lifetime = 0.3;
          particle->param1 = 10;
        }
        baddie->health -= 1.0;
        if (baddie->health <= 0.0) {
          baddie->kind = AZ_BAD_NOTHING;
          az_try_add_pickup(state, AZ_PUP_LARGE_SHIELDS, baddie->position);
        }
        proj->kind = AZ_PROJ_NOTHING;
        break;
      }
    }
    // TODO: if proj is baddie-fired, check if hits ship
  }

  tick_ship(state, time_seconds);
  tick_camera(&state->camera, state->ship.position, time_seconds);
  tick_timer(&state->timer, time_seconds);

  const double fire_cost = 20.0;
  if (state->ship.controls.fire1 && state->ship.player.energy >= fire_cost) {
    az_projectile_t *projectile;
    if (az_insert_projectile(state, &projectile)) {
      state->ship.player.energy -= fire_cost;
      projectile->kind = AZ_PROJ_GUN_NORMAL;
      projectile->position = az_vadd(state->ship.position,
                                     az_vpolar(18, state->ship.angle));
      projectile->velocity = az_vpolar(600.0, state->ship.angle);
      state->ship.controls.fire1 = false;
    }
  }
}

/*===========================================================================*/
