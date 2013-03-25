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

#include "azimuth/control/space.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h> // for sprintf
#include <string.h>

#include "azimuth/control/paused.h"
#include "azimuth/gui/audio.h"
#include "azimuth/gui/event.h"
#include "azimuth/gui/screen.h"
#include "azimuth/state/dialog.h"
#include "azimuth/state/planet.h"
#include "azimuth/state/player.h"
#include "azimuth/state/save.h"
#include "azimuth/state/space.h"
#include "azimuth/system/resource.h"
#include "azimuth/tick/script.h"
#include "azimuth/tick/space.h"
#include "azimuth/util/misc.h"
#include "azimuth/view/space.h"

/*===========================================================================*/

static az_text_fragment_t save_failed_fragments[] = {
  {.color = {255, 255, 255, 255}, .length = 5, .chars = "Save "},
  {.color = {255, 0, 0, 255}, .length = 6, .chars = "failed"},
  {.color = {255, 255, 255, 255}, .length = 1, .chars = "."}
};
static az_text_line_t save_failed_lines[] = {
  {.total_length = 12, .num_fragments = 3, .fragments = save_failed_fragments}
};
static const az_text_t save_failed_text = {
  .num_lines = 1, .lines = save_failed_lines
};

static az_text_fragment_t save_success_fragments[] = {
  {.color = {255, 255, 255, 255}, .length = 14, .chars = "Game has been "},
  {.color = {0, 255, 0, 255}, .length = 5, .chars = "saved"},
  {.color = {255, 255, 255, 255}, .length = 1, .chars = "."}
};
static az_text_line_t save_success_lines[] = {
  {.total_length = 20, .num_fragments = 3, .fragments = save_success_fragments}
};
static const az_text_t save_success_text = {
  .num_lines = 1, .lines = save_success_lines
};

/*===========================================================================*/

static az_space_state_t state;

static void begin_saved_game(const az_planet_t *planet,
                             const az_saved_games_t *saved_games,
                             int saved_game_index) {
  assert(saved_game_index >= 0);
  assert(saved_game_index < AZ_ARRAY_SIZE(saved_games->games));
  const az_saved_game_t *saved_game = &saved_games->games[saved_game_index];

  memset(&state, 0, sizeof(state));
  state.planet = planet;
  state.save_file_index = saved_game_index;
  state.mode = AZ_MODE_NORMAL;

  if (saved_game->present) {
    // Resume saved game:
    state.ship.player = saved_game->player;
    const az_room_t *room = &planet->rooms[state.ship.player.current_room];
    az_enter_room(&state, room);
    state.ship.position = az_bounds_center(&room->camera_bounds);
    AZ_ARRAY_LOOP(node, state.nodes) {
      if (node->kind == AZ_NODE_SAVE_POINT) {
        state.ship.position = node->position;
        state.ship.angle = node->angle;
        break;
      }
    }
  } else {
    // Begin new game:
    az_init_player(&state.ship.player);
    state.ship.player.current_room = planet->start_room;
    az_enter_room(&state, &planet->rooms[planet->start_room]);
    state.ship.position = planet->start_position;
    state.ship.angle = planet->start_angle;
  }

  az_after_entering_room(&state);
}

static bool save_current_game(az_saved_games_t *saved_games) {
  assert(state.save_file_index >= 0);
  assert(state.save_file_index < AZ_ARRAY_SIZE(saved_games->games));
  az_saved_game_t *saved_game = &saved_games->games[state.save_file_index];
  saved_game->present = true;
  saved_game->player = state.ship.player;
  const char *data_dir = az_get_app_data_directory();
  if (data_dir == NULL) return false;
  char path_buffer[strlen(data_dir) + 10u];
  sprintf(path_buffer, "%s/save.txt", data_dir);
  return az_save_games_to_file(saved_games, path_buffer);
}

static void update_controls(void) {
  state.ship.controls.up_held = az_is_key_held(AZ_KEY_UP_ARROW);
  state.ship.controls.down_held = az_is_key_held(AZ_KEY_DOWN_ARROW);
  state.ship.controls.left_held = az_is_key_held(AZ_KEY_LEFT_ARROW);
  state.ship.controls.right_held = az_is_key_held(AZ_KEY_RIGHT_ARROW);
  state.ship.controls.fire_held = az_is_key_held(AZ_KEY_V);
  state.ship.controls.ordn_held = az_is_key_held(AZ_KEY_C);
  state.ship.controls.util_held = az_is_key_held(AZ_KEY_X);
  state.ship.controls.burn_held = az_is_key_held(AZ_KEY_Z);
}

