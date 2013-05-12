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

#include "azimuth/state/dialog.h"
#include "azimuth/state/planet.h"
#include "azimuth/state/space.h"
#include "azimuth/tick/baddie.h"
#include "azimuth/tick/camera.h"
#include "azimuth/tick/door.h"
#include "azimuth/tick/gravfield.h"
#include "azimuth/tick/node.h"
#include "azimuth/tick/object.h"
#include "azimuth/tick/particle.h"
#include "azimuth/tick/pickup.h"
#include "azimuth/tick/projectile.h"
#include "azimuth/tick/script.h"
#include "azimuth/tick/ship.h"
#include "azimuth/tick/speck.h"
#include "azimuth/tick/wall.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/random.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

// How large a radius around the ship center should be made free of
// destructible walls when we enter a room:
#define WALL_REMOVAL_RADIUS 40.0

void az_after_entering_room(az_space_state_t *state) {
  // Mark the room as visited.
  az_set_room_visited(&state->ship.player, state->ship.player.current_room);
  // Remove destructible walls too near where the ship starts (so that the ship
  // doesn't start inside a destructable wall that blocks the entrance).
  AZ_ARRAY_LOOP(wall, state->walls) {
    if (wall->kind == AZ_WALL_NOTHING) continue;
    if (wall->kind == AZ_WALL_INDESTRUCTIBLE) continue;
    if (az_circle_touches_wall(
            wall, WALL_REMOVAL_RADIUS, state->ship.position)) {
      wall->kind = AZ_WALL_NOTHING;
    }
  }
  // Clamp the camera to be within the current room's camera bounds.
  const az_room_t *room =
    &state->planet->rooms[state->ship.player.current_room];
  state->camera.center =
    az_clamp_to_bounds(&room->camera_bounds, state->ship.position);
  // Set the music and run the room script (if any).
  assert(0 <= room->zone_key && room->zone_key < state->planet->num_zones);
  const az_zone_t *zone = &state->planet->zones[room->zone_key];
  az_change_music(&state->soundboard, zone->music);
  az_run_script(state, room->on_start);
}

/*===========================================================================*/

static void tick_boss_death_mode(az_space_state_t *state, double time) {
  assert(state->mode == AZ_MODE_BOSS_DEATH);
  const double shake_time = 1.0; // seconds
  const double boom_time = 1.5; // seconds
  const double fade_time = 1.2; // seconds
  switch (state->mode_data.boss_death.step) {
    case AZ_BDS_SHAKE:
      az_shake_camera(&state->camera, 4, 4);
      assert(state->mode_data.boss_death.boss.kind != AZ_BAD_NOTHING);
      state->mode_data.boss_death.progress =
        fmin(1.0, state->mode_data.boss_death.progress + time / shake_time);
      if (state->mode_data.boss_death.progress >= 1.0) {
        state->mode_data.boss_death.step = AZ_BDS_BOOM;
        state->mode_data.boss_death.progress = 0.0;
      }
      break;
    case AZ_BDS_BOOM:
      az_shake_camera(&state->camera, 3, 3);
      assert(state->mode_data.boss_death.boss.kind != AZ_BAD_NOTHING);
      state->mode_data.boss_death.progress =
        fmin(1.0, state->mode_data.boss_death.progress + time / boom_time);
      if (state->mode_data.boss_death.progress >= 1.0) {
        state->mode_data.boss_death.step = AZ_BDS_FADE;
        state->mode_data.boss_death.progress = 0.0;
        for (int i = 0; i < 20; ++i) {
          const az_vector_t offset = {az_random(-1, 1), az_random(-1, 1)};
          if (az_vdot(offset, offset) > 1.0) continue;
          az_add_random_pickup(
              state, ~AZ_PUPF_NOTHING,
              az_vadd(state->mode_data.boss_death.boss.position,
                      az_vmul(offset, 100.0)));
        }
        state->mode_data.boss_death.boss.kind = AZ_BAD_NOTHING;
        az_run_script(state, state->mode_data.boss_death.boss.on_kill);
      }
      break;
    case AZ_BDS_FADE:
      az_shake_camera(&state->camera, 1, 1);
      assert(state->mode_data.boss_death.boss.kind == AZ_BAD_NOTHING);
      state->mode_data.boss_death.progress =
        fmin(1.0, state->mode_data.boss_death.progress + time / fade_time);
      if (state->mode_data.boss_death.progress >= 1.0) {
        state->mode = AZ_MODE_NORMAL;
      }
      break;
  }
}

