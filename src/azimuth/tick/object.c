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

#include "azimuth/tick/object.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>

#include "azimuth/state/baddie.h"
#include "azimuth/state/door.h"
#include "azimuth/state/gravfield.h"
#include "azimuth/state/object.h"
#include "azimuth/state/node.h"
#include "azimuth/state/ship.h"
#include "azimuth/state/space.h"
#include "azimuth/state/uid.h"
#include "azimuth/state/wall.h"
#include "azimuth/tick/baddie.h"
#include "azimuth/tick/script.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/random.h"
#include "azimuth/util/vector.h"
#include "azimuth/util/warning.h"

/*===========================================================================*/

// Forward-declaration, needed for mutual recursion:
static void move_baddie_cargo_internal(
    az_space_state_t *state, az_baddie_t *baddie, az_vector_t delta_position,
    double delta_angle, int depth);

static void move_object_internal(
    az_space_state_t *state, az_object_t *object, az_vector_t delta_position,
    double delta_angle, int depth) {
  // TODO: Can we instead do this with an explicit stack, the way we do in
  // kill_object_internal?
  if (depth > 3) {
    AZ_WARNING_ONCE("az_move_object recursion overflow\n");
    return;
  }
  assert(object->type != AZ_OBJ_NOTHING);
  switch (object->type) {
    case AZ_OBJ_NOTHING: AZ_ASSERT_UNREACHABLE();
    case AZ_OBJ_BADDIE:
      assert(object->obj.baddie->kind != AZ_BAD_NOTHING);
      object->obj.baddie->position =
        az_vadd(object->obj.baddie->position, delta_position);
      object->obj.baddie->angle =
        az_mod2pi(object->obj.baddie->angle + delta_angle);
      // When a baddie is made to move, its cargo moves with it.
      move_baddie_cargo_internal(state, object->obj.baddie, delta_position,
                                 delta_angle, depth + 1);
      break;
    case AZ_OBJ_DOOR:
      assert(object->obj.door->kind != AZ_DOOR_NOTHING);
      object->obj.door->position =
        az_vadd(object->obj.door->position, delta_position);
      object->obj.door->angle =
        az_mod2pi(object->obj.door->angle + delta_angle);
      break;
    case AZ_OBJ_GRAVFIELD:
      assert(object->obj.gravfield->kind != AZ_GRAV_NOTHING);
      object->obj.gravfield->position =
        az_vadd(object->obj.gravfield->position, delta_position);
      object->obj.gravfield->angle =
        az_mod2pi(object->obj.gravfield->angle + delta_angle);
      break;
    case AZ_OBJ_NODE:
      assert(object->obj.node->kind != AZ_NODE_NOTHING);
      if (az_vnonzero(delta_position)) {
        const az_vector_t new_node_position =
          az_vadd(object->obj.node->position, delta_position);
        // If we move a tractor node while the ship is locked onto it, we have
        // to drag the ship along.
        if (object->obj.node->kind == AZ_NODE_TRACTOR &&
            state->ship.tractor_beam.active) {
          az_ship_t *ship = &state->ship;
          az_node_t *tractor_node = NULL;
          if (az_lookup_node(state, ship->tractor_beam.node_uid,
                             &tractor_node) &&
              tractor_node == object->obj.node) {
            // Figure out where the ship is begin dragged to.
            const az_vector_t new_ship_position =
              az_vadd(new_node_position, az_vwithlen(
                az_vsub(ship->position, new_node_position),
                ship->tractor_beam.distance));
            // Check whether pulling the ship makes it hit something along the
            // way.
            az_impact_flags_t impact_flags = AZ_IMPF_SHIP;
            if (ship->temp_invincibility > 0.0) impact_flags |= AZ_IMPF_BADDIE;
            az_impact_t impact;
            az_circle_impact(
                state, AZ_SHIP_DEFLECTOR_RADIUS, ship->position,
                az_vsub(new_ship_position, ship->position), impact_flags,
                AZ_SHIP_UID, &impact);
            ship->position = impact.position;
            // If the ship hit something, break the tractor beam.
            if (impact.type != AZ_IMP_NOTHING) {
              ship->position =
                az_vadd(ship->position, az_vwithlen(impact.normal, 0.5));
              ship->tractor_beam.active = false;
              ship->tractor_beam.node_uid = AZ_NULL_UID;
            }
          }
        }
        object->obj.node->position = new_node_position;
      }
      object->obj.node->angle =
        az_mod2pi(object->obj.node->angle + delta_angle);
      break;
    case AZ_OBJ_SHIP:
      assert(object->obj.ship == &state->ship);
      assert(az_ship_is_present(object->obj.ship));
      if (az_vnonzero(delta_position)) {
        // If the ship is forced to move while the tractor beam is active,
        // break the tractor beam.
        if (object->obj.ship->tractor_beam.active) {
          object->obj.ship->tractor_beam.active = false;
          object->obj.ship->tractor_beam.node_uid = AZ_NULL_UID;
        }
        object->obj.ship->position =
          az_vadd(object->obj.ship->position, delta_position);
      }
      object->obj.ship->angle =
        az_mod2pi(object->obj.ship->angle + delta_angle);
      break;
    case AZ_OBJ_WALL:
      assert(object->obj.wall->kind != AZ_WALL_NOTHING);
      object->obj.wall->position =
        az_vadd(object->obj.wall->position, delta_position);
      object->obj.wall->angle =
        az_mod2pi(object->obj.wall->angle + delta_angle);
      break;
  }
}

