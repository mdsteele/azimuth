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
#ifndef AZIMUTH_VIEW_MINIMAP_H_
#define AZIMUTH_VIEW_MINIMAP_H_

#include <stdbool.h>

#include "azimuth/state/planet.h"
#include "azimuth/state/room.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

void az_draw_minimap_room(const az_planet_t *planet, const az_room_t *room,
                          bool visited, bool blink, az_vector_t camera_center);

void az_draw_map_marker(az_vector_t center, az_clock_t clock);

void az_draw_save_room_label(az_vector_t center);
void az_draw_comm_room_label(az_vector_t center);
void az_draw_refill_room_label(az_vector_t center);

/*===========================================================================*/

#endif // AZIMUTH_VIEW_MINIMAP_H_
