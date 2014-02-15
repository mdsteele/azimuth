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

#include "azimuth/state/space.h"

#include <assert.h>
#include <stdbool.h>

#include "azimuth/state/room.h"
#include "azimuth/state/uid.h"
#include "azimuth/state/upgrade.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h"
#include "azimuth/util/warning.h"

/*===========================================================================*/

void az_clear_space(az_space_state_t *state) {
  state->darkness = state->dark_goal = 0.0;
  state->boss_uid = AZ_NULL_UID;
  AZ_ZERO_ARRAY(state->baddies);
  AZ_ZERO_ARRAY(state->doors);
  AZ_ZERO_ARRAY(state->gravfields);
  AZ_ZERO_ARRAY(state->nodes);
  AZ_ZERO_ARRAY(state->particles);
  AZ_ZERO_ARRAY(state->pickups);
  AZ_ZERO_ARRAY(state->projectiles);
  AZ_ZERO_ARRAY(state->specks);
  AZ_ZERO_ARRAY(state->timers);
  AZ_ZERO_ARRAY(state->walls);
  AZ_ZERO_ARRAY(state->uuids);
}

static void put_uuid(az_space_state_t *state, int slot,
                     az_uuid_type_t type, az_uid_t uid) {
  if (slot != 0) {
    assert(slot >= 1);
    assert(slot <= AZ_ARRAY_SIZE(state->uuids));
    state->uuids[slot - 1].type = type;
    state->uuids[slot - 1].uid = uid;
  }
}

void az_enter_room(az_space_state_t *state, const az_room_t *room) {
  state->darkness = state->dark_goal = 0.0;
  // Make a map from UUID table indices to the baddie (if any) carrying that
  // object as cargo.
  az_baddie_t *cargo_carriers[AZ_NUM_UUID_SLOTS];
  AZ_ZERO_ARRAY(cargo_carriers);
  // Insert objects into space:
  for (int i = 0; i < room->num_baddies; ++i) {
    const az_baddie_spec_t *spec = &room->baddies[i];
    az_baddie_t *baddie =
      az_add_baddie(state, spec->kind, spec->position, spec->angle);
    if (baddie != NULL) {
      baddie->on_kill = spec->on_kill;
      put_uuid(state, spec->uuid_slot, AZ_UUID_BADDIE, baddie->uid);
      AZ_ARRAY_LOOP(slot, spec->cargo_slots) {
        if (*slot == 0) continue;
        const int slot_index = *slot - 1;
        assert(slot_index >= 0);
        assert(slot_index < AZ_ARRAY_SIZE(cargo_carriers));
        cargo_carriers[slot_index] = baddie;
      }
    }
  }
  for (int i = 0; i < room->num_doors; ++i) {
    const az_door_spec_t *spec = &room->doors[i];
    AZ_ARRAY_LOOP(door, state->doors) {
      if (door->kind == AZ_DOOR_NOTHING) {
        door->kind = spec->kind;
        az_assign_uid(door - state->doors, &door->uid);
        put_uuid(state, spec->uuid_slot, AZ_UUID_DOOR, door->uid);
        door->on_open = spec->on_open;
        door->position = spec->position;
        door->angle = spec->angle;
        door->destination = spec->destination;
        door->is_open = (spec->kind == AZ_DOOR_PASSAGE ||
                         spec->kind == AZ_DOOR_ALWAYS_OPEN);
        door->openness = (door->is_open ? 1.0 : 0.0);
        break;
      }
    }
  }
  for (int i = 0; i < room->num_gravfields; ++i) {
    const az_gravfield_spec_t *spec = &room->gravfields[i];
    AZ_ARRAY_LOOP(gravfield, state->gravfields) {
      if (gravfield->kind == AZ_GRAV_NOTHING) {
        gravfield->kind = spec->kind;
        az_assign_uid(gravfield - state->gravfields, &gravfield->uid);
        put_uuid(state, spec->uuid_slot, AZ_UUID_GRAVFIELD, gravfield->uid);
        gravfield->on_enter = spec->on_enter;
        gravfield->position = spec->position;
        gravfield->angle = spec->angle;
        gravfield->strength = spec->strength;
        gravfield->size = spec->size;
        gravfield->age = 0.0;
        gravfield->script_fired = false;
        break;
      }
    }
  }
  for (int i = 0; i < room->num_nodes; ++i) {
    const az_node_spec_t *spec = &room->nodes[i];
    if (spec->kind == AZ_NODE_UPGRADE &&
        az_has_upgrade(&state->ship.player, spec->subkind.upgrade)) continue;
    AZ_ARRAY_LOOP(node, state->nodes) {
      if (node->kind == AZ_NODE_NOTHING) {
        node->kind = spec->kind;
        node->subkind = spec->subkind;
        az_assign_uid(node - state->nodes, &node->uid);
        put_uuid(state, spec->uuid_slot, AZ_UUID_NODE, node->uid);
        node->on_use = spec->on_use;
        node->position = spec->position;
        node->angle = spec->angle;
        node->status = AZ_NS_FAR;
        break;
      }
    }
  }
  for (int i = 0; i < room->num_walls; ++i) {
    const az_wall_spec_t *spec = &room->walls[i];
    AZ_ARRAY_LOOP(wall, state->walls) {
      if (wall->kind == AZ_WALL_NOTHING) {
        wall->kind = spec->kind;
        wall->data = spec->data;
        az_assign_uid(wall - state->walls, &wall->uid);
        put_uuid(state, spec->uuid_slot, AZ_UUID_WALL, wall->uid);
        wall->position = spec->position;
        wall->angle = spec->angle;
        wall->flare = 0.0;
        break;
      }
    }
  }
  // Now that all objects are inserted and the UUID table is populated, fill in
  // each baddie's cargo table:
  for (int i = 0; i < AZ_ARRAY_SIZE(cargo_carriers); ++i) {
    az_baddie_t *baddie = cargo_carriers[i];
    if (baddie == NULL) continue;
    assert(baddie->kind != AZ_BAD_NOTHING);
    AZ_ARRAY_LOOP(cargo, baddie->cargo_uuids) {
      if (cargo->type == AZ_UUID_NOTHING) {
        *cargo = state->uuids[i];
        break;
      }
    }
  }
}

