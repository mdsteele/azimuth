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
#include "azimuth/tick/cutscene.h"
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
#include "azimuth/util/warning.h"

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
  const az_room_t *room =
    &state->planet->rooms[state->ship.player.current_room];
  state->camera.quake_vert = 0.0;
  state->camera.r_max_override = 0.0;
  state->console_help_message_cooldown = 0.0;
  // Run the room script (if any).
  az_run_script(state, room->on_start);
  state->darkness = state->dark_goal;
  AZ_ARRAY_LOOP(door, state->doors) {
    if (door->kind == AZ_DOOR_LOCKED) door->lockedness = 1.0 - door->openness;
  }
  // Clamp the camera to be within the current room's camera bounds.
  state->camera.center = az_clamp_to_bounds_with_override(
      &room->camera_bounds, state->ship.position,
      state->camera.r_max_override);
}

/*===========================================================================*/

static void tick_boss_death_mode(az_space_state_t *state, double time) {
  assert(state->mode == AZ_MODE_BOSS_DEATH);
  az_boss_death_mode_data_t *mode_data = &state->boss_death_mode;
  const double shake_time = 1.5; // seconds
  const double boom_time = 1.0; // seconds
  const double fade_time = 2.5; // seconds
  switch (mode_data->step) {
    case AZ_BDS_SHAKE:
      assert(mode_data->boss.kind != AZ_BAD_NOTHING);
      az_shake_camera(&state->camera, 4, 4);
      mode_data->progress = fmin(1.0, mode_data->progress + time / shake_time);
      if (mode_data->progress >= 1.0) {
        mode_data->step = AZ_BDS_BOOM;
        mode_data->progress = 0.0;
        if (state->boss_death_mode.boss.kind == AZ_BAD_OTH_GUNSHIP) {
          state->camera.wobble_goal = boom_time;
          az_particle_t *particle;
          if (az_insert_particle(state, &particle)) {
            particle->kind = AZ_PAR_NPS_PORTAL;
            particle->color = (az_color_t){128, 64, 255, 255};
            particle->position = state->boss_death_mode.boss.position;
            particle->velocity = AZ_VZERO;
            particle->angle = 0.0;
            particle->lifetime = 2.0 * boom_time;
            particle->param1 = 50.0 * sqrt(2.0 * boom_time);
          }
          az_play_sound(&state->soundboard, AZ_SND_NPS_PORTAL);
        } else {
          az_play_sound(&state->soundboard, AZ_SND_BOSS_EXPLODE);
        }
      }
      break;
    case AZ_BDS_BOOM:
      assert(mode_data->boss.kind != AZ_BAD_NOTHING);
      az_shake_camera(&state->camera, 3, 3);
      mode_data->progress = fmin(1.0, mode_data->progress + time / boom_time);
      if (mode_data->progress >= 1.0) {
        mode_data->step = AZ_BDS_FADE;
        mode_data->progress = 0.0;
        for (int i = 0; i < 20; ++i) {
          az_pickup_t *pickup = az_add_random_pickup(
              state, mode_data->boss.data->potential_pickups,
              az_vadd(mode_data->boss.position,
                      az_random_point_in_circle(100.0)));
          if (pickup != NULL) {
            pickup->time_remaining = 1e9;
          }
        }
        AZ_ARRAY_LOOP(uuid, mode_data->boss.cargo_uuids) {
          az_object_t object;
          if (az_lookup_object(state, *uuid, &object)) {
            az_kill_object(state, &object);
          }
        }
        AZ_ARRAY_LOOP(baddie, mode_data->legs) baddie->kind = AZ_BAD_NOTHING;
        mode_data->boss.kind = AZ_BAD_NOTHING;
        az_run_script(state, mode_data->boss.on_kill);
      }
      break;
    case AZ_BDS_FADE:
      assert(mode_data->boss.kind == AZ_BAD_NOTHING);
      az_shake_camera(&state->camera, 1, 1);
      mode_data->progress = fmin(1.0, mode_data->progress + time / fade_time);
      if (mode_data->progress >= 1.0) {
        state->mode = AZ_MODE_NORMAL;
      }
      break;
  }
}