void az_move_object(az_space_state_t *state, az_object_t *object,
                    az_vector_t delta_position, double delta_angle) {
  move_object_internal(state, object, delta_position, delta_angle, 0);
}

static void move_baddie_cargo_internal(
    az_space_state_t *state, az_baddie_t *baddie, az_vector_t delta_position,
    double delta_angle, int depth) {
  AZ_ARRAY_LOOP(uuid, baddie->cargo_uuids) {
    az_object_t cargo;
    if (az_lookup_object(state, *uuid, &cargo)) {
      const az_vector_t old_rel =
        az_vsub(az_get_object_position(&cargo),
                az_vsub(baddie->position, delta_position));
      const az_vector_t new_rel = az_vrotate(old_rel, delta_angle);
      move_object_internal(
          state, &cargo,
          az_vadd(delta_position, az_vsub(new_rel, old_rel)), delta_angle,
          depth);
    }
  }
}

void az_move_baddie_cargo(az_space_state_t *state, az_baddie_t *baddie,
                          az_vector_t delta_position, double delta_angle) {
  move_baddie_cargo_internal(state, baddie, delta_position, delta_angle, 1);
}

/*===========================================================================*/

// Kill a baddie, but don't affect its cargo.
static void kill_baddie_internal(
    az_space_state_t *state, az_baddie_t *baddie, bool pickups_and_scripts) {
  assert(baddie->kind != AZ_BAD_NOTHING);
  az_play_sound(&state->soundboard, baddie->data->death_sound);
  // Add particles for baddie debris:
  const double overall_radius = baddie->data->overall_bounding_radius;
  const double step = 6.0;
  for (double y = -overall_radius; y <= overall_radius; y += step) {
    for (double x = -overall_radius; x <= overall_radius; x += step) {
      const az_vector_t pos = {x + baddie->position.x + az_random(-3, 3),
                               y + baddie->position.y + az_random(-3, 3)};
      const az_death_style_t dstyle = baddie->data->death_style;
      const az_component_data_t *component;
      az_vector_t component_pos;
      az_particle_t *particle;
      if (az_point_touches_baddie(baddie, pos, &component, &component_pos) &&
          az_insert_particle(state, &particle)) {
        particle->kind = (dstyle == AZ_DEATH_EMBERS ? AZ_PAR_EMBER :
                          dstyle == AZ_DEATH_OTH ? AZ_PAR_OTH_FRAGMENT :
                          AZ_PAR_SHARD);
        particle->color = baddie->data->color;
        particle->position = pos;
        particle->angle = az_random(0.0, AZ_TWO_PI);
        particle->lifetime = az_random(0.5, 1.0);
        particle->param1 = az_random(0.5, 1.5) * step *
          (particle->kind == AZ_PAR_SHARD ? 0.25 : 1.4);
        particle->param2 = az_random(-10.0, 10.0);
        const double component_radius = component->bounding_radius;
        particle->velocity = az_vsub(pos, component_pos);
        if (dstyle != AZ_DEATH_EMBERS) {
          particle->velocity = az_vmul(particle->velocity, 5.0);
        }
        particle->velocity.x += az_random(-component_radius, component_radius);
        particle->velocity.y += az_random(-component_radius, component_radius);
      }
    }
  }
  for (int i = 0; i < 20; ++i) {
    az_add_speck(state, AZ_WHITE, 2.0, baddie->position,
                 az_vpolar(az_random(20, 70), az_random(0, AZ_TWO_PI)));
  }

  if (pickups_and_scripts) {
    az_add_random_pickup(state, baddie->data->potential_pickups,
                         baddie->position);
  }
  const az_baddie_kind_t kind = baddie->kind;
  const az_vector_t position = baddie->position;
  const double angle = baddie->angle;
  const az_script_t *script = baddie->on_kill;
  // Remove the baddie.  After this point, we can no longer use the baddie
  // object.
  baddie->kind = AZ_BAD_NOTHING;
  if (pickups_and_scripts) {
    az_on_baddie_killed(state, kind, position, angle);
    az_run_script(state, script);
  }
}

