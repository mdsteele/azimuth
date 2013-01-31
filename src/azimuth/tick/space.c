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
#include <math.h>

#include "azimuth/state/planet.h"
#include "azimuth/state/space.h"
#include "azimuth/tick/baddie.h"
#include "azimuth/tick/camera.h"
#include "azimuth/tick/door.h"
#include "azimuth/tick/particle.h"
#include "azimuth/tick/pickup.h"
#include "azimuth/tick/projectile.h"
#include "azimuth/tick/script.h"
#include "azimuth/tick/ship.h"
#include "azimuth/tick/speck.h"
#include "azimuth/tick/wall.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

void az_after_entering_room(az_space_state_t *state) {
  az_set_room_visited(&state->ship.player, state->ship.player.current_room);
  const az_room_t *room =
    &state->planet->rooms[state->ship.player.current_room];
  assert(0 <= room->zone_index && room->zone_index < state->planet->num_zones);
  const az_zone_t *zone = &state->planet->zones[room->zone_index];
  state->camera.center =
    az_clamp_to_bounds(&room->camera_bounds, state->ship.position);
  az_change_music(&state->soundboard, zone->music);
  az_run_script(state, room->on_start);
}

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
        state->ship.player.current_room = key;
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
        // Record that we are now in the new room.
        az_after_entering_room(state);
      }
      break;
    case AZ_DWS_SHIFT:
      state->mode_data.doorway.progress += time / shift_time;
      // TODO: shift camera, etc.
      if (state->mode_data.doorway.progress >= 1.0) {
        state->mode_data.doorway.step = AZ_DWS_FADE_IN;
        state->mode_data.doorway.progress = 0.0;
        az_play_sound(&state->soundboard, AZ_SND_DOOR_CLOSE);
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

static void tick_game_over_mode(az_space_state_t *state, double time) {
  assert(state->mode == AZ_MODE_GAME_OVER);
  const double asplode_time = 0.5; // seconds
  const double fade_time = 2.0; // seconds
  switch (state->mode_data.game_over.step) {
    case AZ_GOS_ASPLODE:
      state->mode_data.game_over.progress += time / asplode_time;
      if (state->mode_data.game_over.progress >= 1.0) {
        state->mode_data.game_over.step = AZ_GOS_FADE_OUT;
        state->mode_data.game_over.progress = 0.0;
        az_stop_music(&state->soundboard, fade_time);
      }
      break;
    case AZ_GOS_FADE_OUT:
      state->mode_data.game_over.progress =
        fmin(1.0, state->mode_data.game_over.progress + time / fade_time);
      break;
  }
}

static void tick_pause_resume_mode(az_space_state_t *state, double time) {
  assert(state->mode == AZ_MODE_PAUSING || state->mode == AZ_MODE_RESUMING);
  const double pause_unpause_time = 0.25; // seconds
  state->mode_data.pause.progress =
    fmin(1.0, state->mode_data.pause.progress + time / pause_unpause_time);
  if (state->mode == AZ_MODE_RESUMING &&
      state->mode_data.pause.progress >= 1.0) {
    state->mode = AZ_MODE_NORMAL;
  }
}

static void tick_upgrade_mode(az_space_state_t *state, double time) {
  assert(state->mode == AZ_MODE_UPGRADE);
  const double open_close_time = 0.5; // seconds
  switch (state->mode_data.doorway.step) {
    case AZ_UGS_OPEN:
      state->mode_data.upgrade.progress += time / open_close_time;
      if (state->mode_data.upgrade.progress >= 1.0) {
        state->mode_data.upgrade.step = AZ_UGS_MESSAGE;
        state->mode_data.upgrade.progress = 0.0;
      }
      break;
    case AZ_UGS_MESSAGE:
      break;
    case AZ_UGS_CLOSE:
      state->mode_data.upgrade.progress += time / open_close_time;
      if (state->mode_data.upgrade.progress >= 1.0) {
        state->mode = AZ_MODE_NORMAL;
      }
      break;
  }
}

static void tick_message(az_message_t *message, double time) {
  message->time_remaining = fmax(0.0, message->time_remaining - time);
}

static void tick_countdown(az_countdown_t *countdown, double time) {
  if (!countdown->is_active) return;
  if (countdown->active_for < 10.0) countdown->active_for += time;
  countdown->time_remaining = fmax(0.0, countdown->time_remaining - time);
}

static void tick_pickups_walls_doors_projectiles_and_baddies(
    az_space_state_t *state, double time) {
  az_tick_pickups(state, time);
  az_tick_walls(state, time);
  az_tick_doors(state, time);
  az_tick_projectiles(state, time);
  az_tick_baddies(state, time);
}

void az_tick_space_state(az_space_state_t *state, double time) {
  if (az_ship_is_present(&state->ship) &&
      state->ship.player.shields <= AZ_SHIELDS_LOW_THRESHOLD) {
    az_loop_sound(&state->soundboard,
                  (state->ship.player.shields > AZ_SHIELDS_VERY_LOW_THRESHOLD ?
                   AZ_SND_KLAXON : AZ_SND_KLAXON_DIRE));
  }

  // If we're pausing or unpausing, nothing else should happen.
  if (state->mode == AZ_MODE_PAUSING || state->mode == AZ_MODE_RESUMING) {
    tick_pause_resume_mode(state, time);
    return;
  }

  // If we're in game over mode and the ship is asploding, go into slow-motion:
  if (state->mode == AZ_MODE_GAME_OVER &&
      state->mode_data.game_over.step == AZ_GOS_ASPLODE) {
    time *= 0.4;
  }

  state->ship.player.total_time += time;
  ++state->clock;
  az_tick_particles(state, time);
  az_tick_specks(state, time);
  switch (state->mode) {
    case AZ_MODE_NORMAL:
      az_tick_timers(state, time);
      tick_pickups_walls_doors_projectiles_and_baddies(state, time);
      az_tick_ship(state, time);
      break;
    case AZ_MODE_DOORWAY:
      tick_doorway_mode(state, time);
      if (state->mode_data.doorway.step == AZ_DWS_FADE_IN) {
        az_tick_timers(state, time);
        tick_pickups_walls_doors_projectiles_and_baddies(state, time);
        az_tick_ship(state, time);
      }
      break;
    case AZ_MODE_GAME_OVER:
      tick_game_over_mode(state, time);
      tick_pickups_walls_doors_projectiles_and_baddies(state, time);
      break;
    case AZ_MODE_PAUSING:
    case AZ_MODE_RESUMING:
      AZ_ASSERT_UNREACHABLE(); // these modes are handled above
    case AZ_MODE_SAVING:
      // TODO: maybe we should add some kind of saving animation?
      break;
    case AZ_MODE_UPGRADE:
      tick_upgrade_mode(state, time);
      break;
  }
  az_tick_camera(state, time);
  tick_message(&state->message, time);
  tick_countdown(&state->countdown, time);
}

/*===========================================================================*/
