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

#include "test/uid.h"

#include "azimuth/state/uid.h"
#include "test/test.h"

/*===========================================================================*/

void test_uids(void) {
  const az_uid_t null_uid = 0;

  az_uid_t uid0 = null_uid;
  az_assign_uid(0, &uid0);
  EXPECT_TRUE(az_uid_index(uid0) == 0);
  EXPECT_TRUE(uid0 != null_uid);

  az_uid_t uid1 = null_uid;
  az_assign_uid(1, &uid1);
  EXPECT_TRUE(az_uid_index(uid1) == 1);
  EXPECT_TRUE(uid1 != null_uid);
  EXPECT_TRUE(uid1 != uid0);

  az_uid_t uid2 = null_uid;
  az_assign_uid(2, &uid2);
  EXPECT_TRUE(az_uid_index(uid2) == 2);
  EXPECT_TRUE(uid2 != null_uid);
  EXPECT_TRUE(uid2 != uid0);
  EXPECT_TRUE(uid2 != uid1);

  const az_uid_t uid0a = uid0;
  az_assign_uid(0, &uid0);
  EXPECT_TRUE(az_uid_index(uid0) == 0);
  EXPECT_TRUE(uid0 != uid0a);
  EXPECT_TRUE(uid0 != null_uid);
  EXPECT_TRUE(uid0 != uid1);
  EXPECT_TRUE(uid0 != uid2);

  const az_uid_t uid1a = uid1;
  az_assign_uid(1, &uid1);
  EXPECT_TRUE(az_uid_index(uid1) == 1);
  EXPECT_TRUE(uid1 != uid1a);
  EXPECT_TRUE(uid1 != uid0a);
  EXPECT_TRUE(uid1 != null_uid);
  EXPECT_TRUE(uid1 != uid0);
  EXPECT_TRUE(uid1 != uid2);

  const az_uid_t uid0b = uid0;
  az_assign_uid(0, &uid0);
  EXPECT_TRUE(az_uid_index(uid0) == 0);
  EXPECT_TRUE(uid0 != uid0b);
  EXPECT_TRUE(uid0 != uid0a);
  EXPECT_TRUE(uid0 != uid1a);
  EXPECT_TRUE(uid0 != null_uid);
  EXPECT_TRUE(uid0 != uid1);
  EXPECT_TRUE(uid0 != uid2);
}

/*===========================================================================*/