static void kill_object_internal(
    az_space_state_t *state, az_object_t *object, bool run_scripts) {
  az_uuid_t stack[AZ_NUM_UUID_SLOTS];
  int stack_size = 0;
  while (true) {
    // Kill the current object.
    switch (object->type) {
      case AZ_OBJ_NOTHING: break;
      case AZ_OBJ_BADDIE:
        {
          az_baddie_t *baddie = object->obj.baddie;
          assert(baddie->kind != AZ_BAD_NOTHING);
          // Push the baddie's cargo (if any) onto the stack.
          for (int i = 0; i < AZ_ARRAY_SIZE(baddie->cargo_uuids); ++i) {
            if (object->obj.baddie->cargo_uuids[i].type != AZ_UUID_NOTHING) {
              if (stack_size >= AZ_ARRAY_SIZE(stack)) {
                AZ_WARNING_ONCE("kill_object stack full\n");
                break;
              }
              stack[stack_size++] = object->obj.baddie->cargo_uuids[i];
            }
          }
          // Now kill this baddie before moving on to its cargo in the next
          // iteration.
          kill_baddie_internal(state, object->obj.baddie, run_scripts);
        }
        break;
      case AZ_OBJ_DOOR:
        assert(object->obj.door->kind != AZ_DOOR_NOTHING);
        object->obj.door->kind = AZ_DOOR_NOTHING;
        break;
      case AZ_OBJ_GRAVFIELD:
        assert(object->obj.gravfield->kind != AZ_GRAV_NOTHING);
        object->obj.gravfield->kind = AZ_GRAV_NOTHING;
        break;
      case AZ_OBJ_NODE:
        assert(object->obj.node->kind != AZ_NODE_NOTHING);
        object->obj.node->kind = AZ_NODE_NOTHING;
        break;
      case AZ_OBJ_SHIP:
        assert(object->obj.ship == &state->ship);
        assert(az_ship_is_present(object->obj.ship));
        az_kill_ship(state);
        break;
      case AZ_OBJ_WALL:
        assert(object->obj.wall->kind != AZ_WALL_NOTHING);
        az_break_wall(state, object->obj.wall);
        break;
    }
    object->type = AZ_OBJ_NOTHING;
    // Advance to the next object.
    do {
      if (stack_size == 0) return;
    } while (!az_lookup_object(state, stack[--stack_size], object));
  }
}

