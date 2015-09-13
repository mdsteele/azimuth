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
#ifndef AZIMUTH_CONTROL_TITLE_H_
#define AZIMUTH_CONTROL_TITLE_H_

#include <stdbool.h>

#include "azimuth/state/planet.h"
#include "azimuth/state/save.h"
#include "azimuth/util/prefs.h"

/*===========================================================================*/

typedef struct {
  enum { AZ_TA_START_GAME, AZ_TA_QUIT } kind;
  int slot_index;
} az_title_action_t;

az_title_action_t az_title_event_loop(
    const az_planet_t *planet, az_saved_games_t *saved_games,
    az_preferences_t *prefs, bool skip_intro);

/*===========================================================================*/

#endif // AZIMUTH_CONTROL_TITLE_H_
