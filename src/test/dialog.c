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

#include <string.h>

#include "azimuth/state/dialog.h"
#include "azimuth/util/misc.h"
#include "test/test.h"

/*===========================================================================*/

static const char text_string1[] = "\n"
  "  This is a $Gtest$W.  However, $Xface42it is\n"
  "  $Ronly$W a test.";

void test_text_scan1(void) {
  az_text_t text;
  ASSERT_TRUE(az_sscan_text(text_string1, strlen(text_string1), &text));
  EXPECT_INT_EQ(2, text.num_lines);
  if (text.num_lines >= 1) {
    const az_text_line_t *line = &text.lines[0];
    EXPECT_INT_EQ(31, line->total_length);
    EXPECT_INT_EQ(4, line->num_fragments);
    if (line->num_fragments >= 1) {
      const az_text_fragment_t *fragment = &line->fragments[0];
      EXPECT_STRING_N_EQ("This is a ", fragment->chars, fragment->length);
      EXPECT_INT_EQ(255, fragment->color.r);
      EXPECT_INT_EQ(255, fragment->color.g);
      EXPECT_INT_EQ(255, fragment->color.b);
    }
    if (line->num_fragments >= 2) {
      const az_text_fragment_t *fragment = &line->fragments[1];
      EXPECT_STRING_N_EQ("test", fragment->chars, fragment->length);
      EXPECT_INT_EQ(0, fragment->color.r);
      EXPECT_INT_EQ(255, fragment->color.g);
      EXPECT_INT_EQ(0, fragment->color.b);
    }
    if (line->num_fragments >= 3) {
      const az_text_fragment_t *fragment = &line->fragments[2];
      EXPECT_STRING_N_EQ(".  However, ", fragment->chars, fragment->length);
      EXPECT_INT_EQ(255, fragment->color.r);
      EXPECT_INT_EQ(255, fragment->color.g);
      EXPECT_INT_EQ(255, fragment->color.b);
    }
    if (line->num_fragments >= 4) {
      const az_text_fragment_t *fragment = &line->fragments[3];
      EXPECT_STRING_N_EQ("it is", fragment->chars, fragment->length);
      EXPECT_INT_EQ(0xfa, fragment->color.r);
      EXPECT_INT_EQ(0xce, fragment->color.g);
      EXPECT_INT_EQ(0x42, fragment->color.b);
    }
  }
  if (text.num_lines >= 2) {
    const az_text_line_t *line = &text.lines[1];
    EXPECT_INT_EQ(12, line->total_length);
    EXPECT_INT_EQ(2, line->num_fragments);
    if (line->num_fragments >= 1) {
      const az_text_fragment_t *fragment = &line->fragments[0];
      EXPECT_STRING_N_EQ("only", fragment->chars, fragment->length);
      EXPECT_INT_EQ(255, fragment->color.r);
      EXPECT_INT_EQ(0, fragment->color.g);
      EXPECT_INT_EQ(0, fragment->color.b);
    }
    if (line->num_fragments >= 2) {
      const az_text_fragment_t *fragment = &line->fragments[1];
      EXPECT_STRING_N_EQ(" a test.", fragment->chars, fragment->length);
      EXPECT_INT_EQ(255, fragment->color.r);
      EXPECT_INT_EQ(255, fragment->color.g);
      EXPECT_INT_EQ(255, fragment->color.b);
    }
  }
  az_destroy_text(&text);
  EXPECT_INT_EQ(0, text.num_lines);
  EXPECT_TRUE(text.lines == NULL);
}

static const char text_string2[] = "\n"
  "  This is $Calso a\n" // colors should persist across linebreaks
  "  test$W, apparently.";

void test_text_scan2(void) {
  az_text_t text;
  ASSERT_TRUE(az_sscan_text(text_string2, strlen(text_string2), &text));
  EXPECT_INT_EQ(2, text.num_lines);
  if (text.num_lines >= 1) {
    const az_text_line_t *line = &text.lines[0];
    EXPECT_INT_EQ(14, line->total_length);
    EXPECT_INT_EQ(2, line->num_fragments);
    if (line->num_fragments >= 1) {
      const az_text_fragment_t *fragment = &line->fragments[0];
      EXPECT_STRING_N_EQ("This is ", fragment->chars, fragment->length);
      EXPECT_INT_EQ(255, fragment->color.r);
      EXPECT_INT_EQ(255, fragment->color.g);
      EXPECT_INT_EQ(255, fragment->color.b);
    }
    if (line->num_fragments >= 2) {
      const az_text_fragment_t *fragment = &line->fragments[1];
      EXPECT_STRING_N_EQ("also a", fragment->chars, fragment->length);
      EXPECT_INT_EQ(0, fragment->color.r);
      EXPECT_INT_EQ(255, fragment->color.g);
      EXPECT_INT_EQ(255, fragment->color.b);
    }
  }
  if (text.num_lines >= 2) {
    const az_text_line_t *line = &text.lines[1];
    EXPECT_INT_EQ(17, line->total_length);
    EXPECT_INT_EQ(2, line->num_fragments);
    if (line->num_fragments >= 1) {
      const az_text_fragment_t *fragment = &line->fragments[0];
      EXPECT_STRING_N_EQ("test", fragment->chars, fragment->length);
      EXPECT_INT_EQ(0, fragment->color.r);
      EXPECT_INT_EQ(255, fragment->color.g);
      EXPECT_INT_EQ(255, fragment->color.b);
    }
    if (line->num_fragments >= 2) {
      const az_text_fragment_t *fragment = &line->fragments[1];
      EXPECT_STRING_N_EQ(", apparently.", fragment->chars, fragment->length);
      EXPECT_INT_EQ(255, fragment->color.r);
      EXPECT_INT_EQ(255, fragment->color.g);
      EXPECT_INT_EQ(255, fragment->color.b);
    }
  }
  az_destroy_text(&text);
  EXPECT_INT_EQ(0, text.num_lines);
  EXPECT_TRUE(text.lines == NULL);
}

// Test cloning a text object, and then test that we can destroy them
// individually.
void test_text_clone(void) {
  az_text_t text1 = {.num_lines = 0};
  ASSERT_TRUE(az_sscan_text(text_string1, strlen(text_string1), &text1));
  EXPECT_INT_EQ(2, text1.num_lines);
  az_text_t text2 = {.num_lines = 0};
  az_clone_text(&text1, &text2);
  EXPECT_INT_EQ(2, text2.num_lines);
  az_destroy_text(&text1);
  EXPECT_INT_EQ(0, text1.num_lines);
  EXPECT_INT_EQ(2, text2.num_lines);
  az_destroy_text(&text2);
}

/*===========================================================================*/
