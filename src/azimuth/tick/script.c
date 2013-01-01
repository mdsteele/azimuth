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

#include "azimuth/tick/script.h"

#include <assert.h>
#include <stdio.h>

#include "azimuth/state/script.h"
#include "azimuth/state/space.h"
#include "azimuth/util/misc.h"

/*===========================================================================*/

// How many instructions a script is allowed to execute before we terminate it
// with an error (to avoid infinite loops).
#define AZ_MAX_SCRIPT_STEPS 100

#ifdef NDEBUG
#define SCRIPT_ERROR(msg) do { return; } while (0)
#else

static void print_error(const char *msg, const az_script_t *script,
                        int pc, int stack_size, double *stack) {
  fprintf(stderr, "SCRIPT ERROR: %s\n  ", msg);
  az_fprint_script(script, stderr);
  fprintf(stderr, "\n  pc = %d\n  stack: ", pc);
  for (int i = 0; i < stack_size; ++i) {
    if (i != 0) fprintf(stderr, ", ");
    fprintf(stderr, "%.12g", stack[i]);
  }
  fputc('\n', stderr);
}

#define SCRIPT_ERROR(msg) do { \
    print_error(msg, script, pc, stack_size, stack); \
    return; \
  } while (0)

#endif // NDEBUG

#define STACK_PUSH(v) do { \
    if (stack_size < AZ_ARRAY_SIZE(stack)) { \
      stack[stack_size++] = (double)(v); \
    } else SCRIPT_ERROR("stack overflow"); \
  } while (0)

#define STACK_POP(p) do { \
    if (stack_size > 0) { \
      *(p) = stack[--stack_size]; \
    } else SCRIPT_ERROR("stack underflow"); \
  } while (0)

#define DO_JUMP() do { \
    const int new_pc = pc + (int)ins.immediate; \
    if (new_pc < 0 || new_pc > script->num_instructions) { \
      SCRIPT_ERROR("jump out of range"); \
    } \
    pc = new_pc - 1; \
  } while (0)

void az_run_script(az_space_state_t *state, const az_script_t *script) {
  assert(state != NULL);
  if (script == NULL || script->num_instructions == 0) return;
  else {
    assert(script->num_instructions > 0);
    assert(script->instructions != NULL);
  }
  int total_steps = 0;
  int pc = 0;
  int stack_size = 0;
  static double stack[24];
  while (pc < script->num_instructions) {
    if (++total_steps > AZ_MAX_SCRIPT_STEPS) SCRIPT_ERROR("ran for too long");
    az_instruction_t ins = script->instructions[pc];
    switch (ins.opcode) {
      case AZ_OP_NOP: break;
      // Stack manipulation:
      case AZ_OP_PUSH: STACK_PUSH(ins.immediate); break;
      // Branches:
      case AZ_OP_BEQZ:
        {
          double p;
          STACK_POP(&p);
          if (p == 0) DO_JUMP();
        }
        break;
      case AZ_OP_BNEZ:
        {
          double p;
          STACK_POP(&p);
          if (p != 0) DO_JUMP();
        }
        break;
      case AZ_OP_JUMP:
        DO_JUMP();
        break;
      // Flags:
      case AZ_OP_TEST:
        STACK_PUSH(az_test_flag(&state->ship.player,
                                (az_flag_t)ins.immediate));
        break;
      case AZ_OP_SET:
        az_set_flag(&state->ship.player, (az_flag_t)ins.immediate);
        break;
      case AZ_OP_CLR:
        az_clear_flag(&state->ship.player, (az_flag_t)ins.immediate);
        break;
      // Baddies:
      case AZ_OP_BAD:
        {
          double k, x, y, angle;
          STACK_POP(&angle); STACK_POP(&y); STACK_POP(&x); STACK_POP(&k);
          int kind = (int)k;
          if (kind < 1 || kind >= AZ_NUM_BADDIE_KINDS) {
            SCRIPT_ERROR("invalid baddie kind");
          }
          az_baddie_t *baddie;
          if (az_insert_baddie(state, &baddie)) {
            az_init_baddie(baddie, kind, (az_vector_t){x, y}, angle);
          }
        }
        break;
      // Termination:
      case AZ_OP_STOP: return;
      case AZ_OP_ERROR: SCRIPT_ERROR("ERROR opcode");
    }
    ++pc;
    assert(pc >= 0);
    assert(pc <= script->num_instructions);
  }
}

/*===========================================================================*/
