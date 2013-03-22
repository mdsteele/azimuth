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
#ifndef AZIMUTH_VIEW_GRAVFIELD_H_
#define AZIMUTH_VIEW_GRAVFIELD_H_

#include "azimuth/state/gravfield.h"
#include "azimuth/state/space.h"

/*===========================================================================*/

// Draw a single gravfield.  The GL matrix should be at the camera position.
// The total_time argument should be state.ship.player.total_time, or some
// equivalent.
void az_draw_gravfield(const az_gravfield_t *gravfield, double total_time);

// Draw all non-water gravfields.  The GL matrix should be at the camera
// position.
void az_draw_gravfields(const az_space_state_t *state);

// Draw all water gravfields.  The GL matrix should be at the camera position.
void az_draw_water(const az_space_state_t *state);

/*===========================================================================*/

#endif // AZIMUTH_VIEW_GRAVFIELD_H_
