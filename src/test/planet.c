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

#include "azimuth/state/planet.h"
#include "azimuth/state/player.h"
#include "test/test.h"

/*===========================================================================*/

void test_hint_matches(void) {
  az_player_t player = {.max_shields = 100};
  az_give_upgrade(&player, AZ_UPG_GUN_CHARGE);
  az_set_flag(&player, 4);
  az_hint_t hint;
  // Test where we already have result:
  hint = (az_hint_t){
    .result = AZ_UPG_GUN_CHARGE
  };
  EXPECT_FALSE(az_hint_matches(&hint, &player));
  // Test no prereqs:
  hint = (az_hint_t){
    .result = AZ_UPG_GUN_FREEZE
  };
  EXPECT_TRUE(az_hint_matches(&hint, &player));
  // Test one prereq:
  hint = (az_hint_t){
    .properties = (AZ_HINTF_OP2_IS_AND),
    .prereq2 = AZ_UPG_GUN_FREEZE,
    .result = AZ_UPG_GUN_TRIPLE
  };
  EXPECT_FALSE(az_hint_matches(&hint, &player));
  hint = (az_hint_t){
    .properties = (AZ_HINTF_OP2_IS_AND),
    .prereq2 = AZ_UPG_GUN_CHARGE,
    .result = AZ_UPG_GUN_TRIPLE
  };
  EXPECT_TRUE(az_hint_matches(&hint, &player));
  // Test prereq1 AND prereq2:
  hint = (az_hint_t){
    .properties = (AZ_HINTF_OP1_IS_AND | AZ_HINTF_OP2_IS_AND |
                   AZ_HINTF_PREREQ2_IS_FLAG),
    .prereq1 = AZ_UPG_GUN_CHARGE,
    .prereq2 = 4,
    .result = AZ_UPG_GUN_TRIPLE
  };
  EXPECT_TRUE(az_hint_matches(&hint, &player));
  hint = (az_hint_t){
    .properties = (AZ_HINTF_OP1_IS_AND | AZ_HINTF_OP2_IS_AND |
                   AZ_HINTF_PREREQ2_IS_FLAG),
    .prereq1 = AZ_UPG_GUN_FREEZE,
    .prereq2 = 4,
    .result = AZ_UPG_GUN_TRIPLE
  };
  EXPECT_FALSE(az_hint_matches(&hint, &player));
  hint = (az_hint_t){
    .properties = (AZ_HINTF_OP1_IS_AND | AZ_HINTF_OP2_IS_AND |
                   AZ_HINTF_PREREQ2_IS_FLAG),
    .prereq1 = AZ_UPG_GUN_CHARGE,
    .prereq2 = 5,
    .result = AZ_UPG_GUN_TRIPLE
  };
  EXPECT_FALSE(az_hint_matches(&hint, &player));
  // Test prereq1 OR prereq2:
  hint = (az_hint_t){
    .properties = (AZ_HINTF_OP1_IS_AND | AZ_HINTF_PREREQ2_IS_FLAG),
    .prereq1 = AZ_UPG_GUN_FREEZE,
    .prereq2 = 5,
    .result = AZ_UPG_GUN_TRIPLE
  };
  EXPECT_FALSE(az_hint_matches(&hint, &player));
  hint = (az_hint_t){
    .properties = (AZ_HINTF_OP1_IS_AND | AZ_HINTF_PREREQ2_IS_FLAG),
    .prereq1 = AZ_UPG_GUN_FREEZE,
    .prereq2 = 4,
    .result = AZ_UPG_GUN_TRIPLE
  };
  EXPECT_TRUE(az_hint_matches(&hint, &player));
  hint = (az_hint_t){
    .properties = (AZ_HINTF_OP1_IS_AND | AZ_HINTF_PREREQ2_IS_FLAG),
    .prereq1 = AZ_UPG_GUN_CHARGE,
    .prereq2 = 54,
    .result = AZ_UPG_GUN_TRIPLE
  };
  EXPECT_TRUE(az_hint_matches(&hint, &player));
}

/*===========================================================================*/