static const char refilled_shields_only_paragraph[] = "Shields refilled.";
static const char refilled_shields_and_ammo_paragraph[] =
  "Shields and ammunition refilled.";

static void tick_console_mode(az_space_state_t *state, double time) {
  assert(state->mode == AZ_MODE_CONSOLE);
  az_console_mode_data_t *mode_data = &state->console_mode;
  az_node_t *node = NULL;
  if (!az_lookup_node(state, mode_data->node_uid, &node)) {
    state->mode = AZ_MODE_NORMAL;
    return;
  }
  assert(node != NULL);
  assert(node->kind == AZ_NODE_CONSOLE);
  az_ship_t *ship = &state->ship;
  assert(az_ship_is_alive(ship));
  assert(ship->thrusters == AZ_THRUST_NONE);
  az_player_t *player = &ship->player;
  const double align_time = 0.3; // seconds
  const double use_time = 1.5; // seconds
  switch (mode_data->step) {
    case AZ_CSS_ALIGN:
      assert(mode_data->progress >= 0.0);
      assert(mode_data->progress < 1.0);
      mode_data->progress = fmin(1.0, mode_data->progress + time / align_time);
      ship->position = az_vadd(
          node->position,
          az_vmul(mode_data->position_delta, 1.0 - mode_data->progress));
      ship->angle =
        az_mod2pi(node->angle + mode_data->angle_delta *
                  (1.0 - mode_data->progress));
      ship->velocity = AZ_VZERO;
      if (mode_data->progress >= 1.0) {
        mode_data->step = AZ_CSS_USE;
        mode_data->progress = 0.0;
        mode_data->start_shields = player->shields;
        mode_data->start_energy = player->energy;
        mode_data->start_rockets = player->rockets;
        mode_data->start_bombs = player->bombs;
        az_sound_key_t sound_key = AZ_SND_NOTHING;
        switch (node->subkind.console) {
          case AZ_CONS_COMM:   sound_key = AZ_SND_USE_COMM_CONSOLE;   break;
          case AZ_CONS_REFILL: sound_key = AZ_SND_USE_REFILL_CONSOLE; break;
          case AZ_CONS_SAVE:   sound_key = AZ_SND_USE_SAVE_CONSOLE;   break;
        }
        az_play_sound(&state->soundboard, sound_key);
      }
      break;
    case AZ_CSS_USE:
      assert(mode_data->progress >= 0.0);
      assert(mode_data->progress < 1.0);
      mode_data->progress = fmin(1.0, mode_data->progress + time / use_time);
      switch (node->subkind.console) {
        case AZ_CONS_COMM: break;
        case AZ_CONS_REFILL:
          player->rockets = mode_data->start_rockets +
            mode_data->progress *
            (player->max_rockets - mode_data->start_rockets);
          player->bombs = mode_data->start_bombs +
            mode_data->progress * (player->max_bombs - mode_data->start_bombs);
          // fallthrough
        case AZ_CONS_SAVE:
          player->shields =
            fmin(player->max_shields, mode_data->start_shields +
                 mode_data->progress *
                 (player->max_shields - mode_data->start_shields));
          player->energy =
            fmin(player->max_energy, mode_data->start_energy +
                 mode_data->progress *
                 (player->max_energy - mode_data->start_energy));
          break;
      }
      if (mode_data->progress >= 1.0) {
        // We need to be in normal mode to run the script.
        state->mode = AZ_MODE_NORMAL;
        az_run_script(state, node->on_use);
        // Don't perform the console action if the script put us into some
        // other mode (e.g. game over mode).
        if (state->mode == AZ_MODE_NORMAL) {
          if (node->subkind.console == AZ_CONS_SAVE) {
            state->mode = AZ_MODE_CONSOLE;
            mode_data->step = AZ_CSS_SAVE;
            mode_data->progress = 0.0;
          } else if (node->subkind.console == AZ_CONS_REFILL) {
            if (player->max_rockets == 0 && player->max_bombs == 0) {
              az_set_message(state, refilled_shields_only_paragraph);
            } else az_set_message(state, refilled_shields_and_ammo_paragraph);
          }
        }
      }
      break;
    case AZ_CSS_SAVE:
      assert(mode_data->progress == 0.0);
      state->mode = AZ_MODE_NORMAL;
      break;
  }
}

