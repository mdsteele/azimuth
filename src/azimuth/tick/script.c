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
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "azimuth/state/dialog.h"
#include "azimuth/state/object.h"
#include "azimuth/state/script.h"
#include "azimuth/state/space.h"
#include "azimuth/tick/object.h"
#include "azimuth/util/audio.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/random.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

// How many instructions a script is allowed to execute before we terminate it
// with an error (to avoid infinite loops).
#define AZ_MAX_SCRIPT_STEPS 1000

#ifdef NDEBUG
#define SCRIPT_ERROR(msg) do { goto halt; } while (0)
#else

static void print_error(const char *msg, const az_script_vm_t *vm) {
  fprintf(stderr, "SCRIPT ERROR: %s\n  ", msg);
  az_fprint_script(vm->script, stderr);
  fprintf(stderr, "\n  pc = %d\n  stack: ", vm->pc);
  for (int i = 0; i < vm->stack_size; ++i) {
    if (i != 0) fprintf(stderr, ", ");
    fprintf(stderr, "%.12g", vm->stack[i]);
  }
  fputc('\n', stderr);
}

#define SCRIPT_ERROR(msg) do { \
    print_error(msg, vm); \
    goto halt; \
  } while (0)

#endif // NDEBUG

/*===========================================================================*/

// STACK_PUSH(...) takes 1 or more double args, and pushes those values onto
// the stack (or errors on overflow), in order (so that the last argument will
// be the new top of the stack).
#define STACK_PUSH(...) do { \
    if (vm->stack_size + AZ_COUNT_ARGS(__VA_ARGS__) <= \
        AZ_ARRAY_SIZE(vm->stack)) { \
      if (!do_stack_push(vm, AZ_COUNT_ARGS(__VA_ARGS__), __VA_ARGS__)) { \
        SCRIPT_ERROR("non-finite result"); \
      } \
    } else SCRIPT_ERROR("stack overflow"); \
  } while (0)

static bool do_stack_push(az_script_vm_t *vm, int num_args, ...) {
  assert(vm->stack_size + num_args <= AZ_ARRAY_SIZE(vm->stack));
  bool finite = true;
  va_list args;
  va_start(args, num_args);
  for (int i = 0; i < num_args; ++i) {
    assert(vm->stack_size + i < AZ_ARRAY_SIZE(vm->stack));
    const double value = va_arg(args, double);
    if (!isfinite(value)) finite = false;
    vm->stack[vm->stack_size + i] = value;
  }
  va_end(args);
  vm->stack_size += num_args;
  assert(vm->stack_size <= AZ_ARRAY_SIZE(vm->stack));
  return finite;
}

/*===========================================================================*/

// STACK_POP(...) takes 1 or more double* args; it pops that many values off
// the stack (or errors on underflow), and assigns them to the pointers.  The
// top of the stack will be stored to the rightmost pointer passed, and so on.
#define STACK_POP(...) do { \
    if (vm->stack_size >= AZ_COUNT_ARGS(__VA_ARGS__)) { \
      do_stack_pop(vm, AZ_COUNT_ARGS(__VA_ARGS__), __VA_ARGS__); \
    } else SCRIPT_ERROR("stack underflow"); \
  } while (0)

static void do_stack_pop(az_script_vm_t *vm, int num_args, ...) {
  assert(vm->stack_size >= num_args);
  vm->stack_size -= num_args;
  va_list args;
  va_start(args, num_args);
  for (int i = 0; i < num_args; ++i) {
    double *ptr = va_arg(args, double *);
    *ptr = vm->stack[vm->stack_size + i];
  }
  va_end(args);
}

/*===========================================================================*/

#define DO_JUMP() do { \
    const int new_pc = vm->pc + (int)ins.immediate; \
    if (new_pc < 0 || new_pc > vm->script->num_instructions) { \
      SCRIPT_ERROR("jump out of range"); \
    } \
    vm->pc = new_pc - 1; \
  } while (0)

static void do_suspend(az_script_vm_t *vm, az_script_vm_t *target_vm) {
  assert(vm != NULL);
  assert(target_vm != NULL);
  assert(vm != target_vm);
  assert(vm->script != NULL);
  assert(target_vm->script == NULL);
  *target_vm = *vm;
  ++target_vm->pc;
  vm->script = NULL;
}

#define SUSPEND(target_vm) do { \
    do_suspend(vm, (target_vm)); \
    return; \
  } while (0)

/*===========================================================================*/

#define UNARY_OP(expr) do { \
    double a; \
    STACK_POP(&a); \
    const double result = (expr); \
    STACK_PUSH(result); \
  } while (0)

#define BINARY_OP(expr) do { \
    double a, b; \
    STACK_POP(&a, &b); \
    const double result = (expr); \
    STACK_PUSH(result); \
  } while (0)

static double modulo(double a, double b) {
  // If b is zero, return NaN (which will trigger an error).
  return (b == 0.0 ? NAN : az_signmod(a, b, b));
}

/*===========================================================================*/

#define GET_UUID(uuid_out) do { \
    const int slot = (int)ins.immediate; \
    if (slot < 0 || slot > AZ_NUM_UUID_SLOTS) { \
      SCRIPT_ERROR("invalid uuid index"); \
    } \
    *(uuid_out) = (slot == 0 ? AZ_SHIP_UUID : state->uuids[slot - 1]); \
  } while (0)

#define GET_UID(uuid_type, uid_out) do { \
    az_uuid_t uuid; \
    GET_UUID(&uuid); \
    if (uuid.type != (uuid_type)) { \
      SCRIPT_ERROR("invalid uuid type"); \
    } \
    *(uid_out) = uuid.uid; \
  } while (0)

