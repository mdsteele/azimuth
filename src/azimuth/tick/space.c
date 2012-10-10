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

#include <math.h> // for pow

#include "azimuth/state/space.h"
#include "azimuth/tick/baddie.h"
#include "azimuth/tick/particle.h"
#include "azimuth/tick/pickup.h"
#include "azimuth/tick/projectile.h"
#include "azimuth/tick/ship.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

static void tick_timer(az_timer_t *timer, double time) {
  if (timer->active_for < 0.0) return;
  if (timer->active_for < 10.0) timer->active_for += time;
  timer->time_remaining = az_dmax(0.0, timer->time_remaining - time);
}

static void tick_camera(az_vector_t *camera, az_vector_t towards,
                        double time) {
  const double tracking_base = 0.00003; // smaller = faster tracking
  const az_vector_t difference = az_vsub(towards, *camera);
  const az_vector_t change =
    az_vmul(difference, 1.0 - pow(tracking_base, time));
  *camera = az_vadd(*camera, change);
}

void az_tick_space_state(az_space_state_t *state, double time) {
  state->ship.player.total_time += time;
  ++state->clock;
  az_tick_particles(state, time);
  az_tick_pickups(state, time);
  az_tick_projectiles(state, time);
  az_tick_baddies(state, time);
  az_tick_ship(state, time);
  tick_camera(&state->camera, state->ship.position, time);
  tick_timer(&state->timer, time);
}

/*===========================================================================*/
