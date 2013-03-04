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

#include "azimuth/control/gameover.h"

#include <string.h>

#include "azimuth/gui/audio.h"
#include "azimuth/gui/event.h"
#include "azimuth/gui/screen.h"
#include "azimuth/state/planet.h"
#include "azimuth/state/save.h"
#include "azimuth/util/misc.h"
#include "azimuth/view/gameover.h"

/*===========================================================================*/

az_gameover_action_t az_gameover_event_loop(void) {
  az_init_gameover_view();
  static az_gameover_state_t state;
  memset(&state, 0, sizeof(state));
  az_change_music(&state.soundboard, AZ_MUS_TITLE);

  while (true) {
    // Tick the state and redraw the screen.
    az_tick_gameover_state(&state, 1.0/60.0);
    az_tick_audio_mixer(&state.soundboard);
    az_start_screen_redraw(); {
      az_gameover_draw_screen(&state);
    } az_finish_screen_redraw();

    // Check if we need to return with an action.
    if (state.mode == AZ_GMODE_QUITTING) {
      return AZ_GOA_QUIT;
    }
    if (state.mode == AZ_GMODE_RETRYING && state.mode_progress >= 1.0) {
      return AZ_GOA_TRY_AGAIN;
    }
    if (state.mode == AZ_GMODE_RETURNING && state.mode_progress >= 1.0) {
      return AZ_GOA_RETURN_TO_TITLE;
    }

    // Get and process GUI events.
    az_event_t event;
    while (az_poll_event(&event)) {
      switch (event.kind) {
        case AZ_EVENT_MOUSE_DOWN:
          az_gameover_on_click(&state, event.mouse.x, event.mouse.y);
          break;
        case AZ_EVENT_MOUSE_MOVE:
          az_gameover_on_hover(&state, event.mouse.x, event.mouse.y);
          break;
        default: break;
      }
    }
  }
  AZ_ASSERT_UNREACHABLE();
}

/*===========================================================================*/
