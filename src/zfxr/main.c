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

#include <stdlib.h> // for EXIT_SUCCESS

#include "SDL.h" // for main() renaming

#include "azimuth/constants.h"
#include "azimuth/gui/audio.h"
#include "azimuth/gui/event.h"
#include "azimuth/gui/screen.h"
#include "zfxr/state.h"
#include "zfxr/view.h"

/*===========================================================================*/

static az_zfxr_state_t state;

static void event_loop(void) {
  while (true) {
    az_tick_zfxr_state(&state, AZ_FRAME_TIME_SECONDS);
    az_tick_audio(&state.soundboard);
    az_start_screen_redraw(); {
      az_zfxr_draw_screen(&state);
    } az_finish_screen_redraw();

    az_event_t event;
    while (az_poll_event(&event)) {
      switch (event.kind) {
        case AZ_EVENT_KEY_DOWN:
          az_zfxr_on_keypress(&state, event.key.id, event.key.shift);
          break;
        case AZ_EVENT_MOUSE_DOWN:
          az_zfxr_on_click(&state, event.mouse.x, event.mouse.y);
          break;
        default: break;
      }
    }
  }
}

int main(int argc, char **argv) {
  az_init_zfxr_state(&state);
  az_init_gui(false, true);

  event_loop();

  az_deinit_gui();
  return EXIT_SUCCESS;
}

/*===========================================================================*/
