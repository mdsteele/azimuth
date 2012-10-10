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

#include "test/test.h"

#include "azimuth/util/random.h" // for az_init_random
#include "test/misc.h"
#include "test/player.h"
#include "test/polygon.h"
#include "test/random.h"
#include "test/uid.h"
#include "test/vector.h"

/*===========================================================================*/

int main(int argc, char **argv) {
  az_init_random();

  RUN_TEST(test_clock_mod);
  RUN_TEST(test_clock_zigzag);
  RUN_TEST(test_convex_polygon_contains);
  RUN_TEST(test_mod2pi);
  RUN_TEST(test_player_give_upgrade);
  RUN_TEST(test_polygon_contains);
  RUN_TEST(test_randint);
  RUN_TEST(test_random);
  RUN_TEST(test_uids);
  RUN_TEST(test_vector_polar);
  RUN_TEST(test_vector_rotate);

  return final_test_summary();
}

/*===========================================================================*/
