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
#ifndef AZIMUTH_CONTROL_UTIL_H_
#define AZIMUTH_CONTROL_UTIL_H_

#include <stdbool.h>

#include "azimuth/state/planet.h"
#include "azimuth/state/save.h"
#include "azimuth/util/prefs.h"
#include "azimuth/view/prefs.h"

/*===========================================================================*/

void az_load_preferences(az_preferences_t *prefs);
bool az_save_preferences(const az_preferences_t *prefs);

void az_update_prefefences(const az_prefs_pane_t *pane,
                           az_preferences_t *prefs, bool *prefs_changed);

void az_load_saved_games(const az_planet_t *planet,
                         az_saved_games_t *saved_games);
bool az_save_saved_games(const az_saved_games_t *saved_games);

/*===========================================================================*/

#endif // AZIMUTH_CONTROL_UTIL_H_
