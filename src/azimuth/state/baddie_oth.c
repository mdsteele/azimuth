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

#include "azimuth/state/baddie_oth.h"

#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

#define DECL_TENDRIL_BASES(b) \
  .num_tendrils=AZ_ARRAY_SIZE(b), .tendril_bases=(b)

/*===========================================================================*/

static const az_vector_t brawler_tendril_bases[] = {
  {-8, -5}, {-10, 0}, {-8, 5}
};
const az_oth_tendrils_data_t AZ_OTH_BRAWLER_TENDRILS = {
  .tracking_base = 0.002,
  .length = 50.0,
  .semithick = 5.0,
  .drift = 10.0,
  DECL_TENDRIL_BASES(brawler_tendril_bases)
};

/*===========================================================================*/

static const az_vector_t crab_tendril_bases[] = {
  {-10, -5}, {-10, 0}, {-10, 5}
};
const az_oth_tendrils_data_t AZ_OTH_CRAB_TENDRILS = {
  .tracking_base = 0.05,
  .length = 60.0,
  .semithick = 4.0,
  .drift = 10.0,
  DECL_TENDRIL_BASES(crab_tendril_bases)
};

/*===========================================================================*/

static const az_vector_t crawler_tendril_bases[] = {
  {4, -5}, {10, 0}, {4, 5}
};
const az_oth_tendrils_data_t AZ_OTH_CRAWLER_TENDRILS = {
  .tracking_base = 0.1,
  .length = 35.0,
  .semithick = 4.0,
  .drift = 10.0,
  DECL_TENDRIL_BASES(crawler_tendril_bases)
};

/*===========================================================================*/

static const az_vector_t gunship_tendril_bases[] = {
  {-8, -5}, {-10, 0}, {-8, 5}
};
const az_oth_tendrils_data_t AZ_OTH_GUNSHIP_TENDRILS = {
  .tracking_base = 0.002,
  .length = 50.0,
  .semithick = 4.0,
  .drift = 10.0,
  DECL_TENDRIL_BASES(gunship_tendril_bases)
};

/*===========================================================================*/

static const az_vector_t minicrab_tendril_bases[] = {
  {-10, -5}, {-10, 0}, {-10, 5}
};
const az_oth_tendrils_data_t AZ_OTH_MINICRAB_TENDRILS = {
  .tracking_base = 0.05,
  .length = 40.0,
  .semithick = 3.0,
  .drift = 10.0,
  DECL_TENDRIL_BASES(minicrab_tendril_bases)
};

/*===========================================================================*/

static const az_vector_t orb_tendril_bases[] = {
  {-10, 0}, {5, -8.660254037844386}, {5, 8.660254037844386}
};
const az_oth_tendrils_data_t AZ_OTH_ORB_TENDRILS = {
  .tracking_base = 0.1,
  .length = 40.0,
  .semithick = 6.0,
  .drift = 10.0,
  DECL_TENDRIL_BASES(orb_tendril_bases)
};

/*===========================================================================*/

static const az_vector_t razor_tendril_bases[] = {
  {-5, 0}
};
const az_oth_tendrils_data_t AZ_OTH_RAZOR_TENDRILS = {
  .tracking_base = 0.00001,
  .length = 30.0,
  .semithick = 3.0,
  .drift = 3.0,
  DECL_TENDRIL_BASES(razor_tendril_bases)
};

/*===========================================================================*/

static const az_vector_t snapdragon_tendril_bases[] = {
  {-20, -15}, {-25, -5}, {-25, 5}, {-20, 15}
};
const az_oth_tendrils_data_t AZ_OTH_SNAPDRAGON_TENDRILS = {
  .tracking_base = 0.005,
  .length = 70.0,
  .semithick = 5.0,
  .drift = 10.0,
  DECL_TENDRIL_BASES(snapdragon_tendril_bases)
};

/*===========================================================================*/

static const az_vector_t supergunship_tendril_bases[] = {
  {-6, -7}, {-8, -5}, {-10, 0}, {-8, 5}, {-6, 7}
};
const az_oth_tendrils_data_t AZ_OTH_SUPERGUNSHIP_TENDRILS = {
  .tracking_base = 0.002,
  .length = 50.0,
  .semithick = 4.0,
  .drift = 10.0,
  DECL_TENDRIL_BASES(supergunship_tendril_bases)
};

/*===========================================================================*/