void az_kill_object(az_space_state_t *state, az_object_t *object) {
  kill_object_internal(state, object, false);
}

/*===========================================================================*/
// Killing baddies:

// The level of health at or below which a baddie can be frozen.
#define AZ_BADDIE_FREEZE_THRESHOLD 4.0

bool az_try_damage_baddie(
    az_space_state_t *state, az_baddie_t *baddie,
    const az_component_data_t *component, az_damage_flags_t damage_kind,
    double damage_amount) {
  assert(baddie->kind != AZ_BAD_NOTHING);
  assert(baddie->health > 0.0);
  assert(damage_amount >= 0.0);

  // If the damage is zero or the baddie is invincible, we can quit early.
  if (damage_amount <= 0.0) return false;
  if (az_baddie_has_flag(baddie, AZ_BADF_INVINCIBLE)) return false;

  // Determine if the baddie is susceptible to this kind of damage; if so,
  // damage the baddie.
  bool damage_was_dealt = false;
  az_damage_flags_t modified_damage_kind = damage_kind;
  modified_damage_kind &= ~AZ_DMGF_FREEZE;
  if (!modified_damage_kind) modified_damage_kind = AZ_DMGF_NORMAL;
  if (modified_damage_kind & ~(component->immunities)) {
    baddie->armor_flare = 1.0;
    baddie->health -= damage_amount;
    damage_was_dealt = true;
  }

  // If the baddie is still alive, it might still get frozen.
  if (baddie->health > 0.0) {
    // If (1) the damage kind includes AZ_DMGF_FREEZE, (2) the baddie is
    // susceptible to being frozen, and (3) the baddie's health is low enough
    // for it to be frozen, then freeze the baddie (even if the baddie didn't
    // actually take any damage from this hit).
    const double freeze_threshold = 4.0;
    if ((damage_kind & AZ_DMGF_FREEZE) &&
        !(component->immunities & AZ_DMGF_FREEZE) &&
        baddie->health <= fmax(damage_amount, 1.0) * freeze_threshold) {
      baddie->frozen = 1.0;
    }
  }
  // Otherwise, the baddie is dead, so have it killed.
  else {
    assert(damage_was_dealt);
    // If this was the boss, go into boss-death mode.
    if (baddie->uid == state->boss_uid) {
      baddie->health = 0.000001;
      // The boss can only die in normal mode (not in, say, game over mode).
      if (!(state->mode == AZ_MODE_NORMAL ||
            (state->mode == AZ_MODE_DOORWAY &&
             state->doorway_mode.step == AZ_DWS_FADE_IN))) return true;
      state->mode = AZ_MODE_BOSS_DEATH;
      state->boss_death_mode = (az_boss_death_mode_data_t){
        .step = AZ_BDS_SHAKE, .progress = 0.0, .boss = *baddie
      };
      baddie->kind = AZ_BAD_NOTHING;
      // When a boss dies, kill all other baddies in the room, except for
      // baddies that are _permanently_ incorporeal.
      AZ_ARRAY_LOOP(other, state->baddies) {
        if (other->kind == AZ_BAD_NOTHING) continue;
        if (other->data->static_properties & AZ_BADF_INCORPOREAL) continue;
        az_kill_baddie(state, other);
      }
      return true;
    }
    // Kill the baddie.  After this point, we can no longer use the baddie
    // object.
    az_object_t object = { .type = AZ_OBJ_BADDIE, .obj.baddie = baddie };
    kill_object_internal(state, &object, true);
  }

  return damage_was_dealt;
}

void az_kill_baddie(az_space_state_t *state, az_baddie_t *baddie) {
  az_object_t object = { .type = AZ_OBJ_BADDIE, .obj.baddie = baddie };
  kill_object_internal(state, &object, false);
}

