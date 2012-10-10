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
#include "azimuth/state/ship.h"
#include "azimuth/state/space.h"
#include "azimuth/util/misc.h"
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
    ship->position = az_vadd(ship->position, az_vmul(ship->velocity, time));
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

  // Fire projectiles:
  const double fire_cost = 20.0;
  if (controls->fire_pressed && player->energy >= fire_cost) {
    az_projectile_t *proj;
    if (az_insert_projectile(state, &proj)) {
      player->energy -= fire_cost;
      az_init_projectile(proj, AZ_PROJ_GUN_NORMAL, false,
                         az_vadd(ship->position, az_vpolar(18.0, ship->angle)),
                         ship->angle);
      state->ship.controls.fire_pressed = false;
    }
  }
}

/*===========================================================================*/
