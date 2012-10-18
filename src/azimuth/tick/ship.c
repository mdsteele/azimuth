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

#include "azimuth/tick/ship.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h> // for NULL

#include "azimuth/constants.h"
#include "azimuth/state/particle.h"
#include "azimuth/state/projectile.h"
#include "azimuth/state/ship.h"
#include "azimuth/state/space.h"
#include "azimuth/util/color.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/random.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

void az_tick_ship(az_space_state_t *state, double time) {
  az_ship_t *ship = &state->ship;
  az_player_t *player = &ship->player;
  az_controls_t *controls = &ship->controls;
  const bool has_lateral = az_has_upgrade(player, AZ_UPG_LATERAL_THRUSTERS);
  const double impulse = AZ_SHIP_BASE_THRUST_ACCEL * time;

  // Recharge energy:
  const double recharge_rate = AZ_SHIP_BASE_RECHARGE_RATE +
    (az_has_upgrade(player, AZ_UPG_FUSION_REACTOR) ?
     AZ_FUSION_REACTOR_RECHARGE_RATE : 0.0) +
    (az_has_upgrade(player, AZ_UPG_QUANTUM_REACTOR) ?
     AZ_QUANTUM_REACTOR_RECHARGE_RATE : 0.0);
  player->energy = az_dmin(player->max_energy,
                           player->energy + recharge_rate * time);

  // Deactivate tractor beam if necessary, otherwise get the node it is locked
  // onto.
  az_node_t *tractor_node = NULL;
  if (ship->tractor_beam.active) {
    if (!controls->util ||
        !az_lookup_node(state, ship->tractor_beam.node_uid, &tractor_node)) {
      ship->tractor_beam.active = false;
    }
  }

  // Apply velocity.  If the tractor beam is active, that implies angular
  // motion (around the locked-onto node); otherwise, linear motion.
  if (ship->tractor_beam.active) {
    // TODO: check for wall/baddie impacts
    assert(tractor_node != NULL);
    const az_vector_t delta = az_vsub(ship->position, tractor_node->position);
    assert(az_dapprox(0.0, az_vdot(ship->velocity, delta)));
    assert(az_dapprox(ship->tractor_beam.distance, az_vnorm(delta)));
    const double invdist = 1.0 / ship->tractor_beam.distance;
    const double dtheta =
      time * invdist *
      az_vdot(ship->velocity, az_vrot90ccw(az_vmul(delta, invdist)));
    ship->position = az_vadd(az_vrotate(delta, dtheta),
                             tractor_node->position);
    ship->velocity = az_vrotate(ship->velocity, dtheta);
    ship->angle = az_mod2pi(ship->angle + dtheta);
  } else {
    assert(tractor_node == NULL);

    // Check for wall impacts.
    // TODO: Also check for baddie impacts.
    // TODO: What do we do if the ship is stationary, but hits the wall due to
    //   rotating?  Maybe the ship should be treated as a circle rather than as
    //   a polygon (at least for wall impacts, if not for projectiles).
    az_vector_t end_pos = az_vadd(ship->position,
                                  az_vmul(ship->velocity, time));
    az_vector_t impact = AZ_VZERO;
    az_vector_t normal = AZ_VZERO;
    az_wall_t *hit_wall = NULL;
    AZ_ARRAY_LOOP(wall, state->walls) {
      if (wall->kind == AZ_WALL_NOTHING) continue;
      if (az_ship_would_hit_wall(wall, ship, az_vsub(end_pos, ship->position),
                                 &end_pos, &impact, &normal)) {
        hit_wall = wall;
      }
    }
    ship->position = end_pos;
    if (hit_wall != NULL) {
      // Push the ship slightly away from the impact point (so that we're
      // hopefully no longer in contact with the wall).
      if (normal.x != 0.0 || normal.y != 0.0) {
        ship->position = az_vadd(ship->position,
                                 az_vmul(az_vunit(normal), 1.0));
      }
      // Bounce the ship off the wall.
      ship->velocity = az_vsub(ship->velocity,
                               az_vmul(az_vproj(ship->velocity, normal),
                                       1.0 + hit_wall->data->elasticity));
      // TODO: Damage the ship.
      // Put a particle at the impact point.
      az_particle_t *particle;
      if (az_insert_particle(state, &particle)) {
        particle->kind = AZ_PAR_BOOM;
        particle->color = (az_color_t){255, 255, 255, 255};
        particle->position = impact;
        particle->velocity = AZ_VZERO;
        particle->lifetime = 0.3;
        particle->param1 = 10;
      }
    }
  }

  // Apply gravity:
  // Gravity works as follows.  By the spherical shell theorem, the force of
  // gravity on the ship (while inside the planetoid) varies linearly with the
  // distance from the planetoid's center, so the change in velocity is time *
  // surface_gravity * vnorm(position) / planetoid_radius.  However, we should
  // then multiply this scalar by the unit vector pointing from the ship
  // towards the core, which is -position/vnorm(position).  The vnorm(position)
  // factors cancel and we end up with what we have here.
  ship->velocity = az_vadd(ship->velocity,
      az_vmul(ship->position, -time *
              (AZ_PLANETOID_SURFACE_GRAVITY / AZ_PLANETOID_RADIUS)));

  // Turning left:
  if (controls->left && !controls->right) {
    if (!controls->burn) {
      ship->angle = az_mod2pi(ship->angle + AZ_SHIP_TURN_RATE * time);
    } else if (has_lateral) {
      ship->velocity = az_vadd(ship->velocity,
                               az_vpolar(impulse / 2,
                                         ship->angle - AZ_HALF_PI));
    }
  }

  // Turning right:
  if (controls->right && !controls->left) {
    if (!controls->burn) {
      ship->angle = az_mod2pi(ship->angle - AZ_SHIP_TURN_RATE * time);
    } else if (has_lateral) {
      ship->velocity = az_vadd(ship->velocity,
                               az_vpolar(impulse / 2,
                                         ship->angle + AZ_HALF_PI));
    }
  }

  // Forward thrust:
  if (controls->up && !controls->down) {
    if (!controls->burn) {
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
    AZ_SHIP_BASE_MAX_SPEED *
    (az_has_upgrade(player, AZ_UPG_DYNAMIC_ARMOR) ?
     AZ_DYNAMIC_ARMOR_SPEED_MULT : 1.0);
  const double drag_coeff =
    AZ_SHIP_BASE_THRUST_ACCEL / (base_max_speed * base_max_speed);
  const az_vector_t drag_force =
    az_vmul(ship->velocity, -drag_coeff * az_vnorm(ship->velocity));
  ship->velocity = az_vadd(ship->velocity, az_vmul(drag_force, time));

  // Activate tractor beam if necessary:
  if (controls->util && controls->fire_pressed && !ship->tractor_beam.active) {
    assert(tractor_node == NULL);
    double best_distance = AZ_TRACTOR_BEAM_MAX_RANGE;
    AZ_ARRAY_LOOP(node, state->nodes) {
      if (node->kind != AZ_NODE_NOTHING) {
        const double dist = az_vnorm(az_vsub(node->position, ship->position));
        if (dist <= best_distance) {
          tractor_node = node;
          best_distance = dist;
        }
      }
    }
    if (tractor_node != NULL) {
      ship->tractor_beam.active = true;
      ship->tractor_beam.node_uid = tractor_node->uid;
      ship->tractor_beam.distance = best_distance;
      controls->fire_pressed = false;
    }
  }
  // TODO: Apply tractor beam's velocity changes
  if (ship->tractor_beam.active) {
    assert(tractor_node != NULL);
    ship->velocity = az_vflatten(ship->velocity,
        az_vsub(ship->position, tractor_node->position));
  }

  const az_vector_t gun_position =
    az_vadd(ship->position, az_vpolar(18, ship->angle));

  // Fire projectiles:
  const double fire_cost = 20.0;
  if (controls->fire_pressed && player->energy >= fire_cost) {
    az_projectile_t *proj;
    if (az_insert_projectile(state, &proj)) {
      player->energy -= fire_cost;
      az_init_projectile(proj, AZ_PROJ_GUN_NORMAL, false, gun_position,
                         ship->angle);
      controls->fire_pressed = false;
    }
  }

  // Fire beam:
  if (controls->fire_held) {
    // Calculate where beam hits:
    az_vector_t hit_at = az_vadd(ship->position, az_vpolar(1000, ship->angle));
    az_vector_t normal = AZ_VZERO;
    bool did_hit = false;
    az_color_t hit_color = AZ_WHITE;
    AZ_ARRAY_LOOP(baddie, state->baddies) {
      if (baddie->kind == AZ_BAD_NOTHING) continue;
      if (az_ray_hits_baddie(baddie, gun_position,
                             az_vsub(hit_at, gun_position),
                             &hit_at, &normal)) {
        did_hit = true;
      }
    }
    AZ_ARRAY_LOOP(wall, state->walls) {
      if (wall->kind == AZ_WALL_NOTHING) continue;
      if (az_ray_hits_wall(wall, gun_position, az_vsub(hit_at, gun_position),
                           &hit_at, &normal)) {
        did_hit = true;
        hit_color = wall->data->color;
      }
    }
    // Add particle for the beam itself:
    az_particle_t *particle;
    if (az_insert_particle(state, &particle)) {
      particle->kind = AZ_PAR_BEAM;
      particle->color = (az_color_t){
        (az_clock_mod(6, 1, state->clock) < 3 ? 255 : 64),
        (az_clock_mod(6, 1, state->clock + 2) < 3 ? 255 : 64),
        (az_clock_mod(6, 1, state->clock + 4) < 3 ? 255 : 64),
        192};
      particle->position = gun_position;
      particle->velocity = AZ_VZERO;
      particle->angle = ship->angle;
      particle->lifetime = 0.0;
      particle->param1 = az_vnorm(az_vsub(hit_at, particle->position));
      particle->param2 = 6 + az_clock_zigzag(6, 1, state->clock);
    }
    // Add particles off of what the beam hits:
    if (did_hit) {
      if (az_insert_particle(state, &particle)) {
        particle->kind = AZ_PAR_SPECK;
        particle->color = hit_color;
        particle->position = hit_at;
        particle->velocity = az_vpolar(20.0 + 50.0 * az_random(),
                                       az_vtheta(normal) +
                                       (az_random() - 0.5) * AZ_PI);
        particle->angle = 0.0;
        particle->lifetime = 1.0;
      }
    }
  }
}

/*===========================================================================*/
