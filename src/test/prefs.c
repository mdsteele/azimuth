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

#include <stdio.h>

#include "azimuth/util/misc.h"
#include "azimuth/util/prefs.h"
#include "test/test.h"

/*===========================================================================*/

void test_prefs_defaults(void) {
  az_preferences_t prefs = { .music_volume = 0.5f };
  az_reset_prefs_to_defaults(&prefs);
  for (int i = 1; i < AZ_ARRAY_SIZE(prefs.keys); ++i) {
    for (int j = 0; j < i; ++j) {
      EXPECT_FALSE(prefs.keys[i] == prefs.keys[j]);
    }
  }
}

void test_prefs_save_load(void) {
  const az_preferences_t expected_prefs = {
    .music_volume = 0.125f, .sound_volume = 0.75f,
    .fullscreen_on_startup = false, .speedrun_timer = true,
    .keys = {
      [AZ_PREFS_UP_KEY_INDEX]    = AZ_KEY_M,
      [AZ_PREFS_DOWN_KEY_INDEX]  = AZ_KEY_A,
      [AZ_PREFS_RIGHT_KEY_INDEX] = AZ_KEY_G,
      [AZ_PREFS_LEFT_KEY_INDEX]  = AZ_KEY_N,
      [AZ_PREFS_FIRE_KEY_INDEX]  = AZ_KEY_E,
      [AZ_PREFS_ORDN_KEY_INDEX]  = AZ_KEY_T,
      [AZ_PREFS_UTIL_KEY_INDEX]  = AZ_KEY_I,
      [AZ_PREFS_PAUSE_KEY_INDEX] = AZ_KEY_C
    }
  };
  az_preferences_t actual_prefs;
  {
    FILE *file = tmpfile();
    ASSERT_TRUE(file != NULL);
    EXPECT_TRUE(az_save_prefs_to_file(&expected_prefs, file));
    rewind(file);
    EXPECT_TRUE(az_load_prefs_from_file(file, &actual_prefs));
    fclose(file);
  }
  RETURN_IF_FAILED();
  EXPECT_TRUE(actual_prefs.music_volume == expected_prefs.music_volume);
  EXPECT_TRUE(actual_prefs.sound_volume == expected_prefs.sound_volume);
  EXPECT_TRUE(actual_prefs.fullscreen_on_startup ==
              expected_prefs.fullscreen_on_startup);
  EXPECT_TRUE(actual_prefs.speedrun_timer == expected_prefs.speedrun_timer);
  for (int i = 0; i < AZ_PREFS_NUM_KEYS; ++i) {
    EXPECT_INT_EQ(expected_prefs.keys[i], actual_prefs.keys[i]);
  }
}

/*===========================================================================*/
