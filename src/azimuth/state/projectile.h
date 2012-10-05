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
#ifndef AZIMUTH_STATE_PROJECTILE_H_
#define AZIMUTH_STATE_PROJECTILE_H_

#include "azimuth/vector.h"

/*===========================================================================*/

typedef enum {
  AZ_PROJ_NOTHING = 0,
  AZ_PROJ_GUN_NORMAL,
  AZ_PROJ_GUN_CHARGED_NORMAL,
  AZ_PROJ_GUN_ICE,
  AZ_PROJ_GUN_CHARGED_ICE,
  AZ_PROJ_GUN_HOMING,
  AZ_PROJ_GUN_CHARGED_HOMING,
  // TODO add more gun projectiles
  AZ_PROJ_ROCKET,
  AZ_PROJ_HYPER_ROCKET,
  AZ_PROJ_BOMB,
  AZ_PROJ_HYPER_BOMB
} az_proj_kind_t;

typedef struct {
  az_proj_kind_t kind; // if AZ_PROJ_NOTHING, this projectile is not present
  az_vector_t position;
  az_vector_t velocity;
  double age;
} az_projectile_t;

/*===========================================================================*/

#endif // AZIMUTH_STATE_PROJECTILE_H_