/*===========================================================================*/
// Killing the ship:

void az_damage_ship(az_space_state_t *state, double damage,
                    bool induce_temp_invincibility) {
  az_ship_t *ship = &state->ship;
  assert(az_ship_is_present(ship));
  assert(damage >= 0.0);
  if (ship->temp_invincibility > 0.0 ||
      ship->cplus.state == AZ_CPLUS_ACTIVE) return;
  if (induce_temp_invincibility) ship->temp_invincibility = 1.0;
  if (damage <= 0.0) return;
  ship->shield_flare = 1.0;
  if (state->mode != AZ_MODE_NORMAL) return;
  // Each armor upgrade cumulatively reduces the damage taken.
  const az_player_t *player = &ship->player;
  if (az_has_upgrade(player, AZ_UPG_HARDENED_ARMOR)) {
    damage *= AZ_ARMOR_DAMAGE_FACTOR;
  }
  if (az_has_upgrade(player, AZ_UPG_DYNAMIC_ARMOR)) {
    damage *= AZ_ARMOR_DAMAGE_FACTOR;
  }
  if (az_has_upgrade(player, AZ_UPG_THERMAL_ARMOR)) {
    damage *= AZ_ARMOR_DAMAGE_FACTOR;
  }
  if (az_has_upgrade(player, AZ_UPG_REACTIVE_ARMOR)) {
    damage *= AZ_ARMOR_DAMAGE_FACTOR;
  }
  // If the ship can survive the damage, reduce shields and we're done.
  if (ship->player.shields > damage) {
    ship->player.shields -= damage;
    return;
  }
  // Otherwise, the ship will be destroyed.
  az_kill_ship(state);
}

void az_kill_ship(az_space_state_t *state) {
  az_ship_t *ship = &state->ship;
  assert(az_ship_is_present(ship));
  // Add particles for ship debris:
  az_particle_t *particle;
  if (az_insert_particle(state, &particle)) {
    particle->kind = AZ_PAR_BOOM;
    particle->color = AZ_WHITE;
    particle->position = ship->position;
    particle->velocity = AZ_VZERO;
    particle->lifetime = 0.5;
    particle->param1 = 30;
  }
  const double radius = 20.0;
  for (double y = -radius; y <= radius; y += 4.0) {
    for (double x = -radius; x <= radius; x += 3.0) {
      const az_vector_t pos = {x + ship->position.x + az_random(-2.0, 2.0),
                               y + ship->position.y + az_random(-2.0, 2.0)};
      if (az_point_touches_ship(ship, pos) &&
          az_insert_particle(state, &particle)) {
        particle->kind = AZ_PAR_SHARD;
        particle->color = (az_color_t){160, 160, 160, 255};
        particle->position = pos;
        particle->velocity = az_vmul(az_vsub(pos, ship->position), 5.0);
        particle->velocity.x += az_random(-50.0, 50.0);
        particle->velocity.y += az_random(-50.0, 50.0);
        particle->angle = az_random(0.0, AZ_TWO_PI);
        particle->lifetime = az_random(0.5, 1.0);
        particle->param1 = az_random(0.5, 1.5);
        particle->param2 = az_random(-10.0, 10.0);
      }
    }
  }
  for (int i = 0; i < 20; ++i) {
    az_add_speck(state, AZ_WHITE, 2.0, ship->position,
                 az_vpolar(az_random(20, 70), az_random(0, AZ_TWO_PI)));
  }
  az_play_sound(&state->soundboard, AZ_SND_EXPLODE_SHIP);
  // Destroy the ship:
  ship->player.shields = 0.0;
  // Put us into game-over mode:
  state->mode = AZ_MODE_GAME_OVER;
  state->game_over_mode = (az_game_over_mode_data_t){
    .step = AZ_GOS_ASPLODE, .progress = 0.0
  };
}

/*===========================================================================*/
// Breaking walls:

