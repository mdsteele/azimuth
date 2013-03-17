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
#include "azimuth/util/misc.h"
#include "azimuth/util/random.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

void az_clear_space(az_space_state_t *state) {
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
  for (int i = 0; i < room->num_baddies; ++i) {
    az_baddie_t *baddie;
    if (az_insert_baddie(state, &baddie)) {
      const az_baddie_spec_t *spec = &room->baddies[i];
      az_init_baddie(baddie, spec->kind, spec->position, spec->angle);
      baddie->on_kill = spec->on_kill;
      put_uuid(state, spec->uuid_slot, AZ_UUID_BADDIE, baddie->uid);
    }
  }
  for (int i = 0; i < room->num_doors; ++i) {
    az_door_t *door;
    if (az_insert_door(state, &door)) {
      const az_door_spec_t *spec = &room->doors[i];
      door->kind = spec->kind;
      door->on_open = spec->on_open;
      door->position = spec->position;
      door->angle = spec->angle;
      door->destination = spec->destination;
      put_uuid(state, spec->uuid_slot, AZ_UUID_DOOR, door->uid);
    }
  }
  for (int i = 0; i < room->num_gravfields; ++i) {
    az_gravfield_t *gravfield;
    if (az_insert_gravfield(state, &gravfield)) {
      const az_gravfield_spec_t *spec = &room->gravfields[i];
      gravfield->kind = spec->kind;
      gravfield->position = spec->position;
      gravfield->angle = spec->angle;
      gravfield->strength = spec->strength;
      gravfield->size = spec->size;
      put_uuid(state, spec->uuid_slot, AZ_UUID_GRAVFIELD, gravfield->uid);
    }
  }
  for (int i = 0; i < room->num_nodes; ++i) {
    const az_node_spec_t *spec = &room->nodes[i];
    if (spec->kind == AZ_NODE_UPGRADE &&
        az_has_upgrade(&state->ship.player, spec->subkind.upgrade)) continue;
    az_node_t *node;
    if (az_insert_node(state, &node)) {
      node->kind = spec->kind;
      node->subkind = spec->subkind;
      node->on_use = spec->on_use;
      node->position = spec->position;
      node->angle = spec->angle;
    }
  }
  for (int i = 0; i < room->num_walls; ++i) {
    az_wall_t *wall;
    if (az_insert_wall(state, &wall)) {
      const az_wall_spec_t *spec = &room->walls[i];
      wall->kind = spec->kind;
      wall->data = spec->data;
      wall->position = spec->position;
      wall->angle = spec->angle;
      put_uuid(state, spec->uuid_slot, AZ_UUID_WALL, wall->uid);
    }
  }
}

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

