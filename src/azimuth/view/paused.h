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

#include "azimuth/state/planet.h"
#include "azimuth/state/player.h"
#include "azimuth/util/clock.h"

/*===========================================================================*/

typedef struct {
  const az_planet_t *planet;
  az_player_t *player;
  az_clock_t clock;
} az_paused_state_t;

void az_paused_draw_screen(az_paused_state_t *state);

void az_tick_paused_state(az_paused_state_t *state, double time);

/*===========================================================================*/

#endif // AZIMUTH_VIEW_PAUSED_H_
