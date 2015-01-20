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
#ifndef AZIMUTH_VIEW_BACKGROUND_H_
#define AZIMUTH_VIEW_BACKGROUND_H_

#include "azimuth/state/background.h"
#include "azimuth/state/camera.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

void az_draw_background_pattern(
    az_background_pattern_t pattern, const az_camera_bounds_t *camera_bounds,
    az_vector_t camera_center, az_clock_t clock);

/*===========================================================================*/

#endif // AZIMUTH_VIEW_BACKGROUND_H_