bool az_insert_baddie(az_space_state_t *state, az_baddie_t **baddie_out) {
  AZ_ARRAY_LOOP(baddie, state->baddies) {
    if (baddie->kind == AZ_BAD_NOTHING) {
      az_assign_uid(baddie - state->baddies, &baddie->uid);
      *baddie_out = baddie;
      return true;
    }
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

bool az_insert_door(az_space_state_t *state, az_door_t **door_out) {
  AZ_ARRAY_LOOP(door, state->doors) {
    if (door->kind == AZ_DOOR_NOTHING) {
      az_assign_uid(door - state->doors, &door->uid);
      door->is_open = false;
      door->openness = 0.0;
      *door_out = door;
      return true;
    }
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

bool az_insert_gravfield(az_space_state_t *state,
                         az_gravfield_t **gravfield_out) {
  AZ_ARRAY_LOOP(gravfield, state->gravfields) {
    if (gravfield->kind == AZ_GRAV_NOTHING) {
      az_assign_uid(gravfield - state->gravfields, &gravfield->uid);
      *gravfield_out = gravfield;
      return true;
    }
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

bool az_insert_node(az_space_state_t *state, az_node_t **node_out) {
  AZ_ARRAY_LOOP(node, state->nodes) {
    if (node->kind == AZ_NODE_NOTHING) {
      az_assign_uid(node - state->nodes, &node->uid);
      *node_out = node;
      return true;
    }
  }
  return false;
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
  return false;
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
}

az_projectile_t *az_add_projectile(
    az_space_state_t *state, az_proj_kind_t kind, bool fired_by_enemy,
    az_vector_t position, double angle) {
  AZ_ARRAY_LOOP(proj, state->projectiles) {
    if (proj->kind == AZ_PROJ_NOTHING) {
      az_init_projectile(proj, kind, fired_by_enemy, position, angle);
      return proj;
    }
  }
  return NULL;
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

bool az_insert_wall(az_space_state_t *state, az_wall_t **wall_out) {
  AZ_ARRAY_LOOP(wall, state->walls) {
    if (wall->kind == AZ_WALL_NOTHING) {
      az_assign_uid(wall - state->walls, &wall->uid);
      wall->flare = 0.0;
      *wall_out = wall;
      return true;
    }
  }
  return false;
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
}

/*===========================================================================*/

void az_damage_ship(az_space_state_t *state, double damage,
                    bool induce_temp_invincibility) {
  az_ship_t *ship = &state->ship;
  assert(az_ship_is_present(ship));
  assert(damage >= 0.0);
  if (ship->temp_invincibility > 0.0) return;
  if (induce_temp_invincibility) ship->temp_invincibility = 1.0;
  if (damage <= 0.0) return;
  ship->shield_flare = 1.0;
  // If the ship can survive the damage, reduce shields and we're done.
  if (ship->player.shields > damage) {
    ship->player.shields -= damage;
    return;
  }
  // Otherwise, the ship will be destroyed.
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
  state->mode_data.game_over.step = AZ_GOS_ASPLODE;
  state->mode_data.game_over.progress = 0.0;
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
      vulnerability = AZ_DMGF_CHARGED;
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
    az_play_sound(&state->soundboard, AZ_SND_KILL_TURRET);
    // Place particles for wall debris.
    const double radius = wall->data->bounding_radius;
    const double step = 3.0 + radius / 10.0;
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
          particle->lifetime = az_random(0.5, 1.0);
          particle->param1 = az_random(1.0, 3.0) * (radius / 75.0);
          particle->param2 = az_random(-10.0, 10.0);
        }
      }
    }
    // Remove the wall.
    wall->kind = AZ_WALL_NOTHING;
    return true;
  }
  // Otherwise, flare the wall (to make visible to the player what kind of
  // damage would be able to destroy this wall).
  else {
    wall->flare = 1.0;
    return false;
  }
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
  az_vector_t *position = &impact_out->position;
  az_vector_t *normal = &impact_out->normal;

  // Walls:
  if (!(skip_types & AZ_IMPF_WALL)) {
    AZ_ARRAY_LOOP(wall, state->walls) {
      if (wall->kind == AZ_WALL_NOTHING) continue;
      if (az_circle_hits_wall(wall, radius, start, delta, position, normal)) {
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
          az_circle_hits_door_inside(door, radius, start, delta,
                                     position, normal)) {
        impact_out->type = AZ_IMP_DOOR_INSIDE;
        impact_out->target.door = door;
        delta = az_vsub(*position, start);
      }
      if (!(skip_types & AZ_IMPF_DOOR_OUTSIDE) &&
          az_circle_hits_door_outside(door, radius, start, delta,
                                      position, normal)) {
        impact_out->type = AZ_IMP_DOOR_OUTSIDE;
        impact_out->target.door = door;
        delta = az_vsub(*position, start);
      }
    }
  }
  // Ship:
  if (!(skip_types & AZ_IMPF_SHIP) && skip_uid != AZ_SHIP_UID &&
      az_ship_is_present(&state->ship)) {
    if (az_circle_hits_ship(&state->ship, radius, start, delta,
                            position, normal)) {
      impact_out->type = AZ_IMP_SHIP;
      delta = az_vsub(*position, start);
    }
  }
  // Baddies:
  if (!(skip_types & AZ_IMPF_BADDIE)) {
    AZ_ARRAY_LOOP(baddie, state->baddies) {
      if (baddie->kind == AZ_BAD_NOTHING) continue;
      if (baddie->uid == skip_uid) continue;
      const az_component_data_t *component;
      if (az_circle_hits_baddie(baddie, radius, start, delta,
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
  } else {
    *normal = az_vsub(*position, *normal);
  }
}

void az_arc_circle_impact(
    az_space_state_t *state, double circle_radius,
    az_vector_t start, az_vector_t spin_center, double spin_angle,
    az_impact_flags_t skip_types, az_uid_t skip_uid, az_impact_t *impact_out) {
  assert(impact_out != NULL);
  impact_out->type = AZ_IMP_NOTHING;
  az_vector_t *position = &impact_out->position;
  az_vector_t *normal = &impact_out->normal;

  // Walls:
  if (!(skip_types & AZ_IMPF_WALL)) {
    AZ_ARRAY_LOOP(wall, state->walls) {
      if (wall->kind == AZ_WALL_NOTHING) continue;
      if (az_arc_circle_hits_wall(wall, circle_radius, start, spin_center,
                                  spin_angle, &spin_angle, position, normal)) {
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
              &spin_angle, position, normal)) {
        impact_out->type = AZ_IMP_DOOR_INSIDE;
        impact_out->target.door = door;
      }
      if (!(skip_types & AZ_IMPF_DOOR_OUTSIDE) &&
          az_arc_circle_hits_door_outside(
              door, circle_radius, start, spin_center, spin_angle,
              &spin_angle, position, normal)) {
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
            &spin_angle, position, normal)) {
      impact_out->type = AZ_IMP_SHIP;
    }
  }
  // Baddies:
  if (!(skip_types & AZ_IMPF_BADDIE)) {
    AZ_ARRAY_LOOP(baddie, state->baddies) {
      if (baddie->kind == AZ_BAD_NOTHING) continue;
      if (baddie->uid == skip_uid) continue;
      const az_component_data_t *component;
      if (az_arc_circle_hits_baddie(
              baddie, circle_radius, start, spin_center, spin_angle,
              &spin_angle, position, normal, &component)) {
        impact_out->type = AZ_IMP_BADDIE;
        impact_out->target.baddie.baddie = baddie;
        impact_out->target.baddie.component = component;
      }
    }
  }

  impact_out->angle = spin_angle;
  if (impact_out->type == AZ_IMP_NOTHING) {
    *position = az_vadd(spin_center,
                        az_vrotate(az_vsub(start, spin_center), spin_angle));
    *normal = AZ_VZERO;
  } else {
    *normal = az_vsub(*position, *normal);
  }
}

/*===========================================================================*/
