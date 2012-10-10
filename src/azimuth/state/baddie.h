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
#ifndef AZIMUTH_STATE_BADDIE_H_
#define AZIMUTH_STATE_BADDIE_H_

#include "azimuth/util/vector.h"

/*===========================================================================*/

typedef enum {
  AZ_BAD_NOTHING = 0,
  AZ_BAD_LUMP,
  AZ_BAD_TURRET
} az_baddie_kind_t;

typedef struct {
  double bounding_radius;
  double max_health;
} az_baddie_data_t;

typedef struct {
  az_baddie_kind_t kind; // if AZ_BAD_NOTHING, this baddie is not present
  const az_baddie_data_t *data;
  az_vector_t position;
  az_vector_t velocity;
  double angle;
  double health;
  double cooldown; // time until baddie can attack again, in seconds
  // The baddie's "components" describe the positions of its subparts.  The
  // meanings of these are specific to the baddie kind.
  struct {
    az_vector_t position;
    double angle;
  } components[4];
} az_baddie_t;

// Set reasonable initial field values for a baddie of the given kind, at the
// given position.
void az_init_baddie(az_baddie_t *baddie, az_baddie_kind_t kind,
                    az_vector_t position, double angle);

/*===========================================================================*/

#endif // AZIMUTH_STATE_BADDIE_H_
