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

#include "azimuth/tick/baddie_oth_sgs.h"

#include <assert.h>

#include "azimuth/state/baddie.h"
#include "azimuth/state/baddie_oth.h"
#include "azimuth/state/space.h"
#include "azimuth/tick/baddie_oth.h"
#include "azimuth/tick/baddie_util.h"

/*===========================================================================*/

void az_tick_bad_oth_supergunship(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_OTH_SUPERGUNSHIP);
  const double old_angle = baddie->angle;
  az_fly_towards_ship(state, baddie, time,
                      5.0, 300.0, 300.0, 200.0, 0.0, 100.0);
  // TODO: implement Oth Supergunship
  az_tick_oth_tendrils(state, baddie, &AZ_OTH_SUPERGUNSHIP_TENDRILS, old_angle,
                       time);
}

/*===========================================================================*/
