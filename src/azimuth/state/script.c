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
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "azimuth/util/misc.h"

/*===========================================================================*/

const char *az_opcode_name(az_opcode_t opcode) {
  switch (opcode) {
    case AZ_OP_NOP:    return "nop";
    case AZ_OP_PUSH:   return "push";
    case AZ_OP_POP:    return "pop";
    case AZ_OP_DUP:    return "dup";
    case AZ_OP_SWAP:   return "swap";
    case AZ_OP_ADD:    return "add";
    case AZ_OP_ADDI:   return "addi";
    case AZ_OP_SUB:    return "sub";
    case AZ_OP_SUBI:   return "subi";
    case AZ_OP_ISUB:   return "isub";
    case AZ_OP_MUL:    return "mul";
    case AZ_OP_MULI:   return "muli";
    case AZ_OP_DIV:    return "div";
    case AZ_OP_DIVI:   return "divi";
    case AZ_OP_IDIV:   return "idiv";
    case AZ_OP_MOD:    return "mod";
    case AZ_OP_MODI:   return "modi";
    case AZ_OP_MIN:    return "min";
    case AZ_OP_MINI:   return "mini";
    case AZ_OP_MAX:    return "max";
    case AZ_OP_MAXI:   return "maxi";
    case AZ_OP_ABS:    return "abs";
    case AZ_OP_MTAU:   return "mtau";
    case AZ_OP_RAND:   return "rand";
    case AZ_OP_SQRT:   return "sqrt";
    case AZ_OP_VADD:   return "vadd";
    case AZ_OP_VSUB:   return "vsub";
    case AZ_OP_VMUL:   return "vmul";
    case AZ_OP_VMULI:  return "vmuli";
    case AZ_OP_VNORM:  return "vnorm";
    case AZ_OP_VTHETA: return "vtheta";
    case AZ_OP_VPOLAR: return "vpolar";
    case AZ_OP_EQ:     return "eq";
    case AZ_OP_EQI:    return "eqi";
    case AZ_OP_NE:     return "ne";
    case AZ_OP_NEI:    return "nei";
    case AZ_OP_LT:     return "lt";
    case AZ_OP_LTI:    return "lti";
    case AZ_OP_GT:     return "gt";
    case AZ_OP_GTI:    return "gti";
    case AZ_OP_LE:     return "le";
    case AZ_OP_LEI:    return "lei";
    case AZ_OP_GE:     return "ge";
    case AZ_OP_GEI:    return "gei";
    case AZ_OP_TEST:   return "test";
    case AZ_OP_SET:    return "set";
    case AZ_OP_CLR:    return "clr";
    case AZ_OP_HAS:    return "has";
    case AZ_OP_MAP:    return "map";
    case AZ_OP_NIX:    return "nix";
    case AZ_OP_KILL:   return "kill";
    case AZ_OP_GPOS:   return "gpos";
    case AZ_OP_SPOS:   return "spos";
    case AZ_OP_GANG:   return "gang";
    case AZ_OP_SANG:   return "sang";
    case AZ_OP_GSTAT:  return "gstat";
    case AZ_OP_SSTAT:  return "sstat";
    case AZ_OP_ACTIV:  return "activ";
    case AZ_OP_DEACT:  return "deact";
    case AZ_OP_GVEL:   return "gvel";
    case AZ_OP_SVEL:   return "svel";
    case AZ_OP_BAD:    return "bad";
    case AZ_OP_SBADK:  return "sbadk";
    case AZ_OP_BOSS:   return "boss";
    case AZ_OP_OPEN:   return "open";
    case AZ_OP_CLOSE:  return "close";
    case AZ_OP_LOCK:   return "lock";
    case AZ_OP_UNLOCK: return "unlock";
    case AZ_OP_GSTR:   return "gstr";
    case AZ_OP_SSTR:   return "sstr";
    case AZ_OP_GCAM:   return "gcam";
    case AZ_OP_DARK:   return "dark";
    case AZ_OP_SHAKE:  return "shake";
    case AZ_OP_QUAKE:  return "quake";
    case AZ_OP_BOOM:   return "boom";
    case AZ_OP_NUKE:   return "nuke";
    case AZ_OP_BOLT:   return "bolt";
    case AZ_OP_NPS:    return "nps";
    case AZ_OP_FADO:   return "fado";
    case AZ_OP_FADI:   return "fadi";
    case AZ_OP_SCENE:  return "scene";
    case AZ_OP_MSG:    return "msg";
    case AZ_OP_DLOG:   return "dlog";
    case AZ_OP_PT:     return "pt";
    case AZ_OP_PB:     return "pb";
    case AZ_OP_TT:     return "tt";
    case AZ_OP_TB:     return "tb";
    case AZ_OP_DEND:   return "dend";
    case AZ_OP_MLOG:   return "mlog";
    case AZ_OP_TM:     return "tm";
    case AZ_OP_MEND:   return "mend";
    case AZ_OP_MUS:    return "mus";
    case AZ_OP_SND:    return "snd";
    case AZ_OP_WAIT:   return "wait";
    case AZ_OP_DOOM:   return "doom";
    case AZ_OP_SAFE:   return "safe";
    case AZ_OP_JUMP:   return "jump";
    case AZ_OP_BEQZ:   return "beqz";
    case AZ_OP_BNEZ:   return "bnez";
    case AZ_OP_HALT:   return "halt";
    case AZ_OP_HEQZ:   return "heqz";
    case AZ_OP_HNEZ:   return "hnez";
    case AZ_OP_VICT:   return "vict";
    case AZ_OP_ERROR:  return "error";
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
#ifndef NDEBUG
  fprintf(stderr, "script.c: no such opcode: %s\n", name);
#endif
  return false;
}

static bool should_print_immediate(az_instruction_t ins) {
  switch (ins.opcode) {
    case AZ_OP_NOP:
    case AZ_OP_ADD:
    case AZ_OP_SUB:
    case AZ_OP_MUL:
    case AZ_OP_DIV:
    case AZ_OP_MOD:
    case AZ_OP_MIN:
    case AZ_OP_MAX:
    case AZ_OP_ABS:
    case AZ_OP_MTAU:
    case AZ_OP_RAND:
    case AZ_OP_SQRT:
    case AZ_OP_VADD:
    case AZ_OP_VSUB:
    case AZ_OP_VMUL:
    case AZ_OP_VNORM:
    case AZ_OP_VTHETA:
    case AZ_OP_VPOLAR:
    case AZ_OP_EQ:
    case AZ_OP_NE:
    case AZ_OP_LT:
    case AZ_OP_GT:
    case AZ_OP_LE:
    case AZ_OP_GE:
    case AZ_OP_GVEL:
    case AZ_OP_SVEL:
    case AZ_OP_GCAM:
    case AZ_OP_BOOM:
    case AZ_OP_NUKE:
    case AZ_OP_FADO:
    case AZ_OP_FADI:
    case AZ_OP_DLOG:
    case AZ_OP_DEND:
    case AZ_OP_MLOG:
    case AZ_OP_MEND:
    case AZ_OP_SAFE:
    case AZ_OP_HALT:
    case AZ_OP_HEQZ:
    case AZ_OP_HNEZ:
    case AZ_OP_VICT:
    case AZ_OP_ERROR:
      return false;
    case AZ_OP_POP:
    case AZ_OP_DUP:
    case AZ_OP_SWAP:
    case AZ_OP_KILL:
    case AZ_OP_GPOS:
    case AZ_OP_SPOS:
    case AZ_OP_GANG:
    case AZ_OP_SANG:
    case AZ_OP_GSTAT:
    case AZ_OP_SSTAT:
    case AZ_OP_ACTIV:
    case AZ_OP_DEACT:
    case AZ_OP_BAD:
      return (ins.immediate != 0.0);
    case AZ_OP_PUSH:
    case AZ_OP_ADDI:
    case AZ_OP_SUBI:
    case AZ_OP_ISUB:
    case AZ_OP_MULI:
    case AZ_OP_DIVI:
    case AZ_OP_IDIV:
    case AZ_OP_MINI:
    case AZ_OP_MAXI:
    case AZ_OP_MODI:
    case AZ_OP_VMULI:
    case AZ_OP_EQI:
    case AZ_OP_NEI:
    case AZ_OP_LTI:
    case AZ_OP_GTI:
    case AZ_OP_LEI:
    case AZ_OP_GEI:
    case AZ_OP_TEST:
    case AZ_OP_SET:
    case AZ_OP_CLR:
    case AZ_OP_HAS:
    case AZ_OP_MAP:
    case AZ_OP_NIX:
    case AZ_OP_SBADK:
    case AZ_OP_BOSS:
    case AZ_OP_OPEN:
    case AZ_OP_CLOSE:
    case AZ_OP_LOCK:
    case AZ_OP_UNLOCK:
    case AZ_OP_GSTR:
    case AZ_OP_SSTR:
    case AZ_OP_DARK:
    case AZ_OP_SHAKE:
    case AZ_OP_QUAKE:
    case AZ_OP_BOLT:
    case AZ_OP_NPS:
    case AZ_OP_SCENE:
    case AZ_OP_MSG:
    case AZ_OP_PT:
    case AZ_OP_PB:
    case AZ_OP_TT:
    case AZ_OP_TB:
    case AZ_OP_TM:
    case AZ_OP_MUS:
    case AZ_OP_SND:
    case AZ_OP_WAIT:
    case AZ_OP_DOOM:
    case AZ_OP_JUMP:
    case AZ_OP_BEQZ:
    case AZ_OP_BNEZ:
      return true;
  }
  AZ_ASSERT_UNREACHABLE();
}

static bool is_jump(az_opcode_t opcode) {
  return opcode == AZ_OP_BEQZ || opcode == AZ_OP_BNEZ || opcode == AZ_OP_JUMP;
}

/*===========================================================================*/

typedef enum {
  AZ_FILE_WRITER,
  AZ_STRING_WRITER,
} az_writer_type_t;

typedef struct {
  az_writer_type_t type;
  union {
    FILE *file;
    struct { char *start; size_t size; } string;
  } data;
} az_writer_t;

static bool az_wprintf(az_writer_t *writer, const char *format, ...)
  __attribute__((__format__(__printf__,2,3)));
static bool az_wprintf(az_writer_t *writer, const char *format, ...) {
  va_list args;
  va_start(args, format);
  bool success = false;
  if (writer->type ==  AZ_FILE_WRITER) {
    success = (0 <= vfprintf(writer->data.file, format, args));
  } else {
    const int result = vsnprintf(writer->data.string.start,
                                 writer->data.string.size, format, args);
    if (result >= 0 && (size_t)result < writer->data.string.size) {
      writer->data.string.start += (size_t)result;
      writer->data.string.size -= (size_t)result;
      success = true;
    }
  }
  va_end(args);
  return success;
}

static void fill_jump_table(const az_script_t *script, char *jump_table) {
  memset(jump_table, '\0', script->num_instructions);
  int next_label = 'A';
  for (int i = 0; i < script->num_instructions; ++i) {
    const az_instruction_t ins = script->instructions[i];
    if (is_jump(ins.opcode)) {
      const int dest = i + (int)ins.immediate;
      if (dest >= 0 && dest < script->num_instructions &&
          jump_table[dest] == '\0') {
        jump_table[dest] = next_label;
        if (next_label == 'Z') break;
        ++next_label;
      }
    }
  }
}

static bool write_script(const az_script_t *script, az_writer_t *writer) {
  const int num_instructions = script->num_instructions;
  char jump_table[num_instructions];
  fill_jump_table(script, jump_table);
  for (int i = 0; i < num_instructions; ++i) {
    // Write the label that points to this instruction, if any.
    if (jump_table[i] != '\0') {
      if (!az_wprintf(writer, "%c#", jump_table[i])) return false;
    }
    // Write the instruction opcode.
    const az_instruction_t ins = script->instructions[i];
    if (!az_wprintf(writer, "%s", az_opcode_name(ins.opcode))) return false;
    // Determine if this instruction should use a label rather than a numeric
    // immediate value.
    char label = '\0';
    if (is_jump(ins.opcode)) {
      const int dest = i + (int)ins.immediate;
      if (dest == script->num_instructions) label = '@';
      else if (dest >= 0 && dest < script->num_instructions) {
        label = jump_table[dest];
      }
    }
    // Write the label/immediate for this instruction.
    if (label != '\0') {
      if (!az_wprintf(writer, "/%c", label)) return false;
    } else if (should_print_immediate(ins)) {
      if (!az_wprintf(writer, "%.12g", ins.immediate)) return false;
    }
    // Write a comma before the next instruction, or a semicolon if this is the
    // end of the script.
    if (!az_wprintf(writer, "%c", (i == num_instructions - 1 ? ';' : ','))) {
      return false;
    }
  }
  return true;
}

bool az_fprint_script(const az_script_t *script, FILE *file) {
  az_writer_t writer = { .type = AZ_FILE_WRITER, .data.file = file };
  return write_script(script, &writer);
}

bool az_sprint_script(const az_script_t *script, char *buffer, int length) {
  az_writer_t writer = { .type = AZ_STRING_WRITER,
                         .data.string = { .start = buffer, .size = length } };
  return write_script(script, &writer);
}

/*===========================================================================*/

static bool fscan_instructions(FILE *file, int num_instructions,
                               az_instruction_t *instructions) {
  int label_table[26] = {0};
  char jump_table[num_instructions];
  memset(jump_table, '\0', num_instructions);
  for (int i = 0; i < num_instructions; ++i) {
    char label[2];
    int num_read;
    if (fscanf(file, "%1[A-Z]#%n", label, &num_read) == 1) {
      const int label_index = label[0] - 'A';
      assert(label_index >= 0 && label_index < AZ_ARRAY_SIZE(label_table));
      label_table[label_index] = i;
      if (num_read != 2) return false;
    }
    char name[12];
    if (fscanf(file, "%11[a-z]%lf", name,
               &instructions[i].immediate) == 0 ||
        !opcode_for_name(name, &instructions[i].opcode)) {
      return false;
    }
    if (fscanf(file, "/%1[@A-Z]", label) == 1) {
      jump_table[i] = label[0];
    }
    if (fgetc(file) != (i == num_instructions - 1 ? ';' : ',')) {
      return false;
    }
  }
  // Use label_table and jump_table to set the immediate values for jump
  // instructions that used labels.
  for (int i = 0; i < num_instructions; ++i) {
    const char label = jump_table[i];
    if (label == '\0') continue;
    else if (label == '@') {
      instructions[i].immediate = num_instructions - i;
    } else {
      assert('A' <= label && label <= 'Z');
      instructions[i].immediate = label_table[label - 'A'] - i;
    }
  }
  return true;
}

az_script_t *az_fscan_script(FILE *file) {
  // Scan ahead and determine how many instructions long this script is.
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
  // Go back to the start of the script.
  if (fsetpos(file, &pos) != 0) return NULL;
  // Parse the instructions.
  az_instruction_t *instructions =
    AZ_ALLOC(num_instructions, az_instruction_t);
  if (!fscan_instructions(file, num_instructions, instructions)) {
    free(instructions);
    return NULL;
  }
  az_script_t *script = AZ_ALLOC(1, az_script_t);
  script->num_instructions = num_instructions;
  script->instructions = instructions;
  return script;
}

az_script_t *az_sscan_script(const char *string, int length) {
  FILE *file = tmpfile();
  if (file == NULL) return NULL;
  az_script_t *script = NULL;
  if (fputs(string, file) >= 0) {
    rewind(file);
    script = az_fscan_script(file);
  }
  fclose(file);
  return script;
}

/*===========================================================================*/

az_script_t *az_clone_script(const az_script_t *script) {
  if (script == NULL) return NULL;
  az_script_t *clone = AZ_ALLOC(1, az_script_t);
  clone->num_instructions = script->num_instructions;
  clone->instructions = AZ_ALLOC(clone->num_instructions, az_instruction_t);
  memcpy(clone->instructions, script->instructions,
         clone->num_instructions * sizeof(az_instruction_t));
  return clone;
}

void az_free_script(az_script_t *script) {
  if (script == NULL) return;
  free(script->instructions);
  free(script);
}

/*===========================================================================*/