/*===========================================================================*/

void az_set_message(az_space_state_t *state, const char *paragraph) {
  state->message = (az_message_t){
    .time_remaining =
        2.5 + 0.05 * az_paragraph_total_length(state->prefs, paragraph),
    .paragraph = paragraph,
    .num_lines = az_paragraph_num_lines(paragraph)
  };
}

az_baddie_t *az_add_baddie(az_space_state_t *state, az_baddie_kind_t kind,
                           az_vector_t position, double angle) {
  AZ_ARRAY_LOOP(baddie, state->baddies) {
    if (baddie->kind == AZ_BAD_NOTHING) {
      az_assign_uid(baddie - state->baddies, &baddie->uid);
      az_init_baddie(baddie, kind, position, angle);
      return baddie;
    }
  }
  AZ_WARNING_ONCE("Failed to add baddie (kind=%d); array is full.\n",
                  (int)kind);
  return NULL;
}

bool az_insert_particle(az_space_state_t *state,
                        az_particle_t **particle_out) {
  AZ_ARRAY_LOOP(particle, state->particles) {
    if (particle->kind == AZ_PAR_NOTHING) {
      particle->age = 0.0;
      *particle_out = particle;
      return true;
    }
  }
  AZ_WARNING_ONCE("Failed to insert particle; array is full.\n");
  return false;
}

void az_add_beam(az_space_state_t *state, az_color_t color, az_vector_t start,
                 az_vector_t end, double lifetime, double semiwidth) {
  az_particle_t *particle;
  if (az_insert_particle(state, &particle)) {
    particle->kind = AZ_PAR_BEAM;
    particle->color = color;
    particle->position = start;
    particle->velocity = AZ_VZERO;
    const az_vector_t delta = az_vsub(end, start);
    particle->angle = az_vtheta(delta);
    particle->lifetime = lifetime;
    particle->param1 = az_vnorm(delta);
    particle->param2 = semiwidth;
  }
}

void az_add_speck(az_space_state_t *state, az_color_t color, double lifetime,
                  az_vector_t position, az_vector_t velocity) {
  AZ_ARRAY_LOOP(speck, state->specks) {
    if (speck->kind == AZ_SPECK_NOTHING) {
      speck->kind = AZ_SPECK_NORMAL;
      speck->color = color;
      speck->position = position;
      speck->velocity = velocity;
      speck->age = 0.0;
      speck->lifetime = lifetime;
      return;
    }
  }
  AZ_WARNING_ONCE("Failed to add speck; array is full.\n");
}

