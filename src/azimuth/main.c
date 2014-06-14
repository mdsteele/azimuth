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

#include <stdbool.h>
#include <stdlib.h>

#include <SDL/SDL.h> // for main() renaming

#include "azimuth/control/gameover.h"
#include "azimuth/control/space.h"
#include "azimuth/control/title.h"
#include "azimuth/control/util.h"
#include "azimuth/gui/audio.h"
#include "azimuth/gui/screen.h"
#include "azimuth/state/baddie.h" // for az_init_baddie_datas
#include "azimuth/state/music.h" // for az_init_music_datas
#include "azimuth/state/planet.h"
#include "azimuth/state/save.h"
#include "azimuth/state/sound.h" // for az_init_sound_datas
#include "azimuth/state/wall.h" // for az_init_wall_datas
#include "azimuth/system/resource.h"
#include "azimuth/util/misc.h" // for AZ_ASSERT_UNREACHABLE
#include "azimuth/util/prefs.h"
#include "azimuth/view/dialog.h" // for az_init_portrait_drawing
#include "azimuth/view/wall.h" // for az_init_wall_drawing

/*===========================================================================*/

static az_planet_t planet;
static az_saved_games_t saved_games;
static az_preferences_t preferences;

static bool load_scenario(void) {
  const char *resource_dir = az_get_resource_directory();
  if (resource_dir == NULL) return false;
  if (!az_init_music_datas(resource_dir)) return false;
  if (!az_load_planet(resource_dir, &planet)) return false;
  return true;
}

typedef enum {
  AZ_CONTROLLER_TITLE,
  AZ_CONTROLLER_SPACE,
  AZ_CONTROLLER_GAME_OVER
} az_controller_t;

int main(int argc, char **argv) {
  az_init_sound_datas();
  az_init_baddie_datas();
  az_init_wall_datas();
  az_register_gl_init_func(az_init_portrait_drawing);
  az_register_gl_init_func(az_init_wall_drawing);

  if (!load_scenario()) {
    printf("Failed to load scenario.\n");
    return EXIT_FAILURE;
  }
  az_load_preferences(&preferences);
  az_load_saved_games(&planet, &saved_games);
  az_init_gui(preferences.fullscreen_on_startup, true);
  az_set_global_music_volume(preferences.music_volume);
  az_set_global_sound_volume(preferences.sound_volume);

  az_controller_t controller = AZ_CONTROLLER_TITLE;
  int saved_game_slot_index = 0;
  while (true) {
    switch (controller) {
      case AZ_CONTROLLER_TITLE:
        {
          const az_title_action_t action =
            az_title_event_loop(&planet, &saved_games, &preferences);
          switch (action.kind) {
            case AZ_TA_QUIT:
              return EXIT_SUCCESS;
            case AZ_TA_START_GAME:
              controller = AZ_CONTROLLER_SPACE;
              saved_game_slot_index = action.slot_index;
              break;
          }
        }
        break;
      case AZ_CONTROLLER_SPACE:
        switch (az_space_event_loop(&planet, &saved_games, &preferences,
                                    saved_game_slot_index)) {
          case AZ_SA_EXIT_TO_TITLE:
            controller = AZ_CONTROLLER_TITLE;
            break;
          case AZ_SA_GAME_OVER:
            controller = AZ_CONTROLLER_GAME_OVER;
            break;
        }
        break;
      case AZ_CONTROLLER_GAME_OVER:
        switch (az_gameover_event_loop()) {
          case AZ_GOA_TRY_AGAIN:
            controller = AZ_CONTROLLER_SPACE;
            break;
          case AZ_GOA_RETURN_TO_TITLE:
            controller = AZ_CONTROLLER_TITLE;
            break;
          case AZ_GOA_QUIT:
            return EXIT_SUCCESS;
        }
        break;
    }
  }
  AZ_ASSERT_UNREACHABLE();
}

/*===========================================================================*/
