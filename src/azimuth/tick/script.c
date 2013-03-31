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

#include "azimuth/state/script.h"
#include "azimuth/state/space.h"
#include "azimuth/tick/baddie.h" // for az_kill_baddie
#include "azimuth/util/audio.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

// How many instructions a script is allowed to execute before we terminate it
// with an error (to avoid infinite loops).
#define AZ_MAX_SCRIPT_STEPS 100

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

#define GET_UUID(uuid_out) do { \
    const int slot = (int)ins.immediate; \
    if (slot < 1 || slot > AZ_NUM_UUID_SLOTS) { \
      SCRIPT_ERROR("invalid uuid index"); \
    } \
    *(uuid_out) = state->uuids[slot - 1]; \
  } while (0)

#define GET_UID(uuid_type, uid_out) do { \
    az_uuid_t uuid; \
    GET_UUID(&uuid); \
    if (uuid.type != (uuid_type)) { \
      SCRIPT_ERROR("invalid uuid type"); \
    } \
    *(uid_out) = uuid.uid; \
  } while (0)

static void do_suspend(az_script_vm_t *vm, const az_script_t *script,
                       az_script_vm_t *target_vm) {
  assert(script != NULL);
  assert(target_vm != NULL);
  ++vm->pc;
  vm->script = NULL;
  memmove(target_vm, vm, sizeof(az_script_vm_t));
  target_vm->script = script;
}

#define SUSPEND(script, target_vm) do { \
    do_suspend(vm, (script), (target_vm)); \
    return; \
  } while (0)

/*===========================================================================*/

typedef enum {
  AZ_OBJ_NOTHING = 0,
  AZ_OBJ_BADDIE,
  AZ_OBJ_DOOR,
  AZ_OBJ_GRAVFIELD,
  AZ_OBJ_NODE,
  AZ_OBJ_WALL
} az_object_type_t;

typedef struct {
  az_object_type_t type;
  union {
    az_baddie_t *baddie;
    az_door_t *door;
    az_gravfield_t *gravfield;
    az_node_t *node;
    az_wall_t *wall;
  } obj;
} az_object_t;

#define GET_OBJECT(object_out) do { \
    az_uuid_t uuid; \
    GET_UUID(&uuid); \
    if (uuid.type == AZ_UUID_NOTHING) SCRIPT_ERROR("invalid uuid type"); \
    else lookup_object(state, uuid, (object_out)); \
  } while (0)

static void lookup_object(az_space_state_t *state, az_uuid_t uuid,
                          az_object_t *object) {
  assert(uuid.type != AZ_UUID_NOTHING);
  object->type = AZ_OBJ_NOTHING;
  switch (uuid.type) {
    case AZ_UUID_NOTHING: AZ_ASSERT_UNREACHABLE();
    case AZ_UUID_BADDIE:
      if (az_lookup_baddie(state, uuid.uid, &object->obj.baddie)) {
        object->type = AZ_OBJ_BADDIE;
      }
      break;
    case AZ_UUID_DOOR:
      if (az_lookup_door(state, uuid.uid, &object->obj.door)) {
        object->type = AZ_OBJ_DOOR;
      }
      break;
    case AZ_UUID_GRAVFIELD:
      if (az_lookup_gravfield(state, uuid.uid, &object->obj.gravfield)) {
        object->type = AZ_OBJ_GRAVFIELD;
      }
      break;
    case AZ_UUID_NODE:
      if (az_lookup_node(state, uuid.uid, &object->obj.node)) {
        object->type = AZ_OBJ_NODE;
      }
      break;
    case AZ_UUID_WALL:
      if (az_lookup_wall(state, uuid.uid, &object->obj.wall)) {
        object->type = AZ_OBJ_WALL;
      }
      break;
  }
}

/*===========================================================================*/

void az_run_script(az_space_state_t *state, const az_script_t *script) {
  if (script == NULL || script->num_instructions == 0) return;
  az_script_vm_t vm = { .script = script };
  az_resume_script(state, &vm);
}