static az_text_fragment_t refilled_shields_only_fragments[] = {
  {.color = {255, 255, 255, 255}, .length = 17, .chars = "Shields refilled."}
};
static az_text_line_t refilled_shields_only_lines[] = {
  {.total_length = 17, .num_fragments = 1,
   .fragments = refilled_shields_only_fragments}
};
static const az_text_t refilled_shields_only_text = {
  .num_lines = 1, .lines = refilled_shields_only_lines
};

static az_text_fragment_t refilled_shields_and_ammo_fragments[] = {
  {.color = {255, 255, 255, 255}, .length = 32,
   .chars = "Shields and ammunition refilled."}
};
static az_text_line_t refilled_shields_and_ammo_lines[] = {
  {.total_length = 32, .num_fragments = 1,
   .fragments = refilled_shields_and_ammo_fragments}
};
static const az_text_t refilled_shields_and_ammo_text = {
  .num_lines = 1, .lines = refilled_shields_and_ammo_lines
};

static void tick_console_mode(az_space_state_t *state, double time) {
  assert(state->mode == AZ_MODE_CONSOLE);
  az_node_t *node = NULL;
  if (!az_lookup_node(state, state->mode_data.console.node_uid, &node)) {
    state->mode = AZ_MODE_NORMAL;
    return;
  }
  assert(node != NULL);
  assert(node->kind == AZ_NODE_CONSOLE);
  az_ship_t *ship = &state->ship;
  az_player_t *player = &ship->player;
  double *progress = &state->mode_data.console.progress;
  const double align_time = 0.3; // seconds
  const double use_time = 1.5; // seconds
  switch (state->mode_data.console.step) {
    case AZ_CSS_ALIGN:
      assert(*progress >= 0.0);
      assert(*progress < 1.0);
      *progress = fmin(1.0, *progress + time / align_time);
      ship->position = az_vadd(
          node->position,
          az_vmul(state->mode_data.console.position_delta, 1.0 - *progress));
      ship->angle =
        az_mod2pi(node->angle + state->mode_data.console.angle_delta *
                  (1.0 - *progress));
      ship->velocity = AZ_VZERO;
      if (*progress >= 1.0) {
        state->mode_data.console.step = AZ_CSS_USE;
        *progress = 0.0;
      }
      break;
    case AZ_CSS_USE:
      assert(*progress >= 0.0);
      assert(*progress < 1.0);
      *progress = fmin(1.0, *progress + time / use_time);
      switch (node->subkind.console) {
        case AZ_CONS_COMM: break;
        case AZ_CONS_REFILL:
          player->rockets =
            az_imax(player->rockets, *progress * player->max_rockets);
          player->bombs =
            az_imax(player->bombs, *progress * player->max_bombs);
          // fallthrough
        case AZ_CONS_SAVE:
          player->shields =
            fmax(player->shields, *progress * player->max_shields);
          player->energy =
            fmax(player->energy, *progress * player->max_energy);
          break;
      }
      if (*progress >= 1.0) {
        state->mode_data.console.step = AZ_CSS_FINISH;
        *progress = 0.0;
      }
      break;
    case AZ_CSS_FINISH:
      assert(*progress == 0.0);
      if (node->subkind.console == AZ_CONS_REFILL) {
        state->message.time_remaining = 4.0;
        if (player->max_rockets == 0 && player->max_bombs == 0) {
          state->message.text = &refilled_shields_only_text;
        } else state->message.text = &refilled_shields_and_ammo_text;
      }
      state->mode = AZ_MODE_NORMAL;
      az_run_script(state, node->on_use);
      break;
  }
}

