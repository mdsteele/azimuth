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
#ifndef AZIMUTH_STATE_GRAVFIELD_H_
#define AZIMUTH_STATE_GRAVFIELD_H_

#include <stdbool.h>

#include "azimuth/state/uid.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

// The number of different gravfield kinds there are, not counting
// AZ_GRAV_NOTHING:
#define AZ_NUM_GRAVFIELD_KINDS 4

typedef enum {
  AZ_GRAV_NOTHING = 0,
  AZ_GRAV_TRAPEZOID,
  AZ_GRAV_SECTOR_PULL,
  AZ_GRAV_SECTOR_SPIN,
  AZ_GRAV_WATER
} az_gravfield_kind_t;

typedef union {
  struct {
    double front_offset;
    double front_semiwidth;
    double rear_semiwidth;
    double semilength;
  } trapezoid;
  struct {
    double sweep_degrees; // interior angle; 0 is equivalent to 360
    double inner_radius;
    double thickness;
  } sector;
} az_gravfield_size_t;

typedef struct {
  az_gravfield_kind_t kind; // if AZ_GRAV_NOTHING, this gravfield isn't present
  az_uid_t uid;
  az_vector_t position;
  double angle;
  double strength; // negative to reverse direction
  az_gravfield_size_t size;
} az_gravfield_t;

// Return true if the given gravfield kind is trapezoidal (i.e. it uses the
// trapezoid field of az_gravfield_size_t), or false if it is sectoral (i.e. it
// uses the sector field of az_gravfield_size_t).
bool az_trapezoidal(az_gravfield_kind_t kind);

double az_sector_interior_angle(const az_gravfield_size_t *size);

bool az_point_within_gravfield(const az_gravfield_t *gravfield,
                               az_vector_t point);

bool az_ray_hits_water_surface(
    const az_gravfield_t *gravfield, az_vector_t start, az_vector_t delta,
    az_vector_t *point_out, double *angle_out);

/*===========================================================================*/

#endif // AZIMUTH_STATE_GRAVFIELD_H_
