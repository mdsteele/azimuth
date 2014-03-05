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
#include "azimuth/control/victory.h"
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

static const char save_failed_paragraph[] = "Save $Rfailed$W.";
static const char save_success_paragraph[] = "Game has been $Gsaved$W.";

static az_space_state_t state;

static void begin_saved_game(
    const az_planet_t *planet, const az_saved_games_t *saved_games,
    const az_preferences_t *prefs, int saved_game_index) {
  assert(saved_game_index >= 0);
  assert(saved_game_index < AZ_ARRAY_SIZE(saved_games->games));
  const az_saved_game_t *saved_game = &saved_games->games[saved_game_index];

  memset(&state, 0, sizeof(state));
  state.planet = planet;
  state.prefs = prefs;
  state.save_file_index = saved_game_index;
  state.mode = AZ_MODE_NORMAL;

  if (saved_game->present) {
    // Resume saved game:
    state.ship.player = saved_game->player;
    const az_room_t *room = &planet->rooms[state.ship.player.current_room];
    az_enter_room(&state, room);
    state.ship.position = az_bounds_center(&room->camera_bounds);
    AZ_ARRAY_LOOP(node, state.nodes) {
      if (node->kind == AZ_NODE_CONSOLE &&
          node->subkind.console == AZ_CONS_SAVE) {
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

static void update_controls(const az_preferences_t *prefs) {
  state.ship.controls.up_held =
    az_is_key_held(prefs->keys[AZ_PREFS_UP_KEY_INDEX]);
  state.ship.controls.down_held =
    az_is_key_held(prefs->keys[AZ_PREFS_DOWN_KEY_INDEX]);
  state.ship.controls.right_held =
    az_is_key_held(prefs->keys[AZ_PREFS_RIGHT_KEY_INDEX]);
  state.ship.controls.left_held =
    az_is_key_held(prefs->keys[AZ_PREFS_LEFT_KEY_INDEX]);
  state.ship.controls.fire_held =
    az_is_key_held(prefs->keys[AZ_PREFS_FIRE_KEY_INDEX]);
  state.ship.controls.ordn_held =
    az_is_key_held(prefs->keys[AZ_PREFS_ORDN_KEY_INDEX]);
  state.ship.controls.util_held =
    az_is_key_held(prefs->keys[AZ_PREFS_UTIL_KEY_INDEX]);
}

az_space_action_t az_space_event_loop(
    const az_planet_t *planet, az_saved_games_t *saved_games,
    const az_preferences_t *prefs, int saved_game_index) {
  begin_saved_game(planet, saved_games, prefs, saved_game_index);

  while (true) {
    // Tick the state and redraw the screen.
    update_controls(prefs);
    az_tick_space_state(&state, 1.0/60.0);
    az_tick_audio_mixer(&state.soundboard);
    az_start_screen_redraw(); {
      az_space_draw_screen(&state);
    } az_finish_screen_redraw();
    memset(&state.ship.controls, 0, sizeof(state.ship.controls));

    // Check the current mode; we may need to do something before we move on to
    // handling events.
    if (state.victory) {
      az_victory_event_loop(&state.ship.player);
      return AZ_SA_EXIT_TO_TITLE;
    } else if (state.mode == AZ_MODE_GAME_OVER) {
      // If we're at the end of the game over animation, exit this controller
      // and signal that we should transition to the game over screen
      // controller.
      if (state.game_over_mode.step == AZ_GOS_FADE_OUT &&
          state.game_over_mode.progress >= 1.0) {
        return AZ_SA_GAME_OVER;
      }
    } else if (state.mode == AZ_MODE_PAUSING) {
      // If we're at the end of the pausing fade-out, directly engage the
      // paused screen controller, and once it's done, either resume the game
      // or exit to the title screen, as appropriate.
      if (state.pausing_mode.step == AZ_PSS_FADE_OUT &&
          state.pausing_mode.fade_alpha == 1.0) {
        switch (az_paused_event_loop(planet, prefs, &state.ship)) {
          case AZ_PA_RESUME:
            state.pausing_mode.step = AZ_PSS_FADE_IN;
            break;
          case AZ_PA_EXIT_TO_TITLE:
            return AZ_SA_EXIT_TO_TITLE;
        }
      }
    } else if (state.mode == AZ_MODE_CONSOLE &&
               state.console_mode.step == AZ_CSS_SAVE) {
      // If we need to save the game, do so.
      const bool ok = save_current_game(saved_games);
      if (ok) az_set_message(&state, save_success_paragraph);
      else az_set_message(&state, save_failed_paragraph);
    }

    // Handle the event queue.
    az_event_t event;
    while (az_poll_event(&event)) {
      switch (event.kind) {
        case AZ_EVENT_KEY_DOWN:
          if (state.mode == AZ_MODE_DIALOG) {
            if (state.dialog_mode.step == AZ_DLS_TALK) {
              state.dialog_mode.step = AZ_DLS_WAIT;
              state.dialog_mode.progress = 0.0;
              state.dialog_mode.chars_to_print =
                state.dialog_mode.paragraph_length;
            } else if (state.dialog_mode.step == AZ_DLS_WAIT &&
                       event.key.id == AZ_KEY_RETURN) {
              assert(state.sync_vm.script != NULL);
              az_resume_script(&state, &state.sync_vm);
            }
            break;
          } else if (state.mode == AZ_MODE_UPGRADE) {
            if (state.upgrade_mode.step == AZ_UGS_MESSAGE) {
              state.upgrade_mode.step = AZ_UGS_CLOSE;
              state.upgrade_mode.progress = 0.0;
            }
            break;
          } else if (state.mode == AZ_MODE_GAME_OVER) break;
          // Handle the keystroke:
          switch (event.key.id) {
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
            default:
              if (event.key.id == prefs->keys[AZ_PREFS_PAUSE_KEY_INDEX]) {
                if (state.mode == AZ_MODE_NORMAL &&
                    state.cutscene.scene == AZ_SCENE_NOTHING) {
                  state.mode = AZ_MODE_PAUSING;
                  state.pausing_mode = (az_pausing_mode_data_t){
                    .step = AZ_PSS_FADE_OUT, .fade_alpha = 0.0
                  };
                }
              } else if (event.key.id == prefs->keys[AZ_PREFS_UP_KEY_INDEX]) {
                state.ship.controls.up_pressed = true;
              } else if (event.key.id ==
                         prefs->keys[AZ_PREFS_DOWN_KEY_INDEX]) {
                state.ship.controls.down_pressed = true;
              } else if (event.key.id ==
                         prefs->keys[AZ_PREFS_FIRE_KEY_INDEX]) {
                state.ship.controls.fire_pressed = true;
              } else if (event.key.id ==
                         prefs->keys[AZ_PREFS_UTIL_KEY_INDEX]) {
                state.ship.controls.util_pressed = true;
              }
              break;
          }
          break;
        default: break;
      }
    }
  }
}

/*===========================================================================*/
