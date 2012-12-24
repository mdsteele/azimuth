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

#include <string.h> // for memset

#include "azimuth/gui/event.h"
#include "azimuth/gui/screen.h"
#include "azimuth/state/planet.h"
#include "azimuth/state/player.h"
#include "azimuth/view/paused.h"

/*===========================================================================*/

az_paused_action_t az_paused_event_loop(const az_planet_t *planet,
                                        az_player_t *player) {
  static az_paused_state_t state;
  memset(&state, 0, sizeof(state));
  state.planet = planet;
  state.player = player;

  while (true) {
    // Tick the state and redraw the screen.
    //az_tick_paused_state(&state, 1.0/60.0); // TODO
    az_start_screen_redraw(); {
      az_paused_draw_screen(&state);
    } az_finish_screen_redraw();

    // Get and process GUI events.
    az_event_t event;
    while (az_poll_event(&event)) {
      switch (event.kind) {
        case AZ_EVENT_KEY_DOWN:
          return AZ_PA_RESUME; // TODO not for just any key
        default: break;
      }
    }
  }
}

/*===========================================================================*/
