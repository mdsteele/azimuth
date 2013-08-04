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
#ifndef AZIMUTH_STATE_CAMERA_H_
#define AZIMUTH_STATE_CAMERA_H_

#include "azimuth/util/clock.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

typedef struct {
  double min_r;
  double r_span;
  double min_theta;
  double theta_span;
} az_camera_bounds_t;

// Return the center point of the camera bounds.
az_vector_t az_bounds_center(const az_camera_bounds_t *bounds);

// Clamp a vector to be within the given camera bounds.
az_vector_t az_clamp_to_bounds(const az_camera_bounds_t *bounds,
                               az_vector_t vec);

/*===========================================================================*/

typedef struct {
  az_vector_t center;
  // Camera shake/quake shakes the screen.  A "shake" dies down over a short
  // time, and use used for e.g. rocket impacts.  A "quake" persists until we
  // turn it off (e.g. via a script) or until we change rooms.
  double shake_horz, shake_vert;
  double quake_vert;
  // Camera wobble is used to wobble the screen for NPS portals.
  double wobble_intensity;
  double wobble_goal;
  double wobble_theta;
} az_camera_t;

az_vector_t az_camera_shake_offset(const az_camera_t *camera,
                                   az_clock_t clock);

void az_track_camera_towards(az_camera_t *camera, az_vector_t goal,
                             double time);

// Apply shake to the camera.
void az_shake_camera(az_camera_t *camera, double horz, double vert);

/*===========================================================================*/

#endif // AZIMUTH_STATE_CAMERA_H_