static void tick_dialog_mode(az_space_state_t *state, double time) {
  assert(state->mode == AZ_MODE_DIALOG);
  const double open_close_time = 0.5; // seconds
  const double char_time = 0.03; // seconds
  switch (state->mode_data.dialog.step) {
    case AZ_DLS_BEGIN:
      assert(state->mode_data.dialog.text == NULL);
      assert(state->mode_data.dialog.vm.script != NULL);
      state->mode_data.dialog.progress += time / open_close_time;
      if (state->mode_data.dialog.progress >= 1.0) {
        az_resume_script(state, &state->mode_data.dialog.vm);
      }
      break;
    case AZ_DLS_TALK:
      assert(state->mode_data.dialog.text != NULL);
      assert(state->mode_data.dialog.vm.script != NULL);
      state->mode_data.dialog.progress += time / char_time;
      if (state->mode_data.dialog.progress >= 1.0) {
        state->mode_data.dialog.progress = 0.0;
        const az_text_t *text = state->mode_data.dialog.text;
        ++state->mode_data.dialog.col;
        if (state->mode_data.dialog.col >=
            text->lines[state->mode_data.dialog.row].total_length) {
          state->mode_data.dialog.col = 0;
          ++state->mode_data.dialog.row;
          if (state->mode_data.dialog.row >= text->num_lines) {
            state->mode_data.dialog.step = AZ_DLS_PAUSE;
          }
        }
      }
      break;
    case AZ_DLS_PAUSE:
      assert(state->mode_data.dialog.text != NULL);
      assert(state->mode_data.dialog.vm.script != NULL);
      break;
    case AZ_DLS_END:
      assert(state->mode_data.dialog.text == NULL);
      state->mode_data.dialog.progress += time / open_close_time;
      if (state->mode_data.dialog.progress >= 1.0) {
        if (state->mode_data.dialog.vm.script != NULL) {
          az_script_vm_t vm = state->mode_data.dialog.vm;
          state->mode = AZ_MODE_NORMAL;
          az_resume_script(state, &vm);
        } else state->mode = AZ_MODE_NORMAL;
      }
      break;
  }
}

