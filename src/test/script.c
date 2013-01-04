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

#include "azimuth/state/script.h"
#include "azimuth/util/misc.h"
#include "test/test.h"

/*===========================================================================*/

static const char script_string[] = "PUSH-23.5,NOP,BEQZ1,STOP;";

void test_script_print(void) {
  az_instruction_t instructions[] = {
    { .opcode = AZ_OP_PUSH, .immediate = -23.5 },
    { .opcode = AZ_OP_NOP },
    { .opcode = AZ_OP_BEQZ, .immediate = 1 },
    { .opcode = AZ_OP_STOP }
  };
  az_script_t script = { .num_instructions = AZ_ARRAY_SIZE(instructions),
                         .instructions = instructions };
  char buffer[100];
  ASSERT_TRUE(az_sprint_script(&script, buffer, sizeof(buffer)));
  EXPECT_STRING_EQ(script_string, buffer);
}

void test_script_scan(void) {
  az_script_t *script = az_sscan_script(script_string, sizeof(script_string));
  ASSERT_TRUE(script != NULL);
  EXPECT_INT_EQ(4, script->num_instructions);
  if (script->num_instructions >= 1) {
    EXPECT_INT_EQ(AZ_OP_PUSH, script->instructions[0].opcode);
    EXPECT_APPROX(-23.5, script->instructions[0].immediate);
  }
  if (script->num_instructions >= 2) {
    EXPECT_INT_EQ(AZ_OP_NOP, script->instructions[1].opcode);
    EXPECT_APPROX(0, script->instructions[1].immediate);
  }
  if (script->num_instructions >= 3) {
    EXPECT_INT_EQ(AZ_OP_BEQZ, script->instructions[2].opcode);
    EXPECT_APPROX(1, script->instructions[2].immediate);
  }
  if (script->num_instructions >= 4) {
    EXPECT_INT_EQ(AZ_OP_STOP, script->instructions[3].opcode);
    EXPECT_APPROX(0, script->instructions[3].immediate);
  }
  az_free_script(script);
}

/*===========================================================================*/
