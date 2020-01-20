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

void expect_valid_controls(const az_control_mapping_t *control_mapping) {
  for (int i = AZ_FIRST_CONTROL; i < AZ_NUM_CONTROLS; ++i) {
    EXPECT_TRUE(az_is_valid_prefs_key(control_mapping->key_for_control[i],
                                      (az_control_id_t)i));
  }
}

void expect_unique_keys_for_controls(const az_control_mapping_t *control_mapping) {
  for (int i = AZ_FIRST_CONTROL + 1; i < AZ_NUM_CONTROLS; ++i) {
    for (int j = AZ_FIRST_CONTROL; j < i; ++j) {
      EXPECT_FALSE(control_mapping->key_for_control[i] ==
                   control_mapping->key_for_control[j]);
    }
  }
}

void expect_working_bidirectional_mapping(const az_control_mapping_t *control_mapping) {
  for (int i = AZ_FIRST_CONTROL; i < AZ_NUM_CONTROLS; ++i) {
    const az_key_id_t key_id = control_mapping->key_for_control[i];
    EXPECT_TRUE((int)control_mapping->control_for_key[(int)key_id] == i);
  }
}

void expect_weapons_from_keys_0_to_9(const az_control_mapping_t *control_mapping) {  
  for (int i = 0; i < 10; ++i) {
    const int control_id = (int)control_mapping->control_for_key[(int)AZ_KEY_0 + i];
    EXPECT_INT_EQ(control_id, (int)AZ_CONTROL_BOMBS + i);
  }
}

void expect_controls_to_match(const az_control_mapping_t *actual,
                              const az_control_mapping_t *expected) {
  for (int i = AZ_FIRST_CONTROL; i < AZ_NUM_CONTROLS; ++i) {
    EXPECT_INT_EQ(expected->key_for_control[i], actual->key_for_control[i]);
  }
  for (int i = 1; i < AZ_NUM_ALLOWED_KEYS; ++i) {
    EXPECT_INT_EQ(expected->control_for_key[i], actual->control_for_key[i]);
  }
}

void test_prefs_defaults(void) {
  az_preferences_t prefs = { .music_volume = 0.5f };
  az_reset_prefs_to_defaults(&prefs);

  expect_valid_controls(&prefs.control_mapping);
  expect_unique_keys_for_controls(&prefs.control_mapping);
  expect_working_bidirectional_mapping(&prefs.control_mapping);
  expect_weapons_from_keys_0_to_9(&prefs.control_mapping);
}

void test_prefs_save_load(void) {
  const az_preferences_t expected_prefs = {
    .music_volume = 0.125f, .sound_volume = 0.75f,
    .fullscreen_on_startup = false, .speedrun_timer = true,
    .control_mapping = {
      .key_for_control = {
        [AZ_CONTROL_UP]      = AZ_KEY_M,
        [AZ_CONTROL_DOWN]    = AZ_KEY_A,
        [AZ_CONTROL_RIGHT]   = AZ_KEY_G,
        [AZ_CONTROL_LEFT]    = AZ_KEY_N,
        [AZ_CONTROL_FIRE]    = AZ_KEY_E,
        [AZ_CONTROL_ORDN]    = AZ_KEY_T,
        [AZ_CONTROL_UTIL]    = AZ_KEY_I,
        [AZ_CONTROL_PAUSE]   = AZ_KEY_C,
        [AZ_CONTROL_BOMBS]   = AZ_KEY_SPACE,
        [AZ_CONTROL_CHARGE]  = AZ_KEY_B,
        [AZ_CONTROL_FREEZE]  = AZ_KEY_U,
        [AZ_CONTROL_TRIPLE]  = AZ_KEY_R,
        [AZ_CONTROL_HOMING]  = AZ_KEY_P,
        [AZ_CONTROL_PHASE]   = AZ_KEY_COMMA,
        [AZ_CONTROL_BURST]   = AZ_KEY_LEFT_ALT,
        [AZ_CONTROL_PIERCE]  = AZ_KEY_HOME,
        [AZ_CONTROL_BEAM]    = AZ_KEY_END,
        [AZ_CONTROL_ROCKETS] = AZ_KEY_LEFT_SUPER,
      },
      .control_for_key = {
        [AZ_KEY_M]          = AZ_CONTROL_UP,
        [AZ_KEY_A]          = AZ_CONTROL_DOWN,
        [AZ_KEY_G]          = AZ_CONTROL_RIGHT,
        [AZ_KEY_N]          = AZ_CONTROL_LEFT,
        [AZ_KEY_E]          = AZ_CONTROL_FIRE,
        [AZ_KEY_T]          = AZ_CONTROL_ORDN,
        [AZ_KEY_I]          = AZ_CONTROL_UTIL,
        [AZ_KEY_C]          = AZ_CONTROL_PAUSE,
        [AZ_KEY_SPACE]      = AZ_CONTROL_BOMBS,
        [AZ_KEY_B]          = AZ_CONTROL_CHARGE,
        [AZ_KEY_U]          = AZ_CONTROL_FREEZE,
        [AZ_KEY_R]          = AZ_CONTROL_TRIPLE,
        [AZ_KEY_P]          = AZ_CONTROL_HOMING,
        [AZ_KEY_COMMA]      = AZ_CONTROL_PHASE,
        [AZ_KEY_LEFT_ALT]   = AZ_CONTROL_BURST,
        [AZ_KEY_HOME]       = AZ_CONTROL_PIERCE,
        [AZ_KEY_END]        = AZ_CONTROL_BEAM,
        [AZ_KEY_LEFT_SUPER] = AZ_CONTROL_ROCKETS,
        // These are overloads that stick around always:
        [AZ_KEY_0] = AZ_CONTROL_BOMBS,
        [AZ_KEY_1] = AZ_CONTROL_CHARGE,
        [AZ_KEY_2] = AZ_CONTROL_FREEZE,
        [AZ_KEY_3] = AZ_CONTROL_TRIPLE,
        [AZ_KEY_4] = AZ_CONTROL_HOMING,
        [AZ_KEY_5] = AZ_CONTROL_PHASE,
        [AZ_KEY_6] = AZ_CONTROL_BURST,
        [AZ_KEY_7] = AZ_CONTROL_PIERCE,
        [AZ_KEY_8] = AZ_CONTROL_BEAM,
        [AZ_KEY_9] = AZ_CONTROL_ROCKETS,
      },
    },
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
  expect_controls_to_match(&actual_prefs.control_mapping,
                           &expected_prefs.control_mapping);
}

void test_prefs_missing_values(void) {
  az_preferences_t default_prefs;
  az_reset_prefs_to_defaults(&default_prefs);
  az_preferences_t actual_prefs;
  {
    FILE *file = tmpfile();
    ASSERT_TRUE(file != NULL);
    EXPECT_TRUE(fputs("@F st=1   sv=-1 \n mv=1.5", file) >= 0);
    rewind(file);
    EXPECT_TRUE(az_load_prefs_from_file(file, &actual_prefs));
    fclose(file);
  }
  RETURN_IF_FAILED();
  EXPECT_APPROX(0, actual_prefs.sound_volume);
  EXPECT_APPROX(1, actual_prefs.music_volume);
  EXPECT_TRUE(actual_prefs.speedrun_timer);
  EXPECT_TRUE(actual_prefs.fullscreen_on_startup ==
              default_prefs.fullscreen_on_startup);
  expect_controls_to_match(&actual_prefs.control_mapping,
                           &default_prefs.control_mapping);
}

/*===========================================================================*/
