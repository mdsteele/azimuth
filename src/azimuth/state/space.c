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

#include "azimuth/state/space.h"

#include <math.h>

#include "azimuth/vector.h"

/*===========================================================================*/

#define TURN_RATE 5.0
#define THRUST_ACCEL 500.0

static void tick_ship(az_ship_t *ship,
                      const az_controls_t *controls,
                      double time_seconds) {
  az_player_t *player = ship->player;
  const bool has_lateral = az_has_upgrade(player, AZ_UPG_LATERAL_THRUSTERS);
  const double impulse = THRUST_ACCEL * time_seconds;

  // Apply velocity:
  ship->position = az_vadd(ship->position,
                           az_vmul(ship->velocity, time_seconds));

  // Turning left:
  if (controls->left && !controls->right) {
    if (!controls->util) {
      ship->angle = fmod(ship->angle + TURN_RATE * time_seconds, AZ_TWO_PI);
    } else if (has_lateral) {
      ship->velocity = az_vadd(ship->velocity,
                               az_vpolar(impulse / 2,
                                         ship->angle - AZ_HALF_PI));
    }
  }

  // Turning right:
  if (controls->right && !controls->left) {
    if (!controls->util) {
      ship->angle = fmod(ship->angle - TURN_RATE * time_seconds, AZ_TWO_PI);
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

  // Recharge energy:
  const double charge_rate = 75.0 +
    (az_has_upgrade(player, AZ_UPG_FUSION_REACTOR) ? 75.0 : 0.0) +
    (az_has_upgrade(player, AZ_UPG_QUANTUM_REACTOR) ? 125.0 : 0.0);
  player->energy = az_dmin(player->max_energy,
                           player->energy + charge_rate * time_seconds);
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

void az_tick_space_state(az_space_state_t *state,
                         const az_controls_t *controls,
                         double time_seconds) {
  ++state->clock;

  for (int i = 0; i < AZ_MAX_PROJECTILES; ++i) {
    az_projectile_t *proj = &state->projectiles[i];
    if (proj->kind != AZ_PROJ_NOTHING) {
      proj->age += time_seconds;
      if (proj->age > 1.0) {
        proj->kind = AZ_PROJ_NOTHING;
      } else {
        proj->position = az_vadd(proj->position,
                                 az_vmul(proj->velocity, time_seconds));
      }
    }
  }

  tick_ship(&state->ship, controls, time_seconds);
  tick_camera(&state->camera, state->ship.position, time_seconds);
  tick_timer(&state->timer, time_seconds);

  const double fire_cost = 5.0;
  if (controls->fire1 && state->ship.player->energy >= fire_cost) {
    az_projectile_t *projectile;
    if (az_insert_projectile(state, &projectile)) {
      state->ship.player->energy -= fire_cost;
      projectile->kind = AZ_PROJ_GUN_NORMAL;
      projectile->position = state->ship.position;
      projectile->velocity = az_vpolar(1000.0, state->ship.angle);
    }
  }

}

bool az_insert_projectile(az_space_state_t *state,
                          az_projectile_t **projectile_out) {
  for (int i = 0; i < AZ_MAX_PROJECTILES; ++i) {
    az_projectile_t *proj = &state->projectiles[i];
    if (proj->kind == AZ_PROJ_NOTHING) {
      proj->age = 0.0;
      *projectile_out = proj;
      return true;
    }
  }
  return false;
}

/*===========================================================================*/
