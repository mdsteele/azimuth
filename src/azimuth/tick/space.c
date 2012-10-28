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
#include <math.h> // for INFINITY and pow

#include "azimuth/state/space.h"
#include "azimuth/tick/baddie.h"
#include "azimuth/tick/door.h"
#include "azimuth/tick/particle.h"
#include "azimuth/tick/pickup.h"
#include "azimuth/tick/projectile.h"
#include "azimuth/tick/ship.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

static void tick_doorway_mode(az_space_state_t *state, double time) {
  assert(state->mode == AZ_MODE_DOORWAY);
  const double fade_time = 0.25; // seconds
  const double shift_time = 0.3; // seconds
  switch (state->mode_data.doorway.step) {
    case AZ_DWS_FADE_OUT:
      assert(state->mode_data.doorway.door != NULL);
      state->mode_data.doorway.progress += time / fade_time;
      if (state->mode_data.doorway.progress >= 1.0) {
        // Go on to the next step.
        state->mode_data.doorway.step =
          (state->mode_data.doorway.door->kind == AZ_DOOR_PASSAGE ?
           AZ_DWS_FADE_IN : AZ_DWS_SHIFT);
        state->mode_data.doorway.progress = 0.0;
        // Replace state with new room data.
        const az_room_key_t old_key = state->ship.player.current_room;
        const az_vector_t old_pos = state->mode_data.doorway.door->position;
        const az_room_key_t key = state->mode_data.doorway.door->destination;
        state->mode_data.doorway.door = NULL;
        az_clear_space(state);
        assert(0 <= key && key < state->planet->num_rooms);
        az_enter_room(state, &state->planet->rooms[key]);
        // Record that we are now in the new room.
        state->ship.player.current_room = key;
        az_set_room_visited(&state->ship.player, key);
        // Pick a door to exit out of.
        double best_dist = INFINITY;
        az_door_t *exit = NULL;
        AZ_ARRAY_LOOP(door, state->doors) {
          if (door->kind == AZ_DOOR_NOTHING) continue;
          if (door->destination != old_key) continue;
          const double dist = az_vnorm(az_vsub(door->position, old_pos));
          if (dist < best_dist) {
            best_dist = dist;
            exit = door;
          }
        }
        // Set new ship position.
        if (exit != NULL) {
          state->ship.position =
            az_vadd(exit->position, az_vpolar(60.0, exit->angle));
          state->ship.velocity =
            az_vpolar(0.25 * az_vnorm(state->ship.velocity), exit->angle);
          exit->openness = 1.0;
          exit->is_open = false;
          state->mode_data.doorway.door = exit;
        }
      }
      break;
    case AZ_DWS_SHIFT:
      state->mode_data.doorway.progress += time / shift_time;
      // TODO: shift camera, etc.
      if (state->mode_data.doorway.progress >= 1.0) {
        state->mode_data.doorway.step = AZ_DWS_FADE_IN;
        state->mode_data.doorway.progress = 0.0;
      }
      break;
    case AZ_DWS_FADE_IN:
      state->mode_data.doorway.progress += time / fade_time;
      if (state->mode_data.doorway.progress >= 1.0) {
        state->mode = AZ_MODE_NORMAL;
      }
      break;
  }
}

static void tick_timer(az_timer_t *timer, double time) {
  if (!timer->is_active) return;
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
  if (state->mode == AZ_MODE_DOORWAY) {
    tick_doorway_mode(state, time);
  }
  if (state->mode != AZ_MODE_DOORWAY ||
      state->mode_data.doorway.step == AZ_DWS_FADE_IN) {
    az_tick_doors(state, time);
    az_tick_projectiles(state, time);
    az_tick_baddies(state, time);
    az_tick_ship(state, time);
  }
  tick_camera(&state->camera, state->ship.position, time);
  tick_timer(&state->timer, time);
}

/*===========================================================================*/
