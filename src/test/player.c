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

#include "azimuth/state/player.h"
#include "test/test.h"

/*===========================================================================*/

void test_player_give_upgrade(void) {
  az_player_t player = {.max_shields = 100};
  EXPECT_FALSE(az_has_upgrade(&player, AZ_UPG_GUN_HOMING));
  EXPECT_FALSE(az_has_upgrade(&player, AZ_UPG_SHIELD_BATTERY_10));

  az_give_upgrade(&player, AZ_UPG_SHIELD_BATTERY_10);
  EXPECT_FALSE(az_has_upgrade(&player, AZ_UPG_GUN_HOMING));
  EXPECT_TRUE(az_has_upgrade(&player, AZ_UPG_SHIELD_BATTERY_10));
  EXPECT_TRUE(player.max_shields == 125);

  az_give_upgrade(&player, AZ_UPG_SHIELD_BATTERY_10);
  EXPECT_FALSE(az_has_upgrade(&player, AZ_UPG_GUN_HOMING));
  EXPECT_TRUE(az_has_upgrade(&player, AZ_UPG_SHIELD_BATTERY_10));
  EXPECT_TRUE(player.max_shields == 125);

  az_give_upgrade(&player, AZ_UPG_GUN_HOMING);
  EXPECT_TRUE(az_has_upgrade(&player, AZ_UPG_GUN_HOMING));
  EXPECT_TRUE(az_has_upgrade(&player, AZ_UPG_SHIELD_BATTERY_10));
  EXPECT_TRUE(player.max_shields == 125);

  az_give_upgrade(&player, AZ_UPG_SHIELD_BATTERY_04);
  EXPECT_TRUE(player.max_shields == 150);
}

void test_player_set_room_visited(void) {
  az_player_t player = {.max_shields = 100};
  EXPECT_FALSE(az_test_room_visited(&player, 37));
  EXPECT_FALSE(az_test_room_visited(&player, 38));
  EXPECT_FALSE(az_test_room_visited(&player, 100));
  EXPECT_FALSE(az_test_room_visited(&player, 150));

  az_set_room_visited(&player, 100);
  EXPECT_FALSE(az_test_room_visited(&player, 37));
  EXPECT_FALSE(az_test_room_visited(&player, 38));
  EXPECT_TRUE(az_test_room_visited(&player, 100));
  EXPECT_FALSE(az_test_room_visited(&player, 150));

  az_set_room_visited(&player, 38);
  EXPECT_FALSE(az_test_room_visited(&player, 37));
  EXPECT_TRUE(az_test_room_visited(&player, 38));
  EXPECT_TRUE(az_test_room_visited(&player, 100));
  EXPECT_FALSE(az_test_room_visited(&player, 150));

  az_set_room_visited(&player, 150);
  EXPECT_FALSE(az_test_room_visited(&player, 37));
  EXPECT_TRUE(az_test_room_visited(&player, 38));
  EXPECT_TRUE(az_test_room_visited(&player, 100));
  EXPECT_TRUE(az_test_room_visited(&player, 150));

  az_set_room_visited(&player, 37);
  EXPECT_TRUE(az_test_room_visited(&player, 37));
  EXPECT_TRUE(az_test_room_visited(&player, 38));
  EXPECT_TRUE(az_test_room_visited(&player, 100));
  EXPECT_TRUE(az_test_room_visited(&player, 150));
}

/*===========================================================================*/