void az_resume_script(az_space_state_t *state, az_script_vm_t *vm) {
  assert(state != NULL);
  assert(vm != NULL);
  assert(vm->script != NULL);
  assert(vm->script->instructions != NULL);
  int total_steps = 0;
  while (vm->pc < vm->script->num_instructions) {
    if (++total_steps > AZ_MAX_SCRIPT_STEPS) SCRIPT_ERROR("ran for too long");
    az_instruction_t ins = vm->script->instructions[vm->pc];
    switch (ins.opcode) {
      case AZ_OP_NOP: break;
      // Stack manipulation:
      case AZ_OP_PUSH:
        STACK_PUSH(ins.immediate);
        break;
      case AZ_OP_POP:
        {
          const int num = az_imax(1, (int)ins.immediate);
          if (vm->stack_size < num) SCRIPT_ERROR("stack underflow");
          vm->stack_size -= num;
        }
        break;
      case AZ_OP_DUP:
        {
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
        }
        break;
      // Arithmetic:
      case AZ_OP_ADD:
        {
          double a, b;
          STACK_POP(&a, &b);
          STACK_PUSH(a + b);
        }
        break;
      case AZ_OP_ADDI:
        {
          double a;
          STACK_POP(&a);
          STACK_PUSH(a + ins.immediate);
        }
        break;
      case AZ_OP_SUB:
        {
          double a, b;
          STACK_POP(&a, &b);
          STACK_PUSH(a - b);
        }
        break;
      case AZ_OP_SUBI:
        {
          double a;
          STACK_POP(&a);
          STACK_PUSH(a - ins.immediate);
        }
        break;
      case AZ_OP_ISUB:
        {
          double a;
          STACK_POP(&a);
          STACK_PUSH(ins.immediate - a);
        }
        break;
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
        {
          const int flag_index = (int)ins.immediate;
          if (flag_index < 0 || flag_index >= AZ_MAX_NUM_FLAGS) {
            SCRIPT_ERROR("invalid flag index");
          }
          STACK_PUSH(az_test_flag(&state->ship.player, (az_flag_t)flag_index) ?
                     1.0 : 0.0);
        }
        break;
      case AZ_OP_SET:
        {
          const int flag_index = (int)ins.immediate;
          if (flag_index < 0 || flag_index >= AZ_MAX_NUM_FLAGS) {
            SCRIPT_ERROR("invalid flag index");
          }
          az_set_flag(&state->ship.player, (az_flag_t)flag_index);
        }
        break;
      case AZ_OP_CLR:
        {
          const int flag_index = (int)ins.immediate;
          if (flag_index < 0 || flag_index >= AZ_MAX_NUM_FLAGS) {
            SCRIPT_ERROR("invalid flag index");
          }
          az_clear_flag(&state->ship.player, (az_flag_t)flag_index);
        }
        break;
      case AZ_OP_MAP:
        {
          const int zone_index = (int)ins.immediate;
          if (zone_index < 0 || zone_index >= state->planet->num_zones) {
            SCRIPT_ERROR("invalid zone index");
          }
          az_set_zone_mapped(&state->ship.player, (az_zone_key_t)zone_index);
        }
        break;
      // Objects:
      case AZ_OP_NIX:
        {
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
            case AZ_OBJ_WALL:
              object.obj.wall->kind = AZ_WALL_NOTHING;
              break;
          }
        }
        break;
      case AZ_OP_GPOS:
        {
          az_object_t object;
          GET_OBJECT(&object);
          az_vector_t position = AZ_VZERO;
          switch (object.type) {
            case AZ_OBJ_NOTHING: break;
            case AZ_OBJ_BADDIE:
              position = object.obj.baddie->position;
              break;
            case AZ_OBJ_DOOR:
              position = object.obj.door->position;
              break;
            case AZ_OBJ_GRAVFIELD:
              position = object.obj.gravfield->position;
              break;
            case AZ_OBJ_NODE:
              position = object.obj.node->position;
              break;
            case AZ_OBJ_WALL:
              position = object.obj.wall->position;
              break;
          }
          STACK_PUSH(position.x, position.y);
        }
        break;
      case AZ_OP_SPOS:
        {
          az_object_t object;
          GET_OBJECT(&object);
          az_vector_t position;
          STACK_POP(&position.x, &position.y);
          switch (object.type) {
            case AZ_OBJ_NOTHING: break;
            case AZ_OBJ_BADDIE:
              object.obj.baddie->position = position;
              break;
            case AZ_OBJ_DOOR:
              object.obj.door->position = position;
              break;
            case AZ_OBJ_GRAVFIELD:
              object.obj.gravfield->position = position;
              break;
            case AZ_OBJ_NODE:
              object.obj.node->position = position;
              break;
            case AZ_OBJ_WALL:
              object.obj.wall->position = position;
              break;
          }
        }
        break;
      // Baddies:
      case AZ_OP_BAD:
        {
          double k, x, y, angle;
          STACK_POP(&k, &x, &y, &angle);
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
      case AZ_OP_BOSS:
        {
          az_uid_t uid;
          GET_UID(AZ_UUID_BADDIE, &uid);
          state->boss_uid = uid;
        }
        break;
      case AZ_OP_SPLAT:
        AZ_ARRAY_LOOP(baddie, state->baddies) {
          if (baddie->kind == AZ_BAD_NOTHING) continue;
          az_kill_baddie(state, baddie);
        }
        break;
      // Doors:
      case AZ_OP_OPEN:
        {
          az_uid_t uid;
          GET_UID(AZ_UUID_DOOR, &uid);
          az_door_t *door;
          if (az_lookup_door(state, uid, &door)) {
            if (door->kind == AZ_DOOR_PASSAGE) {
              SCRIPT_ERROR("wrong door kind");
            }
            if (!door->is_open) {
              az_play_sound(&state->soundboard, AZ_SND_DOOR_OPEN);
              door->is_open = true;
            }
          }
        }
        break;
      case AZ_OP_CLOSE:
        {
          az_uid_t uid;
          GET_UID(AZ_UUID_DOOR, &uid);
          az_door_t *door;
          if (az_lookup_door(state, uid, &door)) {
            if (door->kind == AZ_DOOR_PASSAGE) {
              SCRIPT_ERROR("wrong door kind");
            }
            if (door->is_open) {
              az_play_sound(&state->soundboard, AZ_SND_DOOR_CLOSE);
              door->is_open = false;
            }
          }
        }
        break;
      case AZ_OP_LOCK:
        {
          az_uid_t uid;
          GET_UID(AZ_UUID_DOOR, &uid);
          az_door_t *door;
          if (az_lookup_door(state, uid, &door)) {
            if (door->kind == AZ_DOOR_PASSAGE ||
                door->kind == AZ_DOOR_FORCEFIELD) {
              SCRIPT_ERROR("wrong door kind");
            }
            door->kind = AZ_DOOR_LOCKED;
            if (door->is_open) {
              az_play_sound(&state->soundboard, AZ_SND_DOOR_CLOSE);
              door->is_open = false;
            }
          }
        }
        break;
      case AZ_OP_UNLOCK:
        {
          az_uid_t uid;
          GET_UID(AZ_UUID_DOOR, &uid);
          az_door_t *door;
          if (az_lookup_door(state, uid, &door)) {
            if (door->kind == AZ_DOOR_PASSAGE ||
                door->kind == AZ_DOOR_FORCEFIELD) {
              SCRIPT_ERROR("wrong door kind");
            }
            door->kind = AZ_DOOR_NORMAL;
          }
        }
        break;
      // Gravfields:
      case AZ_OP_GSTR:
        {
          az_uid_t uid;
          GET_UID(AZ_UUID_GRAVFIELD, &uid);
          az_gravfield_t *gravfield;
          STACK_PUSH(az_lookup_gravfield(state, uid, &gravfield) ?
                     gravfield->strength : 0.0);
        }
        break;
      case AZ_OP_SSTR:
        {
          double value;
          STACK_POP(&value);
          az_uid_t uid;
          GET_UID(AZ_UUID_GRAVFIELD, &uid);
          az_gravfield_t *gravfield;
          if (az_lookup_gravfield(state, uid, &gravfield)) {
            gravfield->strength = value;
          }
        }
        break;
      // Nodes:
      case AZ_OP_GMARK:
        {
          az_uid_t uid;
          GET_UID(AZ_UUID_NODE, &uid);
          double value = 0.0;
          az_node_t *node;
          if (az_lookup_node(state, uid, &node)) {
            if (node->kind == AZ_NODE_MARKER) {
              value = (double)node->subkind.marker;
            } else SCRIPT_ERROR("invalid node kind");
          }
          STACK_PUSH(value);
        }
        break;
      case AZ_OP_SMARK:
        {
          az_uid_t uid;
          GET_UID(AZ_UUID_NODE, &uid);
          double value;
          STACK_POP(&value);
          az_node_t *node;
          if (az_lookup_node(state, uid, &node)) {
            if (node->kind == AZ_NODE_MARKER) {
              node->subkind.marker = (int)value;
            } else SCRIPT_ERROR("invalid node kind");
          }
        }
        break;
      // Camera:
      case AZ_OP_GCAM:
        STACK_PUSH(state->camera.center.x, state->camera.center.y);
        break;
      case AZ_OP_NPS:
        {
          double x, y;
          STACK_POP(&x, &y);
          state->camera.wobble_goal = 0.5 * ins.immediate;
          az_particle_t *particle;
          if (az_insert_particle(state, &particle)) {
            particle->kind = AZ_PAR_NPS_PORTAL;
            particle->color = (az_color_t){128, 64, 255, 255};
            particle->position.x = x;
            particle->position.y = y;
            particle->velocity = AZ_VZERO;
            particle->angle = 0.0;
            particle->lifetime = ins.immediate;
            particle->param1 = 50.0 * sqrt(ins.immediate);
          }
        }
        break;
      // Messages/dialog:
      case AZ_OP_MSG:
        {
          const int text_index = (int)ins.immediate;
          if (text_index < 0 || text_index >= state->planet->num_texts) {
            SCRIPT_ERROR("invalid text index");
          }
          const az_text_t *text = &state->planet->texts[text_index];
          state->message.text = text;
          state->message.time_remaining = 2.5;
          for (int i = 0; i < text->num_lines; ++i) {
            state->message.time_remaining +=
              0.05 * text->lines[i].total_length;
          }
        }
        break;
      case AZ_OP_DLOG:
        if (state->mode == AZ_MODE_NORMAL) {
          state->mode = AZ_MODE_DIALOG;
          memset(&state->mode_data.dialog, 0, sizeof(state->mode_data.dialog));
          state->mode_data.dialog.step = AZ_DLS_BEGIN;
          SUSPEND(vm->script, &state->mode_data.dialog.vm);
        }
        SCRIPT_ERROR("can't start dialog now");
      case AZ_OP_PT:
        if (state->mode == AZ_MODE_DIALOG) {
          const int portrait = (int)ins.immediate;
          if (portrait < 0 || portrait > AZ_NUM_PORTRAITS) {
            SCRIPT_ERROR("invalid portrait");
          } else {
            state->mode_data.dialog.top = (az_portrait_t)portrait;
          }
        } else SCRIPT_ERROR("not in dialog");
        break;
      case AZ_OP_PB:
        if (state->mode == AZ_MODE_DIALOG) {
          const int portrait = (int)ins.immediate;
          if (portrait < 0 || portrait > AZ_NUM_PORTRAITS) {
            SCRIPT_ERROR("invalid portrait");
          } else {
            state->mode_data.dialog.bottom = (az_portrait_t)portrait;
          }
        } else SCRIPT_ERROR("not in dialog");
        break;
      case AZ_OP_TT:
        if (state->mode == AZ_MODE_DIALOG) {
          const int text_index = (int)ins.immediate;
          if (text_index < 0 || text_index >= state->planet->num_texts) {
            SCRIPT_ERROR("invalid text index");
          }
          const az_text_t *text = &state->planet->texts[text_index];
          state->mode_data.dialog.step = AZ_DLS_TALK;
          state->mode_data.dialog.progress = 0.0;
          state->mode_data.dialog.bottom_next = false;
          state->mode_data.dialog.text = text;
          state->mode_data.dialog.row = state->mode_data.dialog.col = 0;
          SUSPEND(vm->script, &state->mode_data.dialog.vm);
        }
        SCRIPT_ERROR("not in dialog");
      case AZ_OP_TB:
        if (state->mode == AZ_MODE_DIALOG) {
          const int text_index = (int)ins.immediate;
          if (text_index < 0 || text_index >= state->planet->num_texts) {
            SCRIPT_ERROR("invalid text index");
          }
          const az_text_t *text = &state->planet->texts[text_index];
          state->mode_data.dialog.step = AZ_DLS_TALK;
          state->mode_data.dialog.progress = 0.0;
          state->mode_data.dialog.bottom_next = true;
          state->mode_data.dialog.text = text;
          state->mode_data.dialog.row = state->mode_data.dialog.col = 0;
          SUSPEND(vm->script, &state->mode_data.dialog.vm);
        }
        SCRIPT_ERROR("not in dialog");
      case AZ_OP_DEND:
        if (state->mode == AZ_MODE_DIALOG) {
          state->mode_data.dialog.step = AZ_DLS_END;
          state->mode_data.dialog.progress = 0.0;
          state->mode_data.dialog.text = NULL;
          SUSPEND(vm->script, &state->mode_data.dialog.vm);
        }
        SCRIPT_ERROR("not in dialog");
      // Music/sound:
      case AZ_OP_MUS:
        {
          const int music_index = (int)ins.immediate;
          if (music_index < 0 || music_index >= AZ_NUM_MUSIC_KEYS) {
            SCRIPT_ERROR("invalid music index");
          }
          az_change_music(&state->soundboard, (az_music_key_t)music_index);
        }
        break;
      case AZ_OP_SND:
        {
          const int sound_index = (int)ins.immediate;
          if (sound_index < 0 || sound_index >= AZ_NUM_SOUND_KEYS) {
            SCRIPT_ERROR("invalid sound index");
          }
          az_play_sound(&state->soundboard, (az_sound_key_t)sound_index);
        }
        break;
      // Termination:
      case AZ_OP_WAIT:
        if (state->mode == AZ_MODE_DIALOG) {
          SCRIPT_ERROR("can't WAIT during dialog");
        }
        if (ins.immediate > 0.0) {
          const az_script_t *script = vm->script;
          vm->script = NULL;
          AZ_ARRAY_LOOP(timer, state->timers) {
            if (timer->vm.script != NULL) continue;
            timer->time_remaining = ins.immediate;
            SUSPEND(script, &timer->vm);
          }
          SCRIPT_ERROR("too many timers");
        }
        break;
      case AZ_OP_STOP: goto halt;
      case AZ_OP_ERROR: SCRIPT_ERROR("ERROR opcode");
    }
    ++vm->pc;
    assert(vm->pc >= 0);
    assert(vm->pc <= vm->script->num_instructions);
  }

 halt:
  if (state->mode == AZ_MODE_DIALOG) {
    memset(&state->mode_data.dialog, 0, sizeof(state->mode_data.dialog));
    state->mode_data.dialog.step = AZ_DLS_END;
  }
  // Now that the script has completed, zero out the VM.  If the resumed script
  // was a timer, this will have the effect of marking the timer as inactive.
  memset(vm, 0, sizeof(*vm));
}

/*===========================================================================*/

void az_tick_timers(az_space_state_t *state, double time) {
  AZ_ARRAY_LOOP(timer, state->timers) {
    if (timer->vm.script == NULL) continue;
    assert(timer->time_remaining > 0.0);
    timer->time_remaining -= time;
    if (timer->time_remaining <= 0.0) {
      timer->time_remaining = 0.0;
      az_resume_script(state, &timer->vm);
    }
  }
}

/*===========================================================================*/
