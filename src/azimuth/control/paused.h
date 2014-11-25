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
#ifndef AZIMUTH_CONTROL_PAUSED_H_
#define AZIMUTH_CONTROL_PAUSED_H_

#include "azimuth/state/planet.h"
#include "azimuth/state/ship.h"
#include "azimuth/util/prefs.h"

/*===========================================================================*/

typedef enum {
  AZ_PA_RESUME,
  AZ_PA_EXIT_TO_TITLE
} az_paused_action_t;

az_paused_action_t az_paused_event_loop(
    const az_planet_t *planet, az_preferences_t *prefs, az_ship_t *ship);

/*===========================================================================*/

#endif // AZIMUTH_CONTROL_PAUSED_H_