static void tick_dialogue(az_space_state_t *state, double time) {
  az_dialogue_state_t *dialogue = &state->dialogue;
  assert(dialogue->step != AZ_DLS_INACTIVE);
  assert(dialogue->step != AZ_DLS_BLANK);
  assert(!dialogue->hidden);
  const double open_close_time = 0.5; // seconds
  const double char_time = 0.03; // seconds
  switch (dialogue->step) {
    case AZ_DLS_INACTIVE:
    case AZ_DLS_BLANK:
      AZ_ASSERT_UNREACHABLE();
    case AZ_DLS_BEGIN:
      assert(dialogue->paragraph == NULL);
      assert(state->sync_vm.script != NULL);
      dialogue->progress += time / open_close_time;
      if (dialogue->progress >= 1.0) {
        az_resume_script(state, &state->sync_vm);
      }
      break;
    case AZ_DLS_TALK:
      assert(dialogue->paragraph != NULL);
      assert(dialogue->chars_to_print < dialogue->paragraph_length);
      assert(state->sync_vm.script != NULL);
      dialogue->progress += time / char_time;
      if (dialogue->progress >= 1.0 || state->skip.active) {
        dialogue->progress = 0.0;
        ++dialogue->chars_to_print;
        if (dialogue->chars_to_print == dialogue->paragraph_length) {
          dialogue->step = AZ_DLS_WAIT;
        }
      }
      break;
    case AZ_DLS_WAIT:
      assert(dialogue->paragraph != NULL);
      assert(dialogue->chars_to_print == dialogue->paragraph_length);
      assert(state->sync_vm.script != NULL);
      if (state->skip.active) az_resume_script(state, &state->sync_vm);
      break;
    case AZ_DLS_END:
      assert(dialogue->paragraph == NULL);
      assert(dialogue->paragraph_length == 0);
      assert(dialogue->chars_to_print == 0);
      dialogue->progress += time / open_close_time;
      if (dialogue->progress >= 1.0) {
        *dialogue = (az_dialogue_state_t){ .step = AZ_DLS_INACTIVE };
        if (state->sync_vm.script != NULL) {
          az_resume_script(state, &state->sync_vm);
        }
      }
      break;
  }
}

