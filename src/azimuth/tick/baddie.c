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

#include "azimuth/tick/baddie.h"

#include <assert.h>
#include <stdbool.h>

#include "azimuth/state/baddie.h"
#include "azimuth/state/projectile.h"

/*===========================================================================*/

bool az_proj_hits_baddie(az_projectile_t *proj, az_baddie_t *baddie) {
  if (!az_vwithin(proj->position, baddie->position,
                  baddie->data->bounding_radius)) {
    return false;
  }

  switch (baddie->kind) {
    case AZ_BAD_LUMP:
      return true; // FIXME
    case AZ_BAD_TURRET:
      return true; // FIXME
    default: assert(false);
  }
  return false;
}

/*===========================================================================*/