az_projectile_t *az_add_projectile(
    az_space_state_t *state, az_proj_kind_t kind, az_vector_t position,
    double angle, double power, az_uid_t fired_by) {
  AZ_ARRAY_LOOP(proj, state->projectiles) {
    if (proj->kind == AZ_PROJ_NOTHING) {
      az_init_projectile(proj, kind, position, angle, power, fired_by);
      return proj;
    }
  }
  AZ_WARNING_ONCE("Failed to add projectile (kind=%d); array is full.\n",
                  (int)kind);
  return NULL;
}

void az_add_random_pickup(az_space_state_t *state,
                          az_pickup_flags_t potential_pickups,
                          az_vector_t position) {
  const az_pickup_kind_t kind =
    az_choose_random_pickup_kind(&state->ship.player, potential_pickups);
  if (kind == AZ_PUP_NOTHING) return;
  AZ_ARRAY_LOOP(pickup, state->pickups) {
    if (pickup->kind == AZ_PUP_NOTHING) {
      pickup->kind = kind;
      pickup->position = position;
      pickup->age = 0.0;
      return;
    }
  }
  AZ_WARNING_ONCE("Failed to add pickup (kind=%d); array is full.\n",
                  (int)kind);
}

/*===========================================================================*/

bool az_lookup_baddie(az_space_state_t *state, az_uid_t uid,
                      az_baddie_t **baddie_out) {
  const int index = az_uid_index(uid);
  assert(0 <= index && index < AZ_ARRAY_SIZE(state->baddies));
  az_baddie_t *baddie = &state->baddies[index];
  if (baddie->kind != AZ_BAD_NOTHING && baddie->uid == uid) {
    *baddie_out = baddie;
    return true;
  }
  return false;
}

bool az_lookup_door(az_space_state_t *state, az_uid_t uid,
                    az_door_t **door_out) {
  const int index = az_uid_index(uid);
  assert(0 <= index && index < AZ_ARRAY_SIZE(state->doors));
  az_door_t *door = &state->doors[index];
  if (door->kind != AZ_DOOR_NOTHING && door->uid == uid) {
    *door_out = door;
    return true;
  }
  return false;
}

bool az_lookup_gravfield(az_space_state_t *state, az_uid_t uid,
                         az_gravfield_t **gravfield_out) {
  const int index = az_uid_index(uid);
  assert(0 <= index && index < AZ_ARRAY_SIZE(state->gravfields));
  az_gravfield_t *gravfield = &state->gravfields[index];
  if (gravfield->kind != AZ_GRAV_NOTHING && gravfield->uid == uid) {
    *gravfield_out = gravfield;
    return true;
  }
  return false;
}

bool az_lookup_node(az_space_state_t *state, az_uid_t uid,
                    az_node_t **node_out) {
  const int index = az_uid_index(uid);
  assert(0 <= index && index < AZ_ARRAY_SIZE(state->nodes));
  az_node_t *node = &state->nodes[index];
  if (node->kind != AZ_NODE_NOTHING && node->uid == uid) {
    *node_out = node;
    return true;
  }
  return false;
}

bool az_lookup_wall(az_space_state_t *state, az_uid_t uid,
                    az_wall_t **wall_out) {
  const int index = az_uid_index(uid);
  assert(0 <= index && index < AZ_ARRAY_SIZE(state->walls));
  az_wall_t *wall = &state->walls[index];
  if (wall->kind != AZ_WALL_NOTHING && wall->uid == uid) {
    *wall_out = wall;
    return true;
  }
  return false;
}

/*===========================================================================*/