#define GET_OBJECT(object_out) do { \
    az_uuid_t uuid; \
    GET_UUID(&uuid); \
    az_lookup_object(state, uuid, (object_out)); \
  } while (0)

static void set_object_state(az_object_t *object, double value) {
  switch (object->type) {
    case AZ_OBJ_NOTHING: break;
    case AZ_OBJ_BADDIE:
      assert(object->obj.baddie->kind != AZ_BAD_NOTHING);
      object->obj.baddie->state = (int)value;
      break;
    case AZ_OBJ_DOOR: {
      az_door_t *door = object->obj.door;
      assert(door->kind != AZ_DOOR_NOTHING);
      if (door->kind != AZ_DOOR_PASSAGE &&
          door->kind != AZ_DOOR_ALWAYS_OPEN &&
          door->kind != AZ_DOOR_BOSS) {
        door->is_open = (value != 0.0);
        door->openness = (value != 0.0 ? 1.0 : 0.0);
      }
    } break;
    case AZ_OBJ_GRAVFIELD:
      assert(object->obj.gravfield->kind != AZ_GRAV_NOTHING);
      object->obj.gravfield->script_fired = (value != 0.0);
      break;
    case AZ_OBJ_NODE: {
      az_node_t *node = object->obj.node;
      assert(node->kind != AZ_NODE_NOTHING);
      if (node->kind == AZ_NODE_MARKER) {
        node->subkind.marker = (int)value;
      } else if (node->kind == AZ_NODE_DOODAD_FG ||
                 node->kind == AZ_NODE_DOODAD_BG) {
        if (node->subkind.doodad == AZ_DOOD_DRILL_SHAFT_STILL) {
          if (value) node->subkind.doodad = AZ_DOOD_DRILL_SHAFT_SPIN;
        } else if (node->subkind.doodad == AZ_DOOD_DRILL_SHAFT_SPIN) {
          if (!value) node->subkind.doodad = AZ_DOOD_DRILL_SHAFT_STILL;
        } else if (node->subkind.doodad == AZ_DOOD_RED_TUBE_WINDOW) {
          if (value) node->subkind.doodad = AZ_DOOD_RED_TUBE_BROKEN_WINDOW;
        } else if (node->subkind.doodad == AZ_DOOD_RED_TUBE_BROKEN_WINDOW) {
          if (!value) node->subkind.doodad = AZ_DOOD_RED_TUBE_WINDOW;
        }
      }
    } break;
    case AZ_OBJ_SHIP: break;
    case AZ_OBJ_WALL: break;
  }
}

static void disable_skips(az_space_state_t *state) {
  state->skip.cooldown = 0.0;
  state->skip.allowed = false;
  state->skip.active = false;
}

/*===========================================================================*/