static void tick_doorway_mode(az_space_state_t *state, double time) {
  assert(state->mode == AZ_MODE_DOORWAY);
  az_doorway_mode_data_t *mode_data = &state->doorway_mode;
  const double fade_time = 0.25; // seconds
  const double shift_time = 0.5; // seconds
  switch (mode_data->step) {
    case AZ_DWS_FADE_OUT:
      assert(mode_data->entrance.kind != AZ_DOOR_NOTHING);
      assert(mode_data->entrance.kind != AZ_DOOR_FORCEFIELD);
      assert(mode_data->exit.kind == AZ_DOOR_NOTHING);
      mode_data->progress += time / fade_time;
      if (mode_data->progress >= 1.0) {
        // Replace state with new room data.
        const az_room_key_t origin_key = state->ship.player.current_room;
        const az_zone_key_t old_zone_key =
          state->planet->rooms[origin_key].zone_key;
        const az_room_key_t dest_key = mode_data->destination;
        az_clear_space(state);
        assert(0 <= dest_key && dest_key < state->planet->num_rooms);
        const az_room_t *new_room = &state->planet->rooms[dest_key];
        const az_zone_key_t new_zone_key = new_room->zone_key;
        az_enter_room(state, new_room);
        state->ship.player.current_room = dest_key;
        // Pick a door to exit out of.
        double best_dist = INFINITY;
        az_door_t *exit = NULL;
        AZ_ARRAY_LOOP(door, state->doors) {
          if (door->kind == AZ_DOOR_NOTHING) continue;
          if (door->kind == AZ_DOOR_FORCEFIELD) continue;
          if (door->destination != origin_key) continue;
          const double dist =
            az_vdist(door->position, mode_data->entrance.position);
          if (dist < best_dist) {
            best_dist = dist;
            exit = door;
          }
        }
        // Set new ship position.
        if (exit == NULL) {
          AZ_WARNING_ALWAYS("No exit when entering room %d from room %d\n",
                            (int)dest_key, (int)origin_key);
          state->ship.position = az_vpolar(new_room->camera_bounds.min_r,
                                           new_room->camera_bounds.min_theta);
        } else {
          state->ship.position =
            az_vadd(exit->position, az_vpolar(60.0, exit->angle));
          state->ship.velocity =
            az_vpolar(0.25 * az_vnorm(state->ship.velocity), exit->angle);
          state->ship.angle =
            az_mod2pi(state->ship.angle + AZ_PI + exit->angle -
                      mode_data->entrance.angle);
          if (exit->kind == AZ_DOOR_ALWAYS_OPEN) {
            assert(exit->is_open);
            assert(exit->openness == 1.0);
          } else {
            exit->openness = 1.0;
            exit->lockedness = 0.0;
            exit->is_open = false;
          }
          mode_data->exit.kind = exit->kind;
          mode_data->exit.uid = exit->uid;
          mode_data->exit.position = exit->position;
          mode_data->exit.angle = exit->angle;
        }
        // If entering a new zone, display a message.
        if (new_zone_key != old_zone_key) {
          const az_zone_t *new_zone = &state->planet->zones[new_zone_key];
          az_set_message(state, new_zone->entering_message);
        }
        // Record that we are now in the new room.  We have to set the ship
        // position _before_ this, because this will remove destructable walls
        // that are near the ship.
        const az_vector_t old_camera_center = state->camera.center;
        az_after_entering_room(state);
        const az_vector_t new_camera_center = state->camera.center;
        // Go on to the next step.
        if (mode_data->entrance.kind != AZ_DOOR_PASSAGE && exit != NULL &&
            mode_data->exit.kind != AZ_DOOR_PASSAGE) {
          assert(mode_data->exit.kind != AZ_DOOR_NOTHING);
          mode_data->step = AZ_DWS_SHIFT;
          mode_data->progress = 0.0;
          mode_data->cam_start_r = az_vnorm(old_camera_center);
          mode_data->cam_start_theta = az_vtheta(old_camera_center);
          mode_data->cam_delta_r =
            az_vnorm(new_camera_center) - mode_data->cam_start_r;
          mode_data->cam_delta_theta =
            az_mod2pi(az_vtheta(new_camera_center) -
                      mode_data->cam_start_theta);
          state->camera.center = old_camera_center;
        } else {
          mode_data->step = AZ_DWS_FADE_IN;
          mode_data->progress = 0.0;
        }
      }
      break;
    case AZ_DWS_SHIFT:
      assert(mode_data->entrance.kind != AZ_DOOR_NOTHING);
      assert(mode_data->entrance.kind != AZ_DOOR_PASSAGE);
      assert(mode_data->entrance.kind != AZ_DOOR_FORCEFIELD);
      assert(mode_data->exit.kind != AZ_DOOR_NOTHING);
      assert(mode_data->exit.kind != AZ_DOOR_PASSAGE);
      assert(mode_data->exit.kind != AZ_DOOR_FORCEFIELD);
      // Increase progress:
      mode_data->progress = fmin(1.0, mode_data->progress + time / shift_time);
      // Shift camera position:
      state->camera.center =
        az_vpolar((mode_data->cam_start_r +
                   mode_data->cam_delta_r * mode_data->progress),
                  (mode_data->cam_start_theta +
                   mode_data->cam_delta_theta * mode_data->progress));
      // When progress is complete, go to the next step:
      if (mode_data->progress >= 1.0) {
        mode_data->step = AZ_DWS_FADE_IN;
        mode_data->progress = 0.0;
        if (mode_data->exit.kind != AZ_DOOR_ALWAYS_OPEN) {
          az_play_sound(&state->soundboard, AZ_SND_DOOR_CLOSE);
        }
      }
      break;
    case AZ_DWS_FADE_IN:
      mode_data->progress += time / fade_time;
      if (mode_data->progress >= 1.0) {
        state->mode = AZ_MODE_NORMAL;
      }
      break;
  }
}

