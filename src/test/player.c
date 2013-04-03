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
  EXPECT_INT_EQ(125, player.max_shields);

  az_give_upgrade(&player, AZ_UPG_SHIELD_BATTERY_10);
  EXPECT_FALSE(az_has_upgrade(&player, AZ_UPG_GUN_HOMING));
  EXPECT_TRUE(az_has_upgrade(&player, AZ_UPG_SHIELD_BATTERY_10));
  EXPECT_INT_EQ(125, player.max_shields);

  az_give_upgrade(&player, AZ_UPG_GUN_HOMING);
  EXPECT_TRUE(az_has_upgrade(&player, AZ_UPG_GUN_HOMING));
  EXPECT_TRUE(az_has_upgrade(&player, AZ_UPG_SHIELD_BATTERY_10));
  EXPECT_INT_EQ(125, player.max_shields);

  az_give_upgrade(&player, AZ_UPG_SHIELD_BATTERY_04);
  EXPECT_INT_EQ(150, player.max_shields);
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

void test_player_set_zone_mapped(void) {
  az_player_t player = {.max_shields = 100};
  EXPECT_FALSE(az_test_zone_mapped(&player, 0));
  EXPECT_FALSE(az_test_zone_mapped(&player, 1));
  EXPECT_FALSE(az_test_zone_mapped(&player, 2));
  EXPECT_FALSE(az_test_zone_mapped(&player, 3));

  az_set_zone_mapped(&player, 2);
  EXPECT_FALSE(az_test_zone_mapped(&player, 0));
  EXPECT_FALSE(az_test_zone_mapped(&player, 1));
  EXPECT_TRUE(az_test_zone_mapped(&player, 2));
  EXPECT_FALSE(az_test_zone_mapped(&player, 3));

  az_set_zone_mapped(&player, 1);
  EXPECT_FALSE(az_test_zone_mapped(&player, 0));
  EXPECT_TRUE(az_test_zone_mapped(&player, 1));
  EXPECT_TRUE(az_test_zone_mapped(&player, 2));
  EXPECT_FALSE(az_test_zone_mapped(&player, 3));

  az_set_zone_mapped(&player, 3);
  EXPECT_FALSE(az_test_zone_mapped(&player, 0));
  EXPECT_TRUE(az_test_zone_mapped(&player, 1));
  EXPECT_TRUE(az_test_zone_mapped(&player, 2));
  EXPECT_TRUE(az_test_zone_mapped(&player, 3));

  az_set_zone_mapped(&player, 0);
  EXPECT_TRUE(az_test_zone_mapped(&player, 0));
  EXPECT_TRUE(az_test_zone_mapped(&player, 1));
  EXPECT_TRUE(az_test_zone_mapped(&player, 2));
  EXPECT_TRUE(az_test_zone_mapped(&player, 3));
}

void test_player_flags(void) {
  az_player_t player = {.max_shields = 100};
  EXPECT_FALSE(az_test_flag(&player, 6));
  EXPECT_FALSE(az_test_flag(&player, 47));

  az_set_flag(&player, 6);
  EXPECT_TRUE(az_test_flag(&player, 6));
  EXPECT_FALSE(az_test_flag(&player, 47));

  az_set_flag(&player, 47);
  EXPECT_TRUE(az_test_flag(&player, 6));
  EXPECT_TRUE(az_test_flag(&player, 47));

  az_clear_flag(&player, 6);
  EXPECT_FALSE(az_test_flag(&player, 6));
  EXPECT_TRUE(az_test_flag(&player, 47));

  az_clear_flag(&player, 47);
  EXPECT_FALSE(az_test_flag(&player, 6));
  EXPECT_FALSE(az_test_flag(&player, 47));
}

void test_select_gun(void) {
  az_player_t player = {.max_shields = 100};
  EXPECT_INT_EQ(AZ_GUN_NONE, player.gun1);
  EXPECT_INT_EQ(AZ_GUN_NONE, player.gun2);

  // If we select a gun we don't have, nothing happens.
  az_select_gun(&player, AZ_GUN_CHARGE);
  EXPECT_INT_EQ(AZ_GUN_NONE, player.gun1);
  EXPECT_INT_EQ(AZ_GUN_NONE, player.gun2);

  // When we get a new gun, it is selected.
  az_give_upgrade(&player, AZ_UPG_GUN_CHARGE);
  EXPECT_INT_EQ(AZ_GUN_CHARGE, player.gun1);
  EXPECT_INT_EQ(AZ_GUN_NONE, player.gun2);
  // If we select a gun we don't have, nothing happens.
  az_select_gun(&player, AZ_GUN_FREEZE);
  EXPECT_INT_EQ(AZ_GUN_CHARGE, player.gun1);
  EXPECT_INT_EQ(AZ_GUN_NONE, player.gun2);

  // When we get a new gun, it is selected.
  az_give_upgrade(&player, AZ_UPG_GUN_TRIPLE);
  EXPECT_INT_EQ(AZ_GUN_CHARGE, player.gun1);
  EXPECT_INT_EQ(AZ_GUN_TRIPLE, player.gun2);

  // When we get a new gun, it is selected.
  az_give_upgrade(&player, AZ_UPG_GUN_PHASE);
  EXPECT_INT_EQ(AZ_GUN_TRIPLE, player.gun1);
  EXPECT_INT_EQ(AZ_GUN_PHASE, player.gun2);
  // Selecting a gun swaps out the one selected least recently.
  az_select_gun(&player, AZ_GUN_CHARGE);
  EXPECT_INT_EQ(AZ_GUN_CHARGE, player.gun1);
  EXPECT_INT_EQ(AZ_GUN_PHASE, player.gun2);
  az_select_gun(&player, AZ_GUN_TRIPLE);
  EXPECT_INT_EQ(AZ_GUN_CHARGE, player.gun1);
  EXPECT_INT_EQ(AZ_GUN_TRIPLE, player.gun2);
  // Selecting a gun twice in a row normally has no effect.
  az_select_gun(&player, AZ_GUN_TRIPLE);
  EXPECT_INT_EQ(AZ_GUN_CHARGE, player.gun1);
  EXPECT_INT_EQ(AZ_GUN_TRIPLE, player.gun2);

  // Selecting the CHARGE gun twice in a row puts us back to CHARGE/NONE.
  az_select_gun(&player, AZ_GUN_CHARGE);
  EXPECT_INT_EQ(AZ_GUN_CHARGE, player.gun1);
  EXPECT_INT_EQ(AZ_GUN_TRIPLE, player.gun2);
  az_select_gun(&player, AZ_GUN_CHARGE);
  EXPECT_INT_EQ(AZ_GUN_CHARGE, player.gun1);
  EXPECT_INT_EQ(AZ_GUN_NONE, player.gun2);
  // Then we can select a second gun.
  az_select_gun(&player, AZ_GUN_PHASE);
  EXPECT_INT_EQ(AZ_GUN_CHARGE, player.gun1);
  EXPECT_INT_EQ(AZ_GUN_PHASE, player.gun2);
}

/*===========================================================================*/