void az_ray_impact(az_space_state_t *state, az_vector_t start,
                   az_vector_t delta, az_impact_flags_t skip_types,
                   az_uid_t skip_uid, az_impact_t *impact_out) {
  assert(impact_out != NULL);
  impact_out->type = AZ_IMP_NOTHING;
  az_vector_t *position = &impact_out->position;
  az_vector_t *normal = &impact_out->normal;

  // Walls:
  if (!(skip_types & AZ_IMPF_WALL)) {
    AZ_ARRAY_LOOP(wall, state->walls) {
      if (wall->kind == AZ_WALL_NOTHING) continue;
      if (az_ray_hits_wall(wall, start, delta, position, normal)) {
        impact_out->type = AZ_IMP_WALL;
        impact_out->target.wall = wall;
        delta = az_vsub(*position, start);
      }
    }
  }
  // Doors:
  if (!(skip_types & AZ_IMPF_DOOR_INSIDE) ||
      !(skip_types & AZ_IMPF_DOOR_OUTSIDE)) {
    AZ_ARRAY_LOOP(door, state->doors) {
      if (door->kind == AZ_DOOR_NOTHING) continue;
      if (!(skip_types & AZ_IMPF_DOOR_INSIDE) &&
          az_ray_hits_door_inside(door, start, delta, position, normal)) {
        impact_out->type = AZ_IMP_DOOR_INSIDE;
        impact_out->target.door = door;
        delta = az_vsub(*position, start);
      }
      if (!(skip_types & AZ_IMPF_DOOR_OUTSIDE) &&
          az_ray_hits_door_outside(door, start, delta, position, normal)) {
        impact_out->type = AZ_IMP_DOOR_OUTSIDE;
        impact_out->target.door = door;
        delta = az_vsub(*position, start);
      }
    }
  }
  // Ship:
  if (!(skip_types & AZ_IMPF_SHIP) && skip_uid != AZ_SHIP_UID &&
      az_ship_is_present(&state->ship)) {
    if (az_ray_hits_ship(&state->ship, start, delta, position, normal)) {
      impact_out->type = AZ_IMP_SHIP;
      delta = az_vsub(*position, start);
    }
  }
  // Baddies:
  if (!(skip_types & AZ_IMPF_BADDIE)) {
    AZ_ARRAY_LOOP(baddie, state->baddies) {
      if (baddie->kind == AZ_BAD_NOTHING) continue;
      if (baddie->data->properties & AZ_BADF_INCORPOREAL) continue;
      if (baddie->uid == skip_uid) continue;
      const az_component_data_t *component;
      if (az_ray_hits_baddie(baddie, start, delta,
                             position, normal, &component)) {
        impact_out->type = AZ_IMP_BADDIE;
        impact_out->target.baddie.baddie = baddie;
        impact_out->target.baddie.component = component;
        delta = az_vsub(*position, start);
      }
    }
  }

  if (impact_out->type == AZ_IMP_NOTHING) {
    *position = az_vadd(start, delta);
    *normal = AZ_VZERO;
  }
}

void az_circle_impact(az_space_state_t *state, double radius,
                      az_vector_t start, az_vector_t delta,
                      az_impact_flags_t skip_types, az_uid_t skip_uid,
                      az_impact_t *impact_out) {
  assert(impact_out != NULL);
  impact_out->type = AZ_IMP_NOTHING;
  az_vector_t *position_out = &impact_out->position;
  az_vector_t *normal_out = &impact_out->normal;

  // Walls:
  if (!(skip_types & AZ_IMPF_WALL)) {
    AZ_ARRAY_LOOP(wall, state->walls) {
      if (wall->kind == AZ_WALL_NOTHING) continue;
      if (az_circle_hits_wall(wall, radius, start, delta,
                              position_out, normal_out)) {
        impact_out->type = AZ_IMP_WALL;
        impact_out->target.wall = wall;
        delta = az_vsub(*position_out, start);
      }
    }
  }
  // Doors:
  if (!(skip_types & AZ_IMPF_DOOR_INSIDE) ||
      !(skip_types & AZ_IMPF_DOOR_OUTSIDE)) {
    AZ_ARRAY_LOOP(door, state->doors) {
      if (door->kind == AZ_DOOR_NOTHING) continue;
      if (!(skip_types & AZ_IMPF_DOOR_INSIDE) &&
          az_circle_hits_door_inside(door, radius, start, delta,
                                     position_out, normal_out)) {
        impact_out->type = AZ_IMP_DOOR_INSIDE;
        impact_out->target.door = door;
        delta = az_vsub(*position_out, start);
      }
      if (!(skip_types & AZ_IMPF_DOOR_OUTSIDE) &&
          az_circle_hits_door_outside(door, radius, start, delta,
                                      position_out, normal_out)) {
        impact_out->type = AZ_IMP_DOOR_OUTSIDE;
        impact_out->target.door = door;
        delta = az_vsub(*position_out, start);
      }
    }
  }
  // Ship:
  if (!(skip_types & AZ_IMPF_SHIP) && skip_uid != AZ_SHIP_UID &&
      az_ship_is_present(&state->ship)) {
    if (az_circle_hits_ship(&state->ship, radius, start, delta,
                            position_out, normal_out)) {
      impact_out->type = AZ_IMP_SHIP;
      delta = az_vsub(*position_out, start);
    }
  }
  // Baddies:
  if (!(skip_types & AZ_IMPF_BADDIE)) {
    AZ_ARRAY_LOOP(baddie, state->baddies) {
      if (baddie->kind == AZ_BAD_NOTHING) continue;
      if (baddie->data->properties & AZ_BADF_INCORPOREAL) continue;
      if (baddie->uid == skip_uid) continue;
      const az_component_data_t *component;
      if (az_circle_hits_baddie(baddie, radius, start, delta,
                                position_out, normal_out, &component)) {
        impact_out->type = AZ_IMP_BADDIE;
        impact_out->target.baddie.baddie = baddie;
        impact_out->target.baddie.component = component;
        delta = az_vsub(*position_out, start);
      }
    }
  }

  if (impact_out->type == AZ_IMP_NOTHING) {
    *position_out = az_vadd(start, delta);
    *normal_out = AZ_VZERO;
  }
}