static void tick_game_over_mode(az_space_state_t *state, double time) {
  assert(state->mode == AZ_MODE_GAME_OVER);
  az_game_over_mode_data_t *mode_data = &state->game_over_mode;
  const double asplode_time = 0.5; // seconds
  const double fade_time = 2.0; // seconds
  switch (mode_data->step) {
    case AZ_GOS_ASPLODE:
      mode_data->progress += time / asplode_time;
      if (mode_data->progress >= 1.0) {
        mode_data->step = AZ_GOS_FADE_OUT;
        mode_data->progress = 0.0;
        az_stop_music(&state->soundboard, fade_time);
      }
      break;
    case AZ_GOS_FADE_OUT:
      mode_data->progress = fmin(1.0, mode_data->progress + time / fade_time);
      break;
  }
}

static void tick_global_fade(az_space_state_t *state, double time) {
  const double fade_time = 1.0;
  az_global_fade_state_t *global_fade = &state->global_fade;
  switch (global_fade->step) {
    case AZ_GFS_INACTIVE: break;
    case AZ_GFS_FADE_OUT:
      assert(state->sync_vm.script != NULL);
      global_fade->fade_alpha += time / fade_time;
      if (global_fade->fade_alpha >= 1.0) {
        global_fade->step = AZ_GFS_INACTIVE;
        global_fade->fade_alpha = 0.0;
        if (state->dialogue.step != AZ_DLS_INACTIVE) {
          state->dialogue.hidden = true;
        }
        az_resume_script(state, &state->sync_vm);
      }
      break;
    case AZ_GFS_FADE_IN:
      assert(state->sync_vm.script != NULL);
      global_fade->fade_alpha -= time / fade_time;
      if (global_fade->fade_alpha <= 0.0) {
        global_fade->step = AZ_GFS_INACTIVE;
        global_fade->fade_alpha = 0.0;
        az_resume_script(state, &state->sync_vm);
      }
      break;
  }
}

static void tick_monologue(az_space_state_t *state, double time) {
  az_monologue_state_t *monologue = &state->monologue;
  assert(monologue->step != AZ_MLS_INACTIVE);
  assert(monologue->step != AZ_MLS_BLANK);
  const double open_close_time = 0.5; // seconds
  const double char_time = 0.05; // seconds
  switch (monologue->step) {
    case AZ_MLS_INACTIVE:
    case AZ_MLS_BLANK:
      AZ_ASSERT_UNREACHABLE();
    case AZ_MLS_BEGIN:
      assert(monologue->paragraph == NULL);
      assert(state->sync_vm.script != NULL);
      monologue->progress += time / open_close_time;
      if (monologue->progress >= 1.0) {
        az_resume_script(state, &state->sync_vm);
      }
      break;
    case AZ_MLS_TALK:
      assert(monologue->paragraph != NULL);
      assert(monologue->chars_to_print < monologue->paragraph_length);
      assert(state->sync_vm.script != NULL);
      monologue->progress += time / char_time;
      if (monologue->progress >= 1.0 || state->skip.active) {
        monologue->progress = 0.0;
        ++monologue->chars_to_print;
        if (monologue->chars_to_print == monologue->paragraph_length) {
          monologue->step = AZ_MLS_WAIT;
        }
      }
      break;
    case AZ_MLS_WAIT:
      assert(monologue->paragraph != NULL);
      assert(monologue->chars_to_print == monologue->paragraph_length);
      assert(state->sync_vm.script != NULL);
      if (state->skip.active) az_resume_script(state, &state->sync_vm);
      break;
    case AZ_MLS_END:
      assert(monologue->paragraph == NULL);
      assert(monologue->paragraph_length == 0);
      assert(monologue->chars_to_print == 0);
      monologue->progress += time / open_close_time;
      if (monologue->progress >= 1.0) {
        *monologue = (az_monologue_state_t){ .step = AZ_MLS_INACTIVE };
        if (state->sync_vm.script != NULL) {
          az_resume_script(state, &state->sync_vm);
        }
      }
      break;
  }
}

