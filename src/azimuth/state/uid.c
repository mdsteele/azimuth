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

#include "azimuth/state/uid.h"

#include <assert.h>

/*===========================================================================*/

#define AZ_UID_INDEX_MASK 0xFFFFu
#define AZ_UID_COUNT_STEP 0x10000u

const az_uid_t AZ_NULL_UID = 0u;
const az_uid_t AZ_SHIP_UID = (az_uid_t)(-1);

void az_assign_uid(int index, az_uid_t *uid) {
  assert(index == (index & AZ_UID_INDEX_MASK));
  *uid = ((*uid + AZ_UID_COUNT_STEP) & ~AZ_UID_INDEX_MASK) | index;
}

int az_uid_index(az_uid_t uid) {
  return (int)(uid & AZ_UID_INDEX_MASK);
}

/*===========================================================================*/
