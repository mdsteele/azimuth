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
#ifndef AZIMUTH_VIEW_PAUSED_H_
#define AZIMUTH_VIEW_PAUSED_H_

#include <stdbool.h>

#include "azimuth/state/planet.h"
#include "azimuth/state/ship.h"
#include "azimuth/state/upgrade.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/prefs.h"

/*===========================================================================*/

typedef struct {
  const az_planet_t *planet;
  const az_preferences_t *prefs;
  az_ship_t *ship;
  az_clock_t clock;
  bool show_upgrades_drawer;
  double drawer_openness; // 0.0 to 1.0
  bool hovering_over_upgrade;
  az_upgrade_t hovered_upgrade;
} az_paused_state_t;

void az_paused_draw_screen(const az_paused_state_t *state);

void az_tick_paused_state(az_paused_state_t *state, double time);

void az_paused_on_hover(az_paused_state_t *state, int x, int y);

/*===========================================================================*/

#endif // AZIMUTH_VIEW_PAUSED_H_
