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

#include <stdlib.h>

#include "azimuth/state/dialog.h"
#include "azimuth/util/key.h"
#include "azimuth/util/prefs.h"
#include "test/test.h"

/*===========================================================================*/

static const char input_string[] = "\"\n"
  "  This $$ is a \\\"$Gtest$W\\\".  However$_10, $Xface42it is\n"
  "  $Ronly$W a test.  Press [$u].\"";
static const char paragraph[] =
  "This $$ is a \"$Gtest$W\".  However$_10, $Xface42it is\n"
  "$Ronly$W a test.  Press [$u].";

void test_paragraph_read(void) {
  az_reader_t reader;
  az_cstring_reader(input_string, &reader);
  char *actual_paragraph = az_read_paragraph(&reader);
  az_rclose(&reader);
  ASSERT_TRUE(actual_paragraph != NULL);
  EXPECT_STRING_EQ(paragraph, actual_paragraph);
  free(actual_paragraph);
}

void test_paragraph_length(void) {
  az_preferences_t prefs;
  az_reset_prefs_to_defaults(&prefs);

  EXPECT_INT_EQ(1, az_paragraph_num_lines(""));
  EXPECT_INT_EQ(0, az_paragraph_line_length(&prefs, "", 0));
  EXPECT_INT_EQ(0, az_paragraph_total_length(&prefs, ""));

  EXPECT_INT_EQ(2, az_paragraph_num_lines(paragraph));
  EXPECT_INT_EQ(45, az_paragraph_line_length(&prefs, paragraph, 0));
  EXPECT_INT_EQ(69, az_paragraph_total_length(&prefs, paragraph));

  prefs.keys[AZ_PREFS_UP_KEY_INDEX] = AZ_KEY_TAB;
  EXPECT_INT_EQ(71, az_paragraph_total_length(&prefs, paragraph));

  // Now test some paragraphs with bad escapes:
  EXPECT_INT_EQ(5, az_paragraph_total_length(&prefs, "Wr$_XXong$_4"));
  EXPECT_INT_EQ(5, az_paragraph_total_length(&prefs, "ER$XrrggbbROR$X44"));
}

/*===========================================================================*/