static void tick_doorway_mode(az_space_state_t *state, double time) {
  assert(state->mode == AZ_MODE_DOORWAY);
  const double fade_time = 0.25; // seconds
  const double shift_time = 0.5; // seconds
  switch (state->mode_data.doorway.step) {
    case AZ_DWS_FADE_OUT:
      assert(state->mode_data.doorway.door != NULL);
      state->mode_data.doorway.progress += time / fade_time;
      if (state->mode_data.doorway.progress >= 1.0) {
        // Go on to the next step.
        state->mode_data.doorway.step =
          (state->mode_data.doorway.door->kind == AZ_DOOR_PASSAGE ?
           AZ_DWS_FADE_IN : AZ_DWS_SHIFT);
        const az_vector_t entrance_position =
          state->mode_data.doorway.door->position;
        const double entrance_angle = state->mode_data.doorway.door->angle;
        state->mode_data.doorway.progress = 0.0;
        state->mode_data.doorway.entrance_position = entrance_position;
        state->mode_data.doorway.entrance_angle = entrance_angle;
        state->mode_data.doorway.cam_start_r = az_vnorm(state->camera.center);
        state->mode_data.doorway.cam_start_theta =
          az_vtheta(state->camera.center);
        // Replace state with new room data.
        const az_room_key_t origin_key = state->ship.player.current_room;
        const az_room_key_t dest_key =
          state->mode_data.doorway.door->destination;
        state->mode_data.doorway.door = NULL;
        az_clear_space(state);
        assert(0 <= dest_key && dest_key < state->planet->num_rooms);
        az_enter_room(state, &state->planet->rooms[dest_key]);
        state->ship.player.current_room = dest_key;
        // Pick a door to exit out of.
        double best_dist = INFINITY;
        az_door_t *exit = NULL;
        AZ_ARRAY_LOOP(door, state->doors) {
          if (door->kind == AZ_DOOR_NOTHING) continue;
          if (door->destination != origin_key) continue;
          const double dist = az_vdist(door->position, entrance_position);
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
          state->ship.angle = az_mod2pi(
              state->ship.angle + AZ_PI + exit->angle - entrance_angle);
          exit->openness = 1.0;
          exit->is_open = false;
          state->mode_data.doorway.door = exit;
        }
        // Record that we are now in the new room.
        const az_vector_t old_camera_center = state->camera.center;
        az_after_entering_room(state);
        // Set us up for the camera shift animation.
        if (exit != NULL && state->mode_data.doorway.step == AZ_DWS_SHIFT) {
          state->mode_data.doorway.cam_delta_r =
            az_vnorm(state->camera.center) -
            state->mode_data.doorway.cam_start_r;
          state->mode_data.doorway.cam_delta_theta =
            az_mod2pi(az_vtheta(state->camera.center) -
                      state->mode_data.doorway.cam_start_theta);
          state->camera.center = old_camera_center;
        } else state->mode_data.doorway.step = AZ_DWS_FADE_IN;
      }
      break;
    case AZ_DWS_SHIFT:
      assert(state->mode_data.doorway.door != NULL);
      // Increase progress:
      state->mode_data.doorway.progress =
        fmin(1.0, state->mode_data.doorway.progress + time / shift_time);
      // Shift camera position:
      state->camera.center =
        az_vpolar((state->mode_data.doorway.cam_start_r +
                   state->mode_data.doorway.cam_delta_r *
                   state->mode_data.doorway.progress),
                  (state->mode_data.doorway.cam_start_theta +
                   state->mode_data.doorway.cam_delta_theta *
                   state->mode_data.doorway.progress));
      // When progress is complete, go to the next step:
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

/*===========================================================================*/

static void tick_message(az_message_t *message, double time) {
  if (message->time_remaining == 0.0) {
    assert(message->text == NULL);
  } else {
    assert(message->time_remaining > 0.0);
    message->time_remaining -= time;
    if (message->time_remaining <= 0.0) {
      message->time_remaining = 0.0;
      message->text = NULL;
    }
  }
}

// If the global countdown timer is active, count it down.
static void tick_countdown(az_countdown_t *countdown, double time) {
  if (!countdown->is_active) return;
  assert(countdown->vm.script != NULL);
  countdown->active_for += time;
  countdown->time_remaining = fmax(0.0, countdown->time_remaining - time);
}

// Check if the global countdown timer is active and has reached zero, and if
// so, resume its suspended script.
static void check_countdown(az_space_state_t *state, double time) {
  az_countdown_t *countdown = &state->countdown;
  if (countdown->is_active && countdown->time_remaining <= 0.0) {
    countdown->is_active = false;
    az_resume_script(state, &countdown->vm);
  }
}

static void tick_most_objects(az_space_state_t *state, double time) {
  az_tick_pickups(state, time);
  az_tick_gravfields(state, time);
  az_tick_walls(state, time);
  az_tick_doors(state, time);
  az_tick_projectiles(state, time);
  az_tick_baddies(state, time);
}

static void tick_all_objects(az_space_state_t *state, double time) {
  tick_most_objects(state, time);
  // We just ticked baddies and projectiles, so the ship might've gotten blown
  // up and we could now be in game-over mode; only tick the ship if that's not
  // the case.
  if (state->mode != AZ_MODE_GAME_OVER) az_tick_ship(state, time);
  az_tick_nodes(state, time);
}

/*===========================================================================*/

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
    case AZ_MODE_BOSS_DEATH:
      tick_all_objects(state, time);
      tick_boss_death_mode(state, time);
      break;
    case AZ_MODE_CONSOLE:
      tick_console_mode(state, time);
      tick_most_objects(state, time);
      az_tick_nodes(state, time);
      break;
    case AZ_MODE_DIALOG:
      tick_dialog_mode(state, time);
      break;
    case AZ_MODE_DOORWAY:
      tick_doorway_mode(state, time);
      if (state->mode_data.doorway.step == AZ_DWS_FADE_IN) {
        az_tick_timers(state, time);
        tick_all_objects(state, time);
      }
      break;
    case AZ_MODE_GAME_OVER:
      tick_game_over_mode(state, time);
      tick_most_objects(state, time);
      az_tick_nodes(state, time);
      break;
    case AZ_MODE_NORMAL:
      az_tick_timers(state, time);
      check_countdown(state, time);
      tick_all_objects(state, time);
      break;
    case AZ_MODE_PAUSING:
    case AZ_MODE_RESUMING:
      AZ_ASSERT_UNREACHABLE(); // these modes are handled above
    case AZ_MODE_UPGRADE:
      tick_upgrade_mode(state, time);
      break;
  }
  if (!(state->mode == AZ_MODE_DOORWAY &&
        state->mode_data.doorway.step == AZ_DWS_SHIFT)) {
    const az_vector_t goal =
      (state->mode == AZ_MODE_BOSS_DEATH &&
       state->mode_data.boss_death.boss.kind != AZ_BAD_NOTHING ?
       state->mode_data.boss_death.boss.position : state->ship.position);
    az_tick_camera(state, goal, time);
  }
  tick_message(&state->message, time);
  tick_countdown(&state->countdown, time);
}

/*===========================================================================*/