static void tick_pausing_mode(az_space_state_t *state, double time) {
  assert(state->mode == AZ_MODE_PAUSING);
  az_pausing_mode_data_t *mode_data = &state->pausing_mode;
  const double fade_time = 0.25; // seconds
  switch (mode_data->step) {
    case AZ_PSS_FADE_OUT:
      mode_data->fade_alpha =
        fmin(1.0, mode_data->fade_alpha + time / fade_time);
      break;
    case AZ_PSS_FADE_IN:
      mode_data->fade_alpha =
        fmax(0.0, mode_data->fade_alpha - time / fade_time);
      if (mode_data->fade_alpha == 0.0) state->mode = AZ_MODE_NORMAL;
      break;
  }
}

static void tick_sync_timer(az_space_state_t *state, double time) {
  assert(state->sync_timer.is_active);
  assert(state->sync_timer.time_remaining > 0.0);
  assert(state->sync_vm.script != NULL);
  if (state->skip.active) state->sync_timer.time_remaining = 0.0;
  else state->sync_timer.time_remaining -= time;
  if (state->sync_timer.time_remaining <= 0.0) {
    state->sync_timer.is_active = false;
    state->sync_timer.time_remaining = 0.0;
    az_resume_script(state, &state->sync_vm);
  }
}

static void tick_upgrade_mode(az_space_state_t *state, double time) {
  assert(state->mode == AZ_MODE_UPGRADE);
  az_upgrade_mode_data_t *mode_data = &state->upgrade_mode;
  const double open_close_time = 0.5; // seconds
  switch (mode_data->step) {
    case AZ_UGS_OPEN:
      assert(mode_data->progress >= 0.0);
      assert(mode_data->progress < 1.0);
      mode_data->progress += time / open_close_time;
      if (mode_data->progress >= 1.0) {
        mode_data->step = AZ_UGS_MESSAGE;
        mode_data->progress = 0.0;
      }
      break;
    case AZ_UGS_MESSAGE:
      assert(mode_data->progress == 0.0);
      // Do nothing while we wait for the player to press a key.
      break;
    case AZ_UGS_CLOSE:
      assert(mode_data->progress >= 0.0);
      assert(mode_data->progress < 1.0);
      mode_data->progress += time / open_close_time;
      if (mode_data->progress >= 1.0) {
        state->mode = AZ_MODE_NORMAL;
      }
      break;
  }
}

/*===========================================================================*/

static void tick_darkness(az_space_state_t *state, double time) {
  if (state->darkness == state->dark_goal) return;
  const double tracking_base = 0.05; // smaller = faster tracking
  const double change =
    (state->dark_goal - state->darkness) * (1.0 - pow(tracking_base, time));
  if (fabs(change) > 0.001) {
    state->darkness = fmin(fmax(0.0, state->darkness + change), 1.0);
  } else state->darkness = state->dark_goal;
}

static void tick_message(az_message_t *message, double time) {
  if (message->time_remaining == 0.0) {
    assert(message->paragraph == NULL);
    assert(message->num_lines == 0);
  } else {
    assert(message->time_remaining > 0.0);
    message->time_remaining -= time;
    if (message->time_remaining <= 0.0) {
      message->time_remaining = 0.0;
      message->paragraph = NULL;
      message->num_lines = 0;
    }
  }
}

