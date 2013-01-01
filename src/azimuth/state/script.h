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

#pragma once
#ifndef AZIMUTH_STATE_SCRIPT_H_
#define AZIMUTH_STATE_SCRIPT_H_

#include <stdbool.h>
#include <stdio.h> // for FILE

/*===========================================================================*/

typedef enum {
  AZ_OP_NOP = 0, // does nothing
  // Stack manipulation:
  AZ_OP_PUSH, // push i onto the stack
  // Branches:
  AZ_OP_BEQZ, // pop top, add i to PC if it is zero
  AZ_OP_BNEZ, // pop top, add i to PC if it is not zero
  AZ_OP_JUMP, // add i to PC
  // Flags:
  AZ_OP_TEST, // push 1 if flag i is set, else 0
  AZ_OP_SET, // set flag i
  AZ_OP_CLR, // clear flag i
  // Baddies:
  AZ_OP_BAD, // pop top four, add baddie of kind a, at (b, c), with angle d
  // Termination:
  AZ_OP_STOP, // halt script successfully
  AZ_OP_ERROR // halt script and printf execution state
} az_opcode_t;

const char *az_opcode_name(az_opcode_t opcode);

/*===========================================================================*/

typedef struct {
  az_opcode_t opcode;
  double immediate;
} az_instruction_t;

typedef struct {
  int num_instructions;
  az_instruction_t *instructions;
} az_script_t;

// Serialize the script to a string or file and return true, or return false on
// error.
bool az_fprint_script(const az_script_t *script, FILE *file);
bool az_sprint_script(const az_script_t *script, char *buffer, int length);

// Parse and allocate script and return true, or return false on error.
bool az_fscan_script(FILE *file, az_script_t *script_out);
bool az_sscan_script(const char *string, int length, az_script_t *script_out);

// Delete the data array owned by a script (but not the script object itself).
void az_destroy_script(az_script_t *script);

/*===========================================================================*/

#endif // AZIMUTH_STATE_SCRIPT_H_
