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

#pragma once
#ifndef AZIMUTH_CONTROL_SPACE_H_
#define AZIMUTH_CONTROL_SPACE_H_

#include "azimuth/state/planet.h"
#include "azimuth/state/save.h"
#include "azimuth/util/prefs.h"

/*===========================================================================*/

typedef enum {
  AZ_SA_EXIT_TO_TITLE,
  AZ_SA_GAME_OVER
} az_space_action_t;

az_space_action_t az_space_event_loop(
    const az_planet_t *planet, az_saved_games_t *saved_games,
    const az_preferences_t *prefs, int saved_game_index);

/*===========================================================================*/

#endif // AZIMUTH_CONTROL_SPACE_H_
