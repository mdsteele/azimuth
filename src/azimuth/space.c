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

#include "azimuth/space.h"

#include <math.h>

#include "azimuth/vector.h"

/*===========================================================================*/

#define TURN_RATE 5.0
#define THRUST_ACCEL 500.0
#define MAX_SPEED 400.0

static void az_tick_ship(az_ship_t *ship,
                         const az_controls_t *controls,
                         double time_seconds) {
  ship->position = az_vadd(ship->position,
                           az_vmul(ship->velocity, time_seconds));
  const double impulse = THRUST_ACCEL * time_seconds;
  if (controls->left && !controls->right) {
    if (controls->util) {
      ship->velocity = az_vadd(ship->velocity,
                             az_vpolar(impulse / 2,
                                       ship->angle - AZ_HALF_PI));
    } else {
      ship->angle = fmod(ship->angle + TURN_RATE * time_seconds, AZ_TWO_PI);
    }
  }
  if (controls->right && !controls->left) {
    if (controls->util) {
      ship->velocity = az_vadd(ship->velocity,
                             az_vpolar(impulse / 2,
                                       ship->angle + AZ_HALF_PI));
    } else {
      ship->angle = fmod(ship->angle - TURN_RATE * time_seconds, AZ_TWO_PI);
    }
  }
  if (controls->up && !controls->down) {
    ship->velocity = az_vadd(ship->velocity,
                             az_vpolar((controls->util ? -impulse/2 : impulse),
                                       ship->angle));
  }
  if (controls->down && !controls->up) {
    const double speed = az_vnorm(ship->velocity);
    if (speed <= impulse) {
      ship->velocity = AZ_VZERO;
    } else {
      ship->velocity = az_vmul(ship->velocity, (speed - impulse) / speed);
    }
  }
  const double speed = az_vnorm(ship->velocity);
  if (speed > MAX_SPEED) {
    ship->velocity = az_vmul(ship->velocity, MAX_SPEED / speed);
  }
}

static void az_tick_timer(az_timer_t *timer, double time_seconds) {
  if (timer->active_for < 0.0) return;
  if (timer->active_for < 10.0) timer->active_for += time_seconds;
  timer->time_remaining = az_dmax(0.0, timer->time_remaining - time_seconds);
}

void az_tick_space_state(az_space_state_t *state,
                         const az_controls_t *controls,
                         double time_seconds) {
  ++state->clock;
  az_tick_ship(&state->ship, controls, time_seconds);
  state->camera = state->ship.position;
  az_tick_timer(&state->timer, time_seconds);
}

/*===========================================================================*/