static void tick_console_help(az_space_state_t *state, double time) {
  state->console_help_message_cooldown =
    fmax(0.0, state->console_help_message_cooldown - time);
  if (state->console_help_message_cooldown <= 0.0) {
    AZ_ARRAY_LOOP(node, state->nodes) {
      if (node->kind != AZ_NODE_CONSOLE) continue;
      if (node->status != AZ_NS_READY) continue;
      switch (node->subkind.console) {
        case AZ_CONS_COMM:
          az_set_message(state, "Press [$t] to use the console.");
          break;
        case AZ_CONS_REFILL:
          az_set_message(state, "Press [$t] to refill shields/ammo.");
          break;
        case AZ_CONS_SAVE:
          az_set_message(state, "Press [$t] to use the save point.");
          break;
      }
      state->console_help_message_cooldown = 10.0;
      break;
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

static void tick_nuke(az_space_state_t *state, double time) {
  if (!state->nuke.active) return;
  state->nuke.rho += 700.0 * time;
  // Kill everything (baddies and ship) below the shockwave:
  AZ_ARRAY_LOOP(baddie, state->baddies) {
    if (baddie->kind == AZ_BAD_NOTHING) continue;
    if (az_baddie_has_flag(baddie, AZ_BADF_INCORPOREAL)) continue;
    if (az_vwithin(baddie->position, AZ_VZERO, state->nuke.rho)) {
      az_kill_baddie(state, baddie);
    }
  }
  if (az_ship_is_alive(&state->ship) &&
      az_vwithin(state->ship.position, AZ_VZERO, state->nuke.rho)) {
    az_kill_ship(state);
  }
  // Create minor explosions along the way:
  const double center_theta = az_vtheta(state->camera.center);
  const double theta_span =
    atan(AZ_SCREEN_WIDTH / az_vnorm(state->camera.center));
  const double step = 0.1 * theta_span;
  for (double theta = center_theta - theta_span;
       theta < center_theta + theta_span; theta += step) {
    if (az_random(0, 1) < 1.0 - pow(0.5, 3.0 * time)) {
      az_add_projectile(state, AZ_PROJ_PLANETARY_EXPLOSION,
                        az_vpolar(state->nuke.rho,
                                  theta + step * az_random(-0.5, 0.5)),
                        0, 1, AZ_NULL_UID);
    }
  }
}

static void tick_most_objects(az_space_state_t *state, double time) {
  tick_darkness(state, time);
  az_tick_pickups(state, time);
  az_tick_gravfields(state, time);
  az_tick_walls(state, time);
  az_tick_doors(state, time);
  az_tick_projectiles(state, time);
  tick_nuke(state, time);
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

// Hold the ship's persisted sounds for a frame while we're effectively paused
// (e.g. for PAUSING mode or DOORWAY mode) so that they don't implicitly reset.
static void hold_ship_sounds(az_space_state_t *state) {
  az_hold_sound(&state->soundboard, AZ_SND_CHARGING_GUN);
  az_hold_sound(&state->soundboard, AZ_SND_CHARGING_ORDNANCE);
  az_hold_sound(&state->soundboard, AZ_SND_CPLUS_ACTIVE);
}

/*===========================================================================*/

void az_tick_space_state(az_space_state_t *state, double time) {
  // Cool down skip timer.
  if (state->skip.allowed) {
    assert(state->sync_vm.script != NULL);
    if (!state->skip.active) {
      state->skip.cooldown = fmax(0.0, state->skip.cooldown - time);
    } else assert(state->skip.cooldown == 0.0);
  } else {
    assert(!state->skip.active);
    assert(state->skip.cooldown == 0.0);
  }

  // Loop a klaxon sound if the countdown timer is active.
  if (state->countdown.is_active) {
    az_loop_sound(&state->soundboard,
                  (state->countdown.time_remaining >
                   AZ_COUNTDOWN_TIME_REMAINING_LOW ?
                   AZ_SND_KLAXON_COUNTDOWN : AZ_SND_KLAXON_COUNTDOWN_LOW));
  }

  // Loop a klaxon sound if the ship's shields are low.
  if (az_ship_is_alive(&state->ship) &&
      state->ship.player.shields <= AZ_SHIELDS_LOW_THRESHOLD) {
    az_loop_sound(&state->soundboard,
                  (state->ship.player.shields > AZ_SHIELDS_VERY_LOW_THRESHOLD ?
                   AZ_SND_KLAXON_SHIELDS_LOW :
                   AZ_SND_KLAXON_SHIELDS_VERY_LOW));
  }

  // If we're pausing/unpausing, nothing else should happen.
  if (state->mode == AZ_MODE_PAUSING) {
    hold_ship_sounds(state);
    tick_pausing_mode(state, time);
    return;
  }

  // Advance the animation clock.
  ++state->clock;

  // If we're fading the whole screen in or out, do that and then stop.
  if (state->global_fade.step != AZ_GFS_INACTIVE) {
    if (state->nuke.active) {
      az_tick_particles(state, time);
      az_tick_specks(state, time);
      az_tick_projectiles(state, time);
      tick_nuke(state, time);
    }
    tick_global_fade(state, time);
    return;
  }

  // If we're watching a cutscene, allow dialogue and timer scripts to proceed,
  // but don't do anything else.
  if (state->cutscene.scene != AZ_SCENE_NOTHING) {
    if (state->cutscene.scene == state->cutscene.next) {
      if (state->sync_timer.is_active) {
        tick_sync_timer(state, time);
      } else if (state->monologue.step != AZ_MLS_INACTIVE) {
        tick_monologue(state, time);
      } else if (state->dialogue.step != AZ_DLS_INACTIVE) {
        tick_dialogue(state, time);
      } else assert(false);
    }
    az_tick_cutscene(state, time);
    return;
  }

  // If we're in game over mode and the ship is asploding, go into slow-motion:
  if (state->mode == AZ_MODE_GAME_OVER &&
      state->game_over_mode.step == AZ_GOS_ASPLODE) {
    time *= 0.4;
  }

  // These ticks happen even during dialogue/monologue.
  az_tick_particles(state, time);
  az_tick_specks(state, time);
  tick_message(&state->message, time);
  tick_countdown(&state->countdown, time);

  // If there's a synchronous timer, tick that.
  if (state->sync_timer.is_active) {
    tick_sync_timer(state, time);
    return;
  } else assert(state->sync_timer.time_remaining == 0.0);

  // If we're in monologue, advance the monologue and then stop.
  if (state->monologue.step != AZ_MLS_INACTIVE) {
    tick_monologue(state, time);
    return;
  } else assert(state->monologue.paragraph == NULL);

  // If we're in dialogue, advance the dialogue and then stop.
  if (state->dialogue.step != AZ_DLS_INACTIVE) {
    az_tick_doors(state, time);
    tick_dialogue(state, time);
    return;
  } else assert(state->dialogue.paragraph == NULL);

  assert(!state->skip.allowed);

  // Advance the speedrun timer.
  if (state->mode != AZ_MODE_UPGRADE && !state->ship.autopilot.enabled) {
    state->ship.player.total_time += time;
  }

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
    case AZ_MODE_DOORWAY:
      tick_doorway_mode(state, time);
      if (state->doorway_mode.step == AZ_DWS_FADE_IN) {
        tick_all_objects(state, time);
        az_tick_timers(state, time);
      } else hold_ship_sounds(state);
      break;
    case AZ_MODE_GAME_OVER:
      tick_game_over_mode(state, time);
      tick_most_objects(state, time);
      az_tick_nodes(state, time);
      break;
    case AZ_MODE_NORMAL:
      tick_console_help(state, time);
      tick_all_objects(state, time);
      az_tick_timers(state, time);
      check_countdown(state, time);
      break;
    case AZ_MODE_PAUSING:
      AZ_ASSERT_UNREACHABLE(); // this mode is handled above
    case AZ_MODE_UPGRADE:
      hold_ship_sounds(state);
      tick_upgrade_mode(state, time);
      break;
  }
  if (!(state->mode == AZ_MODE_DOORWAY &&
        state->doorway_mode.step == AZ_DWS_SHIFT)) {
    const az_vector_t goal =
      (state->mode == AZ_MODE_BOSS_DEATH &&
       state->boss_death_mode.boss.kind != AZ_BAD_NOTHING ?
       state->boss_death_mode.boss.position : state->ship.position);
    az_tick_camera(state, goal, time);
  }
}

/*===========================================================================*/
