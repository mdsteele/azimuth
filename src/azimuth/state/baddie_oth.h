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
#ifndef AZIMUTH_STATE_BADDIE_OTH_H_
#define AZIMUTH_STATE_BADDIE_OTH_H_

#include "azimuth/util/vector.h"

/*===========================================================================*/

typedef struct {
  double tracking_base; // smaller = faster tracking
  double length;
  double semithick;
  double drift;
  int num_tendrils;
  const az_vector_t *tendril_bases;
} az_oth_tendrils_data_t;

extern const az_oth_tendrils_data_t AZ_OTH_BRAWLER_TENDRILS;
extern const az_oth_tendrils_data_t AZ_OTH_CRAB_TENDRILS;
extern const az_oth_tendrils_data_t AZ_OTH_CRAWLER_TENDRILS;
extern const az_oth_tendrils_data_t AZ_OTH_GUNSHIP_TENDRILS;
extern const az_oth_tendrils_data_t AZ_OTH_ORB_TENDRILS;
extern const az_oth_tendrils_data_t AZ_OTH_RAZOR_TENDRILS;
extern const az_oth_tendrils_data_t AZ_OTH_SNAPDRAGON_TENDRILS;

/*===========================================================================*/

#endif // AZIMUTH_STATE_BADDIE_OTH_H_
