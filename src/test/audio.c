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

#include "azimuth/util/audio.h"
#include "test/test.h"

/*===========================================================================*/

void test_persist_sound(void) {
  az_soundboard_t soundboard = { .num_persists = 0 };
  az_persist_sound(&soundboard, AZ_SND_CPLUS_ACTIVE);
  az_reset_sound(&soundboard, AZ_SND_CHARGING_ORDNANCE);
  az_hold_sound(&soundboard, AZ_SND_CHARGING_GUN);
  az_persist_sound(&soundboard, AZ_SND_CHARGING_ORDNANCE);
  az_loop_sound(&soundboard, AZ_SND_BEAM_FREEZE);
  EXPECT_INT_EQ(4, soundboard.num_persists);

  EXPECT_INT_EQ(AZ_SND_CPLUS_ACTIVE, soundboard.persists[0].sound);
  EXPECT_TRUE(soundboard.persists[0].play);
  EXPECT_FALSE(soundboard.persists[0].loop);
  EXPECT_FALSE(soundboard.persists[0].reset);

  EXPECT_INT_EQ(AZ_SND_CHARGING_ORDNANCE, soundboard.persists[1].sound);
  EXPECT_TRUE(soundboard.persists[1].play);
  EXPECT_FALSE(soundboard.persists[1].loop);
  EXPECT_TRUE(soundboard.persists[1].reset);

  EXPECT_INT_EQ(AZ_SND_CHARGING_GUN, soundboard.persists[2].sound);
  EXPECT_FALSE(soundboard.persists[2].play);
  EXPECT_FALSE(soundboard.persists[2].loop);
  EXPECT_FALSE(soundboard.persists[2].reset);

  EXPECT_INT_EQ(AZ_SND_BEAM_FREEZE, soundboard.persists[3].sound);
  EXPECT_TRUE(soundboard.persists[3].play);
  EXPECT_TRUE(soundboard.persists[3].loop);
  EXPECT_FALSE(soundboard.persists[3].reset);
}

/*===========================================================================*/