void az_arc_circle_impact(
    az_space_state_t *state, double circle_radius,
    az_vector_t start, az_vector_t spin_center, double spin_angle,
    az_impact_flags_t skip_types, az_uid_t skip_uid, az_impact_t *impact_out) {
  assert(impact_out != NULL);
  impact_out->type = AZ_IMP_NOTHING;
  az_vector_t *position_out = &impact_out->position;
  az_vector_t *normal_out = &impact_out->normal;

  // Walls:
  if (!(skip_types & AZ_IMPF_WALL)) {
    AZ_ARRAY_LOOP(wall, state->walls) {
      if (wall->kind == AZ_WALL_NOTHING) continue;
      if (az_arc_circle_hits_wall(
              wall, circle_radius, start, spin_center, spin_angle,
              &spin_angle, position_out, normal_out)) {
        impact_out->type = AZ_IMP_WALL;
        impact_out->target.wall = wall;
      }
    }
  }
  // Doors:
  if (!(skip_types & AZ_IMPF_DOOR_INSIDE) ||
      !(skip_types & AZ_IMPF_DOOR_OUTSIDE)) {
    AZ_ARRAY_LOOP(door, state->doors) {
      if (door->kind == AZ_DOOR_NOTHING) continue;
      if (!(skip_types & AZ_IMPF_DOOR_INSIDE) &&
          az_arc_circle_hits_door_inside(
              door, circle_radius, start, spin_center, spin_angle,
              &spin_angle, position_out, normal_out)) {
        impact_out->type = AZ_IMP_DOOR_INSIDE;
        impact_out->target.door = door;
      }
      if (!(skip_types & AZ_IMPF_DOOR_OUTSIDE) &&
          az_arc_circle_hits_door_outside(
              door, circle_radius, start, spin_center, spin_angle,
              &spin_angle, position_out, normal_out)) {
        impact_out->type = AZ_IMP_DOOR_OUTSIDE;
        impact_out->target.door = door;
      }
    }
  }
  // Ship:
  if (!(skip_types & AZ_IMPF_SHIP) && skip_uid != AZ_SHIP_UID &&
      az_ship_is_present(&state->ship)) {
    if (az_arc_circle_hits_ship(
            &state->ship, circle_radius, start, spin_center, spin_angle,
            &spin_angle, position_out, normal_out)) {
      impact_out->type = AZ_IMP_SHIP;
    }
  }
  // Baddies:
  if (!(skip_types & AZ_IMPF_BADDIE)) {
    AZ_ARRAY_LOOP(baddie, state->baddies) {
      if (baddie->kind == AZ_BAD_NOTHING) continue;
      if (baddie->data->properties & AZ_BADF_INCORPOREAL) continue;
      if (baddie->uid == skip_uid) continue;
      const az_component_data_t *component;
      if (az_arc_circle_hits_baddie(
              baddie, circle_radius, start, spin_center, spin_angle,
              &spin_angle, position_out, normal_out, &component)) {
        impact_out->type = AZ_IMP_BADDIE;
        impact_out->target.baddie.baddie = baddie;
        impact_out->target.baddie.component = component;
      }
    }
  }

  impact_out->angle = spin_angle;
  if (impact_out->type == AZ_IMP_NOTHING) {
    *position_out = az_vadd(spin_center,
                            az_vrotate(az_vsub(start, spin_center),
                                       spin_angle));
    *normal_out = AZ_VZERO;
  }
}

/*===========================================================================*/
