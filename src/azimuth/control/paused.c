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

#include "azimuth/control/paused.h"

#include "azimuth/gui/event.h"
#include "azimuth/gui/screen.h"
#include "azimuth/state/planet.h"
#include "azimuth/state/player.h"
#include "azimuth/state/ship.h"
#include "azimuth/view/paused.h"

/*===========================================================================*/

az_paused_action_t az_paused_event_loop(
    const az_planet_t *planet, const az_preferences_t *prefs,
    az_ship_t *ship) {
  static az_paused_state_t state;
  az_init_paused_state(&state, planet, prefs, ship);
  az_player_t *player = &ship->player;

  while (true) {
    // Tick the state and redraw the screen.
    az_tick_paused_state(&state, 1.0/60.0);
    az_start_screen_redraw(); {
      az_paused_draw_screen(&state);
    } az_finish_screen_redraw();

    // Get and process GUI events.
    az_event_t event;
    while (az_poll_event(&event)) {
      switch (event.kind) {
        case AZ_EVENT_KEY_DOWN:
          switch (event.key.id) {
            case AZ_KEY_RETURN: return AZ_PA_RESUME;
            case AZ_KEY_1: az_select_gun(player, AZ_GUN_CHARGE); break;
            case AZ_KEY_2: az_select_gun(player, AZ_GUN_FREEZE); break;
            case AZ_KEY_3: az_select_gun(player, AZ_GUN_TRIPLE); break;
            case AZ_KEY_4: az_select_gun(player, AZ_GUN_HOMING); break;
            case AZ_KEY_5: az_select_gun(player, AZ_GUN_PHASE);  break;
            case AZ_KEY_6: az_select_gun(player, AZ_GUN_BURST);  break;
            case AZ_KEY_7: az_select_gun(player, AZ_GUN_PIERCE); break;
            case AZ_KEY_8: az_select_gun(player, AZ_GUN_BEAM);   break;
            case AZ_KEY_9: az_select_ordnance(player, AZ_ORDN_ROCKETS); break;
            case AZ_KEY_0: az_select_ordnance(player, AZ_ORDN_BOMBS);   break;
            default:
              if (event.key.id ==
                  state.prefs->keys[AZ_PREFS_PAUSE_KEY_INDEX]) {
                return AZ_PA_RESUME;
              } else if (event.key.id ==
                         state.prefs->keys[AZ_PREFS_FIRE_KEY_INDEX]) {
                state.show_upgrades_drawer = true;
              } else if (event.key.id ==
                         state.prefs->keys[AZ_PREFS_ORDN_KEY_INDEX]) {
                state.show_upgrades_drawer = false;
              }
              break;
          }
          break;
        case AZ_EVENT_MOUSE_DOWN:
          az_paused_on_click(&state, event.mouse.x, event.mouse.y);
          break;
        case AZ_EVENT_MOUSE_MOVE:
          az_paused_on_hover(&state, event.mouse.x, event.mouse.y);
          break;
        default: break;
      }
    }
  }
}

/*===========================================================================*/
