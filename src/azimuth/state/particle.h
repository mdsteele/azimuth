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
#ifndef AZIMUTH_STATE_PARTICLE_H_
#define AZIMUTH_STATE_PARTICLE_H_

#include "azimuth/util/color.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

typedef enum {
  AZ_PAR_NOTHING = 0,
  // BOOM: An explosion.  param1=radius
  AZ_PAR_BOOM,
  // BEAM: A beam.  param1=length, param2=width/2
  AZ_PAR_BEAM,
  // EMBER: A shrinking glowball.  param1=radius
  AZ_PAR_EMBER,
  // NPS_PORTAL: A Nonlinear Paradimensional Shear portal.  param1=radius
  AZ_PAR_NPS_PORTAL,
  // SHARD: A trianglar shard of metal/rock.
  AZ_PAR_SHARD,
  // SPLOOSH: A splash made from entering/exiting water.  param1=amplitude
  AZ_PAR_SPLOOSH
} az_particle_kind_t;

typedef struct {
  az_particle_kind_t kind; // if AZ_PAR_NOTHING, this particle is not present
  az_color_t color;
  az_vector_t position;
  az_vector_t velocity;
  double angle;
  double age, lifetime; // seconds
  double param1, param2; // meaning depends on kind
} az_particle_t;

/*===========================================================================*/

#endif // AZIMUTH_STATE_PARTICLE_H_
