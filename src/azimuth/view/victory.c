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

#include "azimuth/view/victory.h"

#include <GL/gl.h>

#include "azimuth/constants.h"
#include "azimuth/state/save.h"
#include "azimuth/util/audio.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/misc.h"
#include "azimuth/view/string.h"

/*===========================================================================*/

void az_victory_draw_screen(const az_victory_state_t *state) {
  glColor3f(1, 1, 1);
  const int total_seconds = (int)state->clear_time;
  const int hours = total_seconds / 3600;
  const int minutes = (total_seconds % 3600) / 60;
  const int seconds = total_seconds % 60;
  az_draw_printf(16, AZ_ALIGN_CENTER, AZ_SCREEN_WIDTH/2, 100,
                 "Clear time: %d:%02d:%02d", hours, minutes, seconds);
  AZ_STATIC_ASSERT(AZ_NUM_UPGRADES == 100);
  const int percent = state->num_upgrades;
  az_draw_printf(16, AZ_ALIGN_CENTER, AZ_SCREEN_WIDTH/2, 200,
                 "Items collected: %d%%", percent);
}

void az_tick_victory_state(az_victory_state_t *state, double time) {
  ++state->clock;
  state->step = AZ_VS_DONE;
}

/*===========================================================================*/