az_space_action_t az_space_event_loop(const az_planet_t *planet,
                                      az_saved_games_t *saved_games,
                                      int saved_game_index) {
  begin_saved_game(planet, saved_games, saved_game_index);

  while (true) {
    // Tick the state and redraw the screen.
    update_controls();
    az_tick_space_state(&state, 1.0/60.0);
    az_tick_audio_mixer(&state.soundboard);
    az_start_screen_redraw(); {
      az_space_draw_screen(&state);
    } az_finish_screen_redraw();

    // Check the current mode; we may need to do something before we move on to
    // handling events.
    if (state.mode == AZ_MODE_GAME_OVER) {
      // If we're at the end of the game over animation, exit this controller
      // and signal that we should transition to the game over screen
      // controller.
      if (state.mode_data.game_over.step == AZ_GOS_FADE_OUT &&
          state.mode_data.game_over.progress >= 1.0) {
        return AZ_SA_GAME_OVER;
      }
    } else if (state.mode == AZ_MODE_PAUSING) {
      // If we're at the end of the pausing animation, directly engage the
      // paused screen controller, and once it's done, either resume the game
      // or exit to the title screen, as appropriate.
      if (state.mode_data.pause.progress >= 1.0) {
        switch (az_paused_event_loop(planet, &state.ship.player)) {
          case AZ_PA_RESUME:
            state.mode = AZ_MODE_RESUMING;
            state.mode_data.pause.progress = 0.0;
            break;
          case AZ_PA_EXIT_TO_TITLE:
            return AZ_SA_EXIT_TO_TITLE;
        }
      }
    } else if (state.mode == AZ_MODE_CONSOLE &&
               state.mode_data.console.step == AZ_CSS_FINISH) {
      az_node_t *node;
      if (az_lookup_node(&state, state.mode_data.console.node_uid, &node) &&
          node->kind == AZ_NODE_SAVE_POINT) {
        // If we need to save the game, do so.
        const bool ok = save_current_game(saved_games);
        state.message.time_remaining = 4.0;
        if (ok) state.message.text = &save_success_text;
        else state.message.text = &save_failed_text;
      }
    }

    // Handle the event queue.
    az_event_t event;
    while (az_poll_event(&event)) {
      switch (event.kind) {
        case AZ_EVENT_KEY_DOWN:
          // Ignore keystrokes if not in normal mode (except to dismiss
          // upgrade message box).
          if (state.mode == AZ_MODE_DIALOG) {
            if (state.mode_data.dialog.step == AZ_DLS_TALK) {
              state.mode_data.dialog.step = AZ_DLS_PAUSE;
              state.mode_data.dialog.progress = 0.0;
              state.mode_data.dialog.row =
                state.mode_data.dialog.text->num_lines;
              state.mode_data.dialog.col = 0;
            } else if (state.mode_data.dialog.step == AZ_DLS_PAUSE &&
                       event.key.name == AZ_KEY_RETURN) {
              az_resume_script(&state, &state.mode_data.dialog.vm);
            }
            break;
          } else if (state.mode == AZ_MODE_UPGRADE) {
            if (state.mode_data.upgrade.step == AZ_UGS_MESSAGE) {
              state.mode_data.upgrade.step = AZ_UGS_CLOSE;
              state.mode_data.upgrade.progress = 0.0;
            }
            break;
          } else if (state.mode != AZ_MODE_NORMAL) break;
          // Handle the keystroke:
          switch (event.key.name) {
            case AZ_KEY_RETURN:
              state.mode = AZ_MODE_PAUSING;
              state.mode_data.pause.progress = 0.0;
              break;
            case AZ_KEY_UP_ARROW:
              state.ship.controls.up_pressed = true;
              break;
            case AZ_KEY_DOWN_ARROW:
              state.ship.controls.down_pressed = true;
              break;
            case AZ_KEY_V: state.ship.controls.fire_pressed = true; break;
            case AZ_KEY_X: state.ship.controls.util_pressed = true; break;
            case AZ_KEY_1:
              az_select_gun(&state.ship.player, AZ_GUN_CHARGE);
              break;
            case AZ_KEY_2:
              az_select_gun(&state.ship.player, AZ_GUN_FREEZE);
              break;
            case AZ_KEY_3:
              az_select_gun(&state.ship.player, AZ_GUN_TRIPLE);
              break;
            case AZ_KEY_4:
              az_select_gun(&state.ship.player, AZ_GUN_HOMING);
              break;
            case AZ_KEY_5:
              az_select_gun(&state.ship.player, AZ_GUN_PHASE);
              break;
            case AZ_KEY_6:
              az_select_gun(&state.ship.player, AZ_GUN_BURST);
              break;
            case AZ_KEY_7:
              az_select_gun(&state.ship.player, AZ_GUN_PIERCE);
              break;
            case AZ_KEY_8:
              az_select_gun(&state.ship.player, AZ_GUN_BEAM);
              break;
            case AZ_KEY_9:
              az_select_ordnance(&state.ship.player, AZ_ORDN_ROCKETS);
              break;
            case AZ_KEY_0:
              az_select_ordnance(&state.ship.player, AZ_ORDN_BOMBS);
              break;
            default: break;
          }
          break;
        default: break;
      }
    }
  }
}

/*===========================================================================*/
