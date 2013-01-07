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

#include "azimuth/state/script.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h> // for free
#include <string.h> // for strcmp

#include "azimuth/util/misc.h"

/*===========================================================================*/

const char *az_opcode_name(az_opcode_t opcode) {
  switch (opcode) {
    case AZ_OP_NOP:    return "NOP";
    case AZ_OP_PUSH:   return "PUSH";
    case AZ_OP_ADD:    return "ADD";
    case AZ_OP_ADDI:   return "ADDI";
    case AZ_OP_BEQZ:   return "BEQZ";
    case AZ_OP_BNEZ:   return "BNEZ";
    case AZ_OP_JUMP:   return "JUMP";
    case AZ_OP_TEST:   return "TEST";
    case AZ_OP_SET:    return "SET";
    case AZ_OP_CLR:    return "CLR";
    case AZ_OP_BAD:    return "BAD";
    case AZ_OP_UNBAD:  return "UNBAD";
    case AZ_OP_LOCK:   return "LOCK";
    case AZ_OP_UNLOCK: return "UNLOCK";
    case AZ_OP_GETGS:  return "GETGS";
    case AZ_OP_SETGS:  return "SETGS";
    case AZ_OP_STOP:   return "STOP";
    case AZ_OP_ERROR:  return "ERROR";
  }
  AZ_ASSERT_UNREACHABLE();
}

static bool opcode_for_name(const char *name, az_opcode_t *opcode_out) {
  for (az_opcode_t op = AZ_OP_NOP; op <= AZ_OP_ERROR; ++op) {
    if (strcmp(name, az_opcode_name(op)) == 0) {
      *opcode_out = op;
      return true;
    }
  }
  return false;
}

/*===========================================================================*/

bool az_fprint_script(const az_script_t *script, FILE *file) {
  const int num_instructions = script->num_instructions;
  for (int i = 0; i < num_instructions; ++i) {
    const az_instruction_t ins = script->instructions[i];
    if (fputs(az_opcode_name(ins.opcode), file) < 0) return false;
    if (ins.immediate != 0.0) {
      if (fprintf(file, "%.12g", ins.immediate) < 0) return false;
    }
    if (fputc((i == num_instructions - 1 ? ';' : ','), file) < 0) return false;
  }
  return true;
}

bool az_sprint_script(const az_script_t *script, char *buffer, int length) {
  const int num_instructions = script->num_instructions;
  int written = 0;
  for (int i = 0; i < num_instructions; ++i) {
    const az_instruction_t ins = script->instructions[i];
    written += snprintf(buffer + written, length - written, "%s",
                        az_opcode_name(ins.opcode));
    if (written >= length) return false;
    if (ins.immediate != 0.0) {
      written += snprintf(buffer + written, length - written, "%.12g",
                          ins.immediate);
      if (written >= length) return false;
    }
    buffer[written++] = (i == num_instructions - 1 ? ';' : ',');
    if (written >= length) return false;
  }
  assert(written < length);
  buffer[written] = '\0';
  return true;
}

/*===========================================================================*/

az_script_t *az_fscan_script(FILE *file) {
  fpos_t pos;
  if (fgetpos(file, &pos) != 0) return NULL;
  int num_instructions = 1;
  for (bool finished = false; !finished;) {
    switch (fgetc(file)) {
      case EOF: return NULL;
      case ',': ++num_instructions; break;
      case ';': finished = true; break;
      default: break;
    }
  }
  if (fsetpos(file, &pos) != 0) return NULL;
  az_instruction_t *instructions =
    AZ_ALLOC(num_instructions, az_instruction_t);
  for (int i = 0; i < num_instructions; ++i) {
    char name[12];
    if (fscanf(file, "%11[A-Z]%lf", name,
               &instructions[i].immediate) == 0 ||
        !opcode_for_name(name, &instructions[i].opcode) ||
        fgetc(file) != (i == num_instructions - 1 ? ';' : ',')) {
      free(instructions);
      return NULL;
    }
  }
  az_script_t *script = AZ_ALLOC(1, az_script_t);
  script->num_instructions = num_instructions;
  script->instructions = instructions;
  return script;
}

az_script_t *az_sscan_script(const char *string, int length) {
  int num_instructions = 1;
  for (int i = 0;; ++i) {
    if (i >= length) return NULL;
    else if (string[i] == ';') break;
    else if (string[i] == ',') ++num_instructions;
  }
  az_instruction_t *instructions =
    AZ_ALLOC(num_instructions, az_instruction_t);
  int index = 0;
  for (int i = 0; i < num_instructions; ++i) {
    int num_read;
    char name[12];
    if (sscanf(string + index, "%11[A-Z]%n%lf%n", name, &num_read,
               &instructions[i].immediate, &num_read) == 0) {
      free(instructions);
      return NULL;
    }
    index += num_read;
    assert(index <= length);
    if (index >= length ||
        string[index++] != (i == num_instructions - 1 ? ';' : ',') ||
        !opcode_for_name(name, &instructions[i].opcode)) {
      free(instructions);
      return NULL;
    }
  }
  az_script_t *script = AZ_ALLOC(1, az_script_t);
  script->num_instructions = num_instructions;
  script->instructions = instructions;
  return script;
}

/*===========================================================================*/

void az_free_script(az_script_t *script) {
  if (script == NULL) return;
  free(script->instructions);
  free(script);
}

/*===========================================================================*/