static void break_wall_internal(
    az_space_state_t *state, az_wall_t *wall, az_vector_t impact_point) {
  assert(wall->kind != AZ_WALL_NOTHING);
  az_play_sound(&state->soundboard, AZ_SND_KILL_TURRET);
  // Place particles for wall debris.
  const double radius = wall->data->bounding_radius;
  const double step = 3.0 + radius / 10.0;
  const double size = 0.7 + radius / 60.0;
  az_particle_t *particle;
  for (double y = -radius; y <= radius; y += step) {
    for (double x = -radius; x <= radius; x += step) {
      const az_vector_t pos = {x + wall->position.x + az_random(-5.0, 5.0),
                               y + wall->position.y + az_random(-5.0, 5.0)};
      if (az_point_touches_wall(wall, pos) &&
          az_insert_particle(state, &particle)) {
        particle->kind = AZ_PAR_SHARD;
        particle->color = wall->data->color1;
        particle->position = pos;
        particle->velocity =
          az_vwithlen(az_vsub(pos, impact_point),
                      az_vdist(pos, wall->position) * 2.5);
        particle->velocity.x += az_random(-radius, radius);
        particle->velocity.y += az_random(-radius, radius);
        particle->angle = az_random(0.0, AZ_TWO_PI);
        particle->lifetime = az_random(0.3, 0.8);
        particle->param1 = az_random(0.5, 1.5) * size;
        particle->param2 = az_random(-10.0, 10.0);
      }
    }
  }
  // Remove the wall.
  wall->kind = AZ_WALL_NOTHING;
}

bool az_try_break_wall(az_space_state_t *state, az_wall_t *wall,
                       az_damage_flags_t damage_kind,
                       az_vector_t impact_point) {
  assert(wall->kind != AZ_WALL_NOTHING);
  // If the damage kind can't destroy any kind of wall, then we quit early
  // (without even flaring the wall).
  if ((damage_kind & AZ_DMGF_WALL_FLARE) == 0) return false;
  // Determine what kind of damage this wall is vulnerable to.
  az_damage_flags_t vulnerability = 0;
  switch (wall->kind) {
    case AZ_WALL_NOTHING: AZ_ASSERT_UNREACHABLE();
    case AZ_WALL_INDESTRUCTIBLE: return false;
    case AZ_WALL_DESTRUCTIBLE_CHARGED:
      vulnerability = AZ_DMGF_WALL_FLARE;
      break;
    case AZ_WALL_DESTRUCTIBLE_ROCKET:
      vulnerability = AZ_DMGF_ROCKET | AZ_DMGF_HYPER_ROCKET;
      break;
    case AZ_WALL_DESTRUCTIBLE_HYPER_ROCKET:
      vulnerability = AZ_DMGF_HYPER_ROCKET;
      break;
    case AZ_WALL_DESTRUCTIBLE_BOMB:
      vulnerability = AZ_DMGF_BOMB | AZ_DMGF_MEGA_BOMB;
      break;
    case AZ_WALL_DESTRUCTIBLE_MEGA_BOMB:
      vulnerability = AZ_DMGF_MEGA_BOMB;
      break;
    case AZ_WALL_DESTRUCTIBLE_CPLUS:
      vulnerability = AZ_DMGF_CPLUS;
      break;
  }
  // If the wall is vulnerable to the damage being dealt, destroy the wall.
  if ((damage_kind & vulnerability) != 0) {
    break_wall_internal(state, wall, impact_point);
    return true;
  }
  // Otherwise, flare the wall (to make visible to the player what kind of
  // damage would be able to destroy this wall).
  else {
    wall->flare = 1.0;
    az_play_sound(&state->soundboard, AZ_SND_METAL_CLINK);
    return false;
  }
}

void az_break_wall(az_space_state_t *state, az_wall_t *wall) {
  assert(wall->kind != AZ_WALL_NOTHING);
  break_wall_internal(state, wall, wall->position);
}

/*===========================================================================*/