static void run_vm(az_space_state_t *state, az_script_vm_t *vm) {
  assert(vm != NULL);
  assert(vm->script != NULL);
  assert(vm->script->instructions != NULL);
  assert(state != NULL);
  assert(state->sync_vm.script == NULL);
  int total_steps = 0;
  while (vm->pc < vm->script->num_instructions) {
    if (++total_steps > AZ_MAX_SCRIPT_STEPS) SCRIPT_ERROR("ran for too long");
    const az_instruction_t ins = vm->script->instructions[vm->pc];
    switch (ins.opcode) {
      case AZ_OP_NOP: break;
      // Stack manipulation:
      case AZ_OP_PUSH:
        STACK_PUSH(ins.immediate);
        break;
      case AZ_OP_POP: {
        const int num = az_imax(1, (int)ins.immediate);
        if (vm->stack_size < num) SCRIPT_ERROR("stack underflow");
        vm->stack_size -= num;
      } break;
      case AZ_OP_DUP: {
        const int num = az_imax(1, (int)ins.immediate);
        if (vm->stack_size < num) SCRIPT_ERROR("stack underflow");
        if (vm->stack_size + num > AZ_ARRAY_SIZE(vm->stack)) {
          SCRIPT_ERROR("stack overflow");
        }
        for (int i = 0; i < num; ++i) {
          vm->stack[vm->stack_size + i] =
            vm->stack[vm->stack_size - num + i];
        }
        vm->stack_size += num;
      } break;
      case AZ_OP_SWAP: {
        const int size = vm->stack_size;
        int cycle = (int)ins.immediate;
        if (cycle == 0) cycle = 2;
        if (cycle < 0) {
          if (cycle < -size) SCRIPT_ERROR("stack underflow");
          const double temp = vm->stack[size - 1];
          for (int i = 1; i < -cycle; ++i) {
            vm->stack[size - i] = vm->stack[size - (i + 1)];
          }
          vm->stack[size + cycle] = temp;
        } else {
          if (cycle > size) SCRIPT_ERROR("stack underflow");
          const double temp = vm->stack[size - cycle];
          for (int i = cycle - 1; i >= 1; --i) {
            vm->stack[size - (i + 1)] = vm->stack[size - i];
          }
          vm->stack[size - 1] = temp;
        }
      } break;
      // Arithmetic:
      case AZ_OP_ADD: BINARY_OP(a + b); break;
      case AZ_OP_ADDI: UNARY_OP(a + ins.immediate); break;
      case AZ_OP_SUB: BINARY_OP(a - b); break;
      case AZ_OP_SUBI: UNARY_OP(a - ins.immediate); break;
      case AZ_OP_ISUB: UNARY_OP(ins.immediate - a); break;
      case AZ_OP_MUL: BINARY_OP(a * b); break;
      case AZ_OP_MULI: UNARY_OP(a * ins.immediate); break;
      case AZ_OP_DIV: BINARY_OP(a / b); break;
      case AZ_OP_DIVI: UNARY_OP(a / ins.immediate); break;
      case AZ_OP_IDIV: UNARY_OP(ins.immediate / a); break;
      case AZ_OP_MOD: BINARY_OP(modulo(a, b)); break;
      case AZ_OP_MODI: UNARY_OP(modulo(a, ins.immediate)); break;
      case AZ_OP_MIN: BINARY_OP(fmin(a, b)); break;
      case AZ_OP_MINI: UNARY_OP(fmin(a, ins.immediate)); break;
      case AZ_OP_MAX: BINARY_OP(fmax(a, b)); break;
      case AZ_OP_MAXI: UNARY_OP(fmax(a, ins.immediate)); break;
      // Math:
      case AZ_OP_ABS: UNARY_OP(fabs(a)); break;
      case AZ_OP_MTAU: UNARY_OP(az_mod2pi(a)); break;
      case AZ_OP_RAND: STACK_PUSH(az_random(0.0, 1.0)); break;
      case AZ_OP_SQRT: UNARY_OP(a < 0.0 ? NAN : sqrt(a)); break;
      // Vectors:
      case AZ_OP_VADD: {
        double xa, ya, xb, yb;
        STACK_POP(&xa, &ya, &xb, &yb);
        STACK_PUSH(xa + xb, ya + yb);
      } break;
      case AZ_OP_VSUB: {
        double xa, ya, xb, yb;
        STACK_POP(&xa, &ya, &xb, &yb);
        STACK_PUSH(xa - xb, ya - yb);
      } break;
      case AZ_OP_VMUL: {
        double x, y, f;
        STACK_POP(&x, &y, &f);
        STACK_PUSH(x * f, y * f);
      } break;
      case AZ_OP_VMULI: {
        double x, y;
        STACK_POP(&x, &y);
        STACK_PUSH(x * ins.immediate, y * ins.immediate);
      } break;
      case AZ_OP_VNORM: BINARY_OP(hypot(a, b)); break;
      case AZ_OP_VTHETA: BINARY_OP(atan2(b, a)); break;
      case AZ_OP_VPOLAR: {
        double magnitude, theta;
        STACK_POP(&magnitude, &theta);
        const az_vector_t vec = az_vpolar(magnitude, theta);
        STACK_PUSH(vec.x, vec.y);
      } break;
      // Comparisons:
      case AZ_OP_EQ: BINARY_OP(a == b ? 1.0 : 0.0); break;
      case AZ_OP_EQI: UNARY_OP(a == ins.immediate ? 1.0 : 0.0); break;
      case AZ_OP_NE: BINARY_OP(a != b ? 1.0 : 0.0); break;
      case AZ_OP_NEI: UNARY_OP(a != ins.immediate ? 1.0 : 0.0); break;
      case AZ_OP_LT: BINARY_OP(a < b ? 1.0 : 0.0); break;
      case AZ_OP_LTI: UNARY_OP(a < ins.immediate ? 1.0 : 0.0); break;
      case AZ_OP_GT: BINARY_OP(a > b ? 1.0 : 0.0); break;
      case AZ_OP_GTI: UNARY_OP(a > ins.immediate ? 1.0 : 0.0); break;
      case AZ_OP_LE: BINARY_OP(a <= b ? 1.0 : 0.0); break;
      case AZ_OP_LEI: UNARY_OP(a <= ins.immediate ? 1.0 : 0.0); break;
      case AZ_OP_GE: BINARY_OP(a >= b ? 1.0 : 0.0); break;
      case AZ_OP_GEI: UNARY_OP(a >= ins.immediate ? 1.0 : 0.0); break;
      // Flags:
      case AZ_OP_TEST: {
        const int flag_index = (int)ins.immediate;
        if (flag_index < 0 || flag_index >= AZ_MAX_NUM_FLAGS) {
          SCRIPT_ERROR("invalid flag index");
        }
        STACK_PUSH(az_test_flag(&state->ship.player, (az_flag_t)flag_index) ?
                   1.0 : 0.0);
      } break;
      case AZ_OP_SET: {
        const int flag_index = (int)ins.immediate;
        if (flag_index < 0 || flag_index >= AZ_MAX_NUM_FLAGS) {
          SCRIPT_ERROR("invalid flag index");
        }
        az_set_flag(&state->ship.player, (az_flag_t)flag_index);
      } break;
      case AZ_OP_CLR: {
        const int flag_index = (int)ins.immediate;
        if (flag_index < 0 || flag_index >= AZ_MAX_NUM_FLAGS) {
          SCRIPT_ERROR("invalid flag index");
        }
        az_clear_flag(&state->ship.player, (az_flag_t)flag_index);
      } break;
      case AZ_OP_HAS: {
        const int upgrade_index = (int)ins.immediate;
        if (upgrade_index < 0 || upgrade_index >= AZ_NUM_UPGRADES) {
          SCRIPT_ERROR("invalid upgrade index");
        }
        STACK_PUSH(az_has_upgrade(&state->ship.player,
                                  (az_upgrade_t)upgrade_index) ? 1.0 : 0.0);
      } break;
      case AZ_OP_MAP: {
        const int zone_index = (int)ins.immediate;
        if (zone_index < 0 || zone_index >= state->planet->num_zones) {
          SCRIPT_ERROR("invalid zone index");
        }
        az_set_zone_mapped(&state->ship.player, (az_zone_key_t)zone_index);
      } break;
      // Objects:
      case AZ_OP_NIX: {
        az_object_t object;
        GET_OBJECT(&object);
        switch (object.type) {
          case AZ_OBJ_NOTHING: break;
          case AZ_OBJ_BADDIE:
            object.obj.baddie->kind = AZ_BAD_NOTHING;
            break;
          case AZ_OBJ_DOOR:
            object.obj.door->kind = AZ_DOOR_NOTHING;
            break;
          case AZ_OBJ_GRAVFIELD:
            object.obj.gravfield->kind = AZ_GRAV_NOTHING;
            break;
          case AZ_OBJ_NODE:
            object.obj.node->kind = AZ_NODE_NOTHING;
            break;
          case AZ_OBJ_SHIP: SCRIPT_ERROR("invalid object type");
          case AZ_OBJ_WALL:
            object.obj.wall->kind = AZ_WALL_NOTHING;
            break;
        }
      } break;
      case AZ_OP_KILL: {
        az_object_t object;
        GET_OBJECT(&object);
        az_kill_object(state, &object);
      } break;
      case AZ_OP_GHEAL: {
        az_object_t object;
        GET_OBJECT(&object);
        switch (object.type) {
          case AZ_OBJ_NOTHING: STACK_PUSH(0.0); break;
          case AZ_OBJ_SHIP: STACK_PUSH(object.obj.ship->player.shields); break;
          case AZ_OBJ_BADDIE: STACK_PUSH(object.obj.baddie->health); break;
          default: SCRIPT_ERROR("invalid object type");
        }
      } break;
      case AZ_OP_SHEAL: {
        az_object_t object;
        GET_OBJECT(&object);
        double new_health;
        STACK_POP(&new_health);
        switch (object.type) {
          case AZ_OBJ_NOTHING: break;
          case AZ_OBJ_SHIP:
            if (new_health <= 0.0) az_kill_object(state, &object);
            else object.obj.ship->player.shields =
                   fmin(new_health, object.obj.ship->player.max_shields);
            break;
          case AZ_OBJ_BADDIE:
            if (new_health <= 0.0) az_kill_object(state, &object);
            else object.obj.baddie->health =
                   fmin(new_health, object.obj.baddie->data->max_health);
            break;
          default: SCRIPT_ERROR("invalid object type");
        }
      } break;
      case AZ_OP_GPOS: {
        az_object_t object;
        GET_OBJECT(&object);
        const az_vector_t position =
          (object.type == AZ_OBJ_NOTHING ? AZ_VZERO :
           az_get_object_position(&object));
        STACK_PUSH(position.x, position.y);
      } break;
      case AZ_OP_SPOS: {
        az_object_t object;
        GET_OBJECT(&object);
        az_vector_t new_position;
        STACK_POP(&new_position.x, &new_position.y);
        if (object.type != AZ_OBJ_NOTHING) {
          const az_vector_t old_position = az_get_object_position(&object);
          az_move_object(state, &object, az_vsub(new_position, old_position),
                         0.0);
        }
      } break;
      case AZ_OP_GANG: {
        az_object_t object;
        GET_OBJECT(&object);
        STACK_PUSH(object.type == AZ_OBJ_NOTHING ? 0.0 :
                   az_get_object_angle(&object));
      } break;
      case AZ_OP_SANG: {
        az_object_t object;
        GET_OBJECT(&object);
        double new_angle;
        STACK_POP(&new_angle);
        if (object.type != AZ_OBJ_NOTHING) {
          const double old_angle = az_get_object_angle(&object);
          az_move_object(state, &object, AZ_VZERO,
                         az_mod2pi(new_angle - old_angle));
        }
      } break;
      case AZ_OP_GSTAT: {
        az_object_t object;
        GET_OBJECT(&object);
        double value = 0.0;
        switch (object.type) {
          case AZ_OBJ_NOTHING: break;
          case AZ_OBJ_BADDIE:
            assert(object.obj.baddie->kind != AZ_BAD_NOTHING);
            value = (double)object.obj.baddie->state;
            break;
          case AZ_OBJ_DOOR:
            assert(object.obj.door->kind != AZ_DOOR_NOTHING);
            if (object.obj.door->is_open) value = 1.0;
            break;
          case AZ_OBJ_GRAVFIELD:
            assert(object.obj.gravfield->kind != AZ_GRAV_NOTHING);
            if (object.obj.gravfield->script_fired) value = 1.0;
            break;
          case AZ_OBJ_NODE: {
            az_node_t *node = object.obj.node;
            assert(node->kind != AZ_NODE_NOTHING);
            if (node->kind == AZ_NODE_MARKER) {
              value = (double)node->subkind.marker;
            } else if ((node->kind == AZ_NODE_DOODAD_FG ||
                        node->kind == AZ_NODE_DOODAD_BG) &&
                       (node->subkind.doodad == AZ_DOOD_DRILL_SHAFT_SPIN ||
                        node->subkind.doodad ==
                        AZ_DOOD_RED_TUBE_BROKEN_WINDOW)) {
              value = 1.0;
            }
          } break;
          case AZ_OBJ_SHIP: break;
          case AZ_OBJ_WALL: break;
        }
        STACK_PUSH(value);
      } break;
      case AZ_OP_SSTAT: {
        az_object_t object;
        GET_OBJECT(&object);
        double value;
        STACK_POP(&value);
        set_object_state(&object, value);
      } break;
      case AZ_OP_ACTIV: {
        az_object_t object;
        GET_OBJECT(&object);
        set_object_state(&object, 1);
      } break;
      case AZ_OP_DEACT: {
        az_object_t object;
        GET_OBJECT(&object);
        set_object_state(&object, 0);
      } break;
      // Ship:
      case AZ_OP_GVEL:
        STACK_PUSH(state->ship.velocity.x, state->ship.velocity.y);
        break;
      case AZ_OP_SVEL: {
        az_vector_t new_velocity;
        STACK_POP(&new_velocity.x, &new_velocity.y);
        state->ship.velocity = new_velocity;
      } break;
      case AZ_OP_AUTOP: {
        if (ins.immediate) {
          state->ship.autopilot.enabled = true;
          state->ship.autopilot.thrust = 0;
          state->ship.autopilot.cplus = false;
          state->ship.autopilot.goal_angle = state->ship.angle;
        } else {
          state->ship.autopilot.enabled = false;
        }
      } break;
      case AZ_OP_TURN: {
        double goal_angle;
        STACK_POP(&goal_angle);
        if (state->ship.autopilot.enabled) {
          state->ship.autopilot.goal_angle = az_mod2pi(goal_angle);
        }
      } break;
      case AZ_OP_THRUST: {
        if (state->ship.autopilot.enabled) {
          if (ins.immediate > 0) state->ship.autopilot.thrust = 1;
          else if (ins.immediate < 0) state->ship.autopilot.thrust = -1;
          else state->ship.autopilot.thrust = 0;
        }
      } break;
      case AZ_OP_CPLUS: {
        if (state->ship.autopilot.enabled) {
          state->ship.autopilot.cplus = (bool)ins.immediate;
        }
      } break;
      // Baddies:
      case AZ_OP_BAD: {
        const int slot = (int)ins.immediate;
        if (slot < 0 || slot > AZ_NUM_UUID_SLOTS) {
          SCRIPT_ERROR("invalid uuid index");
        }
        double k, x, y, angle;
        STACK_POP(&k, &x, &y, &angle);
        int kind = (int)k;
        if (kind < 1 || kind > AZ_NUM_BADDIE_KINDS) {
          SCRIPT_ERROR("invalid baddie kind");
        }
        const az_baddie_t *baddie = az_add_baddie(
            state, kind, (az_vector_t){x, y}, angle);
        if (slot > 0) {
          if (baddie != NULL) {
            state->uuids[slot - 1] = (az_uuid_t){
              .type = AZ_UUID_BADDIE, .uid = baddie->uid
            };
          } else state->uuids[slot - 1] = AZ_NULL_UUID;
        }
      } break;
      case AZ_OP_SBADK: {
        az_uid_t uid;
        GET_UID(AZ_UUID_BADDIE, &uid);
        double argument;
        STACK_POP(&argument);
        const int kind = (int)argument;
        if (kind < 1 || kind > AZ_NUM_BADDIE_KINDS) {
          SCRIPT_ERROR("invalid baddie kind");
        }
        az_baddie_t *baddie;
        if (az_lookup_baddie(state, uid, &baddie)) {
          const az_script_t *baddie_script = baddie->on_kill;
          az_init_baddie(baddie, (az_baddie_kind_t)kind, baddie->position,
                         baddie->angle);
          baddie->on_kill = baddie_script;
        }
      } break;
      case AZ_OP_BOSS: {
        az_uid_t uid;
        GET_UID(AZ_UUID_BADDIE, &uid);
        state->boss_uid = uid;
      } break;
      // Doors:
      case AZ_OP_OPEN: {
        az_uid_t uid;
        GET_UID(AZ_UUID_DOOR, &uid);
        az_door_t *door;
        if (az_lookup_door(state, uid, &door)) {
          if (door->kind == AZ_DOOR_PASSAGE ||
              door->kind == AZ_DOOR_ALWAYS_OPEN) {
            SCRIPT_ERROR("invalid door kind");
          }
          if (!door->is_open) {
            az_play_sound(&state->soundboard, AZ_SND_DOOR_OPEN);
            door->is_open = true;
          }
        }
      } break;
      case AZ_OP_CLOSE: {
        az_uid_t uid;
        GET_UID(AZ_UUID_DOOR, &uid);
        az_door_t *door;
        if (az_lookup_door(state, uid, &door)) {
          if (door->kind == AZ_DOOR_PASSAGE ||
              door->kind == AZ_DOOR_ALWAYS_OPEN) {
            SCRIPT_ERROR("invalid door kind");
          }
          if (door->is_open) {
            az_play_sound(&state->soundboard, AZ_SND_DOOR_CLOSE);
            door->is_open = false;
          }
        }
      } break;
      case AZ_OP_LOCK: {
        az_uid_t uid;
        GET_UID(AZ_UUID_DOOR, &uid);
        az_door_t *door;
        if (az_lookup_door(state, uid, &door)) {
          if (door->kind == AZ_DOOR_PASSAGE ||
              door->kind == AZ_DOOR_FORCEFIELD ||
              door->kind == AZ_DOOR_ALWAYS_OPEN) {
            SCRIPT_ERROR("invalid door kind");
          }
          door->kind = AZ_DOOR_LOCKED;
          if (door->is_open) {
            az_play_sound(&state->soundboard, AZ_SND_DOOR_CLOSE);
            door->is_open = false;
          }
        }
      } break;
      case AZ_OP_UNLOCK: {
        az_uid_t uid;
        GET_UID(AZ_UUID_DOOR, &uid);
        az_door_t *door;
        if (az_lookup_door(state, uid, &door)) {
          if (door->kind == AZ_DOOR_PASSAGE ||
              door->kind == AZ_DOOR_FORCEFIELD ||
              door->kind == AZ_DOOR_ALWAYS_OPEN) {
            SCRIPT_ERROR("invalid door kind");
          }
          door->kind = (door->kind == AZ_DOOR_LOCKED ||
                        door->kind == AZ_DOOR_BOSS ?
                        AZ_DOOR_UNLOCKED : AZ_DOOR_NORMAL);
        }
      } break;
      // Gravfields:
      case AZ_OP_GSTR: {
        az_uid_t uid;
        GET_UID(AZ_UUID_GRAVFIELD, &uid);
        az_gravfield_t *gravfield;
        STACK_PUSH(az_lookup_gravfield(state, uid, &gravfield) ?
                   gravfield->strength : 0.0);
      } break;
      case AZ_OP_SSTR: {
        double value;
        STACK_POP(&value);
        az_uid_t uid;
        GET_UID(AZ_UUID_GRAVFIELD, &uid);
        az_gravfield_t *gravfield;
        if (az_lookup_gravfield(state, uid, &gravfield)) {
          if (az_is_liquid(gravfield->kind)) {
            SCRIPT_ERROR("invalid gravfield kind");
          }
          gravfield->strength = value;
        }
      } break;
      // Camera:
      case AZ_OP_GCAM:
        STACK_PUSH(state->camera.center.x, state->camera.center.y);
        break;
      case AZ_OP_RCAM: {
        double value;
        STACK_POP(&value);
        state->camera.r_max_override = value;
      } break;
      case AZ_OP_DARK:
        state->dark_goal = fmin(fmax(0.0, ins.immediate), 1.0);
        break;
      case AZ_OP_DARKS: {
        double value;
        STACK_POP(&value);
        state->dark_goal = fmin(fmax(0.0, value), 1.0);
      } break;
      case AZ_OP_BLINK:
        state->darkness = fmin(fmax(0.0, ins.immediate), 1.0);
        break;
      case AZ_OP_SHAKE:
        az_shake_camera(&state->camera, ins.immediate, ins.immediate * 0.75);
        break;
      case AZ_OP_QUAKE:
        state->camera.quake_vert = fmax(0, ins.immediate);
        break;
      // Pyrotechnics:
      case AZ_OP_BOOM: {
        az_vector_t position;
        STACK_POP(&position.x, &position.y);
        az_add_projectile(state, AZ_PROJ_NUCLEAR_EXPLOSION, position,
                          0.0, 1.0, AZ_NULL_UID);
      } break;
      case AZ_OP_NUKE: {
        state->nuke.active = true;
        state->nuke.rho =
          fmax(az_vnorm(state->camera.center) - AZ_SCREEN_HEIGHT, 0.0);
      } break;
      case AZ_OP_BOLT: {
        az_vector_t p1, p2;
        STACK_POP(&p1.x, &p1.y, &p2.x, &p2.y);
        az_particle_t *particle;
        if (az_insert_particle(state, &particle)) {
          particle->kind = AZ_PAR_LIGHTNING_BOLT;
          particle->color = (az_color_t){128, 64, 255, 255};
          particle->position = p2;
          particle->velocity = AZ_VZERO;
          particle->angle = az_vtheta(az_vsub(p1, p2));
          particle->lifetime = ins.immediate;
          particle->param1 = az_vdist(p1, p2);
          particle->param2 = 0.66;
        }
        az_play_sound(&state->soundboard, AZ_SND_ELECTRICITY);
      } break;
      case AZ_OP_NPS: {
        az_vector_t position;
        STACK_POP(&position.x, &position.y);
        state->camera.wobble_goal = 0.5 * ins.immediate;
        az_particle_t *particle;
        if (az_insert_particle(state, &particle)) {
          particle->kind = AZ_PAR_NPS_PORTAL;
          particle->color = (az_color_t){128, 64, 255, 255};
          particle->position = position;
          particle->velocity = AZ_VZERO;
          particle->angle = 0.0;
          particle->lifetime = ins.immediate;
          particle->param1 = 50.0 * sqrt(ins.immediate);
        }
        az_play_sound(&state->soundboard, AZ_SND_NPS_PORTAL);
      } break;
      // Cutscenes:
      case AZ_OP_FADO:
        assert(state->global_fade.step == AZ_GFS_INACTIVE);
        if (state->skip.active) break;
        state->global_fade.step = AZ_GFS_FADE_OUT;
        state->global_fade.fade_alpha = 0.0;
        state->global_fade.fade_gray = 0.0f;
        SUSPEND(&state->sync_vm);
      case AZ_OP_FADI:
        assert(state->global_fade.step == AZ_GFS_INACTIVE);
        if (state->skip.active) break;
        state->global_fade.step = AZ_GFS_FADE_IN;
        state->global_fade.fade_alpha = 1.0;
        state->global_fade.fade_gray = 0.0f;
        state->dialogue.hidden = false;
        SUSPEND(&state->sync_vm);
      case AZ_OP_FLASH:
        assert(state->global_fade.step == AZ_GFS_INACTIVE);
        if (state->skip.active) break;
        state->global_fade.step = AZ_GFS_FADE_IN;
        state->global_fade.fade_alpha = 1.0;
        state->global_fade.fade_gray = 1.0f;
        SUSPEND(&state->sync_vm);
      case AZ_OP_SCENE: {
        const int scene_index = (int)ins.immediate;
        if (scene_index < 0 || scene_index > AZ_NUM_SCENES) {
          SCRIPT_ERROR("invalid scene index");
        }
        if (state->skip.active && scene_index != (int)AZ_SCENE_NOTHING) break;
        state->cutscene.next = (az_scene_t)scene_index;
        state->cutscene.next_text = NULL;
        if (state->cutscene.scene == AZ_SCENE_NOTHING) {
          state->cutscene.scene = state->cutscene.next;
          state->cutscene.scene_text = state->cutscene.next_text;
          state->cutscene.fade_alpha = 1.0;
        } else SUSPEND(&state->sync_vm);
      } break;
      case AZ_OP_SCTXT: {
        const int paragraph_index = (int)ins.immediate;
        if (paragraph_index < 0 ||
            paragraph_index >= state->planet->num_paragraphs) {
          SCRIPT_ERROR("invalid paragraph index");
        }
        if (state->skip.active) break;
        state->cutscene.next = AZ_SCENE_TEXT;
        state->cutscene.next_text = state->planet->paragraphs[paragraph_index];
        if (state->cutscene.scene == AZ_SCENE_NOTHING) {
          state->cutscene.scene = state->cutscene.next;
          state->cutscene.scene_text = state->cutscene.next_text;
          state->cutscene.fade_alpha = 1.0;
        } else SUSPEND(&state->sync_vm);
      } break;
      case AZ_OP_SKIP: {
        if (ins.immediate) state->skip.allowed = true;
        else disable_skips(state);
      } break;
      // Messages/dialog:
      case AZ_OP_MSG: {
        const int paragraph_index = (int)ins.immediate;
        if (paragraph_index < 0 ||
            paragraph_index >= state->planet->num_paragraphs) {
          SCRIPT_ERROR("invalid paragraph index");
        }
        az_set_message(state, state->planet->paragraphs[paragraph_index]);
      } break;
      case AZ_OP_DLOG:
        if (state->skip.active) break;
        if (state->mode == AZ_MODE_NORMAL &&
            state->dialogue.step == AZ_DLS_INACTIVE &&
            state->monologue.step == AZ_MLS_INACTIVE) {
          state->dialogue = (az_dialogue_state_t){ .step = AZ_DLS_BEGIN };
          SUSPEND(&state->sync_vm);
        }
        SCRIPT_ERROR("can't start dialogue now");
      case AZ_OP_PT:
        if (state->skip.active) break;
        if (state->dialogue.step != AZ_DLS_INACTIVE) {
          const int portrait = (int)ins.immediate;
          if (portrait < 0 || portrait > AZ_NUM_PORTRAITS) {
            SCRIPT_ERROR("invalid portrait");
          } else {
            state->dialogue.top = (az_portrait_t)portrait;
          }
        } else SCRIPT_ERROR("can't PT when not in dialogue");
        break;
      case AZ_OP_PB:
        if (state->skip.active) break;
        if (state->dialogue.step != AZ_DLS_INACTIVE) {
          const int portrait = (int)ins.immediate;
          if (portrait < 0 || portrait > AZ_NUM_PORTRAITS) {
            SCRIPT_ERROR("invalid portrait");
          } else {
            state->dialogue.bottom = (az_portrait_t)portrait;
          }
        } else SCRIPT_ERROR("can't PB when not in dialogue");
        break;
      case AZ_OP_TT:
        if (state->skip.active) break;
        if (state->dialogue.step == AZ_DLS_INACTIVE) {
          SCRIPT_ERROR("can't TT when not in dialogue");
        } else if (state->monologue.step != AZ_MLS_INACTIVE) {
          SCRIPT_ERROR("can't TT during monologue");
        } else {
          const int paragraph_index = (int)ins.immediate;
          if (paragraph_index < 0 ||
              paragraph_index >= state->planet->num_paragraphs) {
            SCRIPT_ERROR("invalid paragraph index");
          }
          const char *paragraph = state->planet->paragraphs[paragraph_index];
          az_set_dialogue_text(&state->dialogue, state->prefs,
                               paragraph, true);
        }
        SUSPEND(&state->sync_vm);
      case AZ_OP_TB:
        if (state->skip.active) break;
        if (state->dialogue.step == AZ_DLS_INACTIVE) {
          SCRIPT_ERROR("can't TB when not in dialogue");
        } else if (state->monologue.step != AZ_MLS_INACTIVE) {
          SCRIPT_ERROR("can't TB during monologue");
        } else {
          const int paragraph_index = (int)ins.immediate;
          if (paragraph_index < 0 ||
              paragraph_index >= state->planet->num_paragraphs) {
            SCRIPT_ERROR("invalid paragraph index");
          }
          const char *paragraph = state->planet->paragraphs[paragraph_index];
          az_set_dialogue_text(&state->dialogue, state->prefs,
                               paragraph, false);
        }
        SUSPEND(&state->sync_vm);
      case AZ_OP_DEND:
        if (state->dialogue.step == AZ_DLS_INACTIVE) {
          if (state->skip.active) break;
          SCRIPT_ERROR("can't DEND when not in dialogue");
        } else if (state->monologue.step != AZ_MLS_INACTIVE) {
          if (state->skip.active) break;
          SCRIPT_ERROR("can't DEND during monologue");
        }
        state->dialogue = (az_dialogue_state_t){ .step = AZ_DLS_END };
        SUSPEND(&state->sync_vm);
      case AZ_OP_MLOG:
        if (state->skip.active) break;
        if (state->monologue.step == AZ_MLS_INACTIVE) {
          state->monologue = (az_monologue_state_t){ .step = AZ_MLS_BEGIN };
          SUSPEND(&state->sync_vm);
        }
        SCRIPT_ERROR("can't MLOG while already in monologue");
      case AZ_OP_TM:
        if (state->skip.active) break;
        if (state->monologue.step != AZ_MLS_INACTIVE) {
          const int paragraph_index = (int)ins.immediate;
          if (paragraph_index < 0 ||
              paragraph_index >= state->planet->num_paragraphs) {
            SCRIPT_ERROR("invalid paragraph index");
          }
          const char *paragraph = state->planet->paragraphs[paragraph_index];
          state->monologue.step = AZ_MLS_TALK;
          state->monologue.progress = 0.0;
          state->monologue.paragraph = paragraph;
          state->monologue.paragraph_length =
            az_paragraph_total_length(state->prefs, paragraph);
          state->monologue.chars_to_print = 0;
          SUSPEND(&state->sync_vm);
        }
        SCRIPT_ERROR("can't TM when not in monologue");
      case AZ_OP_MEND:
        if (state->monologue.step != AZ_MLS_INACTIVE) {
          state->monologue.step = AZ_MLS_END;
          state->monologue.progress = 0.0;
          state->monologue.paragraph = NULL;
          state->monologue.paragraph_length = 0;
          state->monologue.chars_to_print = 0;
          SUSPEND(&state->sync_vm);
        }
        if (state->skip.active) break;
        SCRIPT_ERROR("can't MEND when not in monologue");
      // Music/sound:
      case AZ_OP_MUS: {
        const int music_index = (int)ins.immediate;
        if (music_index < 0 || music_index > AZ_NUM_MUSIC_KEYS) {
          SCRIPT_ERROR("invalid music index");
        }
        az_change_music(&state->soundboard, (az_music_key_t)music_index);
      } break;
      case AZ_OP_MUSF:
        az_change_music_flag(&state->soundboard, (int)ins.immediate);
        break;
      case AZ_OP_SND: {
        const int sound_index = (int)ins.immediate;
        if (sound_index < 0 || sound_index > AZ_NUM_SOUND_KEYS) {
          SCRIPT_ERROR("invalid sound index");
        }
        if (state->skip.active) break;
        az_play_sound(&state->soundboard, (az_sound_key_t)sound_index);
      } break;
      // Timers:
      case AZ_OP_WAIT:
      case AZ_OP_WAITS: {
        double wait_duration;
        if (ins.opcode == AZ_OP_WAITS) {
          STACK_POP(&wait_duration);
        } else wait_duration = ins.immediate;
        if (state->skip.active) break;
        if (wait_duration > 0.0) {
          if (state->cutscene.scene != AZ_SCENE_NOTHING ||
              state->dialogue.step != AZ_DLS_INACTIVE ||
              state->monologue.step != AZ_MLS_INACTIVE) {
            state->sync_timer.is_active = true;
            state->sync_timer.time_remaining = wait_duration;
            if (state->dialogue.step != AZ_DLS_INACTIVE) {
              state->dialogue.step = AZ_DLS_BLANK;
              state->dialogue.paragraph = NULL;
              state->dialogue.paragraph_length = 0;
              state->dialogue.chars_to_print = 0;
            }
            if (state->monologue.step != AZ_MLS_INACTIVE) {
              state->monologue.step = AZ_MLS_BLANK;
              state->monologue.paragraph = NULL;
              state->monologue.paragraph_length = 0;
              state->monologue.chars_to_print = 0;
            }
            SUSPEND(&state->sync_vm);
          }
          AZ_ARRAY_LOOP(timer, state->timers) {
            if (timer->vm.script != NULL) continue;
            timer->time_remaining = wait_duration;
            disable_skips(state);
            SUSPEND(&timer->vm);
          }
          SCRIPT_ERROR("too many timers");
        }
      } break;
      case AZ_OP_DOOM:
        if (state->dialogue.step != AZ_DLS_INACTIVE) {
          SCRIPT_ERROR("can't DOOM during dialogue");
        }
        state->countdown.is_active = true;
        state->countdown.active_for = 0.0;
        state->countdown.time_remaining = fmax(0.0, ins.immediate);
        disable_skips(state);
        SUSPEND(&state->countdown.vm);
        break;
      case AZ_OP_SAFE:
        state->countdown.is_active = false;
        state->countdown.vm.script = NULL;
        break;
      // Control flow:
      case AZ_OP_JUMP:
        DO_JUMP();
        break;
      case AZ_OP_BEQZ: {
        double p;
        STACK_POP(&p);
        if (p == 0) DO_JUMP();
      } break;
      case AZ_OP_BNEZ: {
        double p;
        STACK_POP(&p);
        if (p != 0) DO_JUMP();
      } break;
      case AZ_OP_HALT: goto halt;
      case AZ_OP_HEQZ: {
        double p;
        STACK_POP(&p);
        if (p == 0) goto halt;
      } break;
      case AZ_OP_HNEZ: {
        double p;
        STACK_POP(&p);
        if (p != 0) goto halt;
      } break;
      case AZ_OP_VICT:
        state->victory = true;
        goto halt;
      case AZ_OP_ERROR: SCRIPT_ERROR("ERROR opcode");
    }
    ++vm->pc;
    assert(vm->pc >= 0);
    assert(vm->pc <= vm->script->num_instructions);
  }

 halt:
  disable_skips(state);
  state->cutscene.next = AZ_SCENE_NOTHING;
  state->cutscene.next_text = NULL;
  if (state->dialogue.step != AZ_DLS_INACTIVE) {
    state->dialogue = (az_dialogue_state_t){ .step = AZ_DLS_END };
  }
  if (state->monologue.step != AZ_MLS_INACTIVE) {
    state->monologue = (az_monologue_state_t){ .step = AZ_MLS_END };
  }
}

void az_run_script(az_space_state_t *state, const az_script_t *script) {
  if (script == NULL || script->num_instructions == 0) return;
  az_script_vm_t vm = { .script = script };
  run_vm(state, &vm);
}

void az_resume_script(az_space_state_t *state, az_script_vm_t *vm) {
  assert(vm != NULL);
  assert(vm->script != NULL);
  az_script_vm_t local_vm = *vm;
  vm->script = NULL;
  run_vm(state, &local_vm);
}

/*===========================================================================*/

void az_tick_timers(az_space_state_t *state, double time) {
  AZ_ARRAY_LOOP(timer, state->timers) {
    if (timer->vm.script == NULL) continue;
    assert(timer->time_remaining >= 0.0);
    timer->time_remaining -= time;
    if (timer->time_remaining <= 0.0) {
      timer->time_remaining = 0.0;
      if (state->sync_vm.script == NULL) {
        az_resume_script(state, &timer->vm);
      }
    }
  }
}

/*===========================================================================*/
