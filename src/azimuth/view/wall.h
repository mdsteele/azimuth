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
#ifndef AZIMUTH_VIEW_WALL_H_
#define AZIMUTH_VIEW_WALL_H_

#include "azimuth/state/space.h"
#include "azimuth/state/wall.h"
#include "azimuth/util/clock.h"

/*===========================================================================*/

// Call this at program startup to initialize drawing of walls.  This must be
// called _after_ both az_init_gui and az_init_wall_datas, and must be called
// _before_ any calls to az_draw_wall or az_draw_walls.
void az_init_wall_drawing(void);

// Draw a single wall, without flare or additional transforms.  The GL matrix
// should be at the wall position.
void az_draw_wall_data(const az_wall_data_t *data, az_clock_t clock);

// Draw a single wall.  The GL matrix should be at the camera position.
void az_draw_wall(const az_wall_t *wall, az_clock_t clock);

// Draw all walls.  The GL matrix should be at the camera position.
void az_draw_walls(const az_space_state_t *state);

/*===========================================================================*/

#endif // AZIMUTH_VIEW_WALL_H_
