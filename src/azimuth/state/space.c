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
  AZ_ARRAY_LOOP(baddie, state->baddies) baddie->kind = AZ_BAD_NOTHING;
  AZ_ARRAY_LOOP(door, state->doors) door->kind = AZ_DOOR_NOTHING;
  AZ_ARRAY_LOOP(node, state->nodes) node->kind = AZ_NODE_NOTHING;
  AZ_ARRAY_LOOP(particle, state->particles) particle->kind = AZ_PAR_NOTHING;
  AZ_ARRAY_LOOP(pickup, state->pickups) pickup->kind = AZ_PUP_NOTHING;
  AZ_ARRAY_LOOP(proj, state->projectiles) proj->kind = AZ_PROJ_NOTHING;
  AZ_ARRAY_LOOP(wall, state->walls) wall->kind = AZ_WALL_NOTHING;
}

void az_enter_room(az_space_state_t *state, const az_room_t *room) {
  for (int i = 0; i < room->num_walls; ++i) {
    az_wall_t *wall;
    if (az_insert_wall(state, &wall)) {
      *wall = room->walls[i];
    }
  }
  for (int i = 0; i < room->num_baddies; ++i) {
    az_baddie_t *baddie;
    if (az_insert_baddie(state, &baddie)) {
      const az_baddie_spec_t *spec = &room->baddies[i];
      az_init_baddie(baddie, spec->kind, spec->position, spec->angle);
    }
  }
  for (int i = 0; i < room->num_doors; ++i) {
    az_door_t *door;
    if (az_insert_door(state, &door)) {
      const az_door_spec_t *spec = &room->doors[i];
      door->kind = spec->kind;
      door->position = spec->position;
      door->angle = spec->angle;
      door->destination = spec->destination;
    }
  }
  for (int i = 0; i < room->num_nodes; ++i) {
    az_node_t *node;
    if (az_insert_node(state, &node)) {
      const az_node_spec_t *spec = &room->nodes[i];
      node->kind = spec->kind;
      node->position = spec->position;
      node->angle = spec->angle;
    }
  }
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

bool az_insert_door(az_space_state_t *state, az_door_t **door_out) {
  AZ_ARRAY_LOOP(door, state->doors) {
    if (door->kind == AZ_DOOR_NOTHING) {
      door->is_open = false;
      door->openness = 0.0;
      *door_out = door;
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
  if (node->uid == uid) {
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

bool az_insert_projectile(az_space_state_t *state,
                          az_projectile_t **proj_out) {
  AZ_ARRAY_LOOP(proj, state->projectiles) {
    if (proj->kind == AZ_PROJ_NOTHING) {
      proj->age = 0.0;
      *proj_out = proj;
      return true;
    }
  }
  return false;
}

bool az_insert_wall(az_space_state_t *state, az_wall_t **wall_out) {
  AZ_ARRAY_LOOP(wall, state->walls) {
    if (wall->kind == AZ_WALL_NOTHING) {
      *wall_out = wall;
      return true;
    }
  }
  return false;
}

void az_try_add_pickup(az_space_state_t *state, az_pickup_kind_t kind,
                       az_vector_t position) {
  assert(kind != AZ_PUP_NOTHING);
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

void az_damage_ship(az_space_state_t *state, double damage) {
  state->ship.player.shields -= damage;
  if (state->ship.player.shields <= 0.0) {
    state->ship.player.shields = 0.0;
    // Put us into game-over mode:
    state->mode = AZ_MODE_GAME_OVER;
    state->mode_data.game_over.step = AZ_GOS_ASPLODE;
    state->mode_data.game_over.progress = 0.0;
    // Add particles for ship debris:
    az_particle_t *particle;
    if (az_insert_particle(state, &particle)) {
      particle->kind = AZ_PAR_BOOM;
      particle->color = AZ_WHITE;
      particle->position = state->ship.position;
      particle->velocity = AZ_VZERO;
      particle->lifetime = 0.5;
      particle->param1 = 30;
    }
    for (int i = 0; i < 20; ++i) {
      if (az_insert_particle(state, &particle)) {
        particle->kind = AZ_PAR_SPECK;
        particle->color = AZ_WHITE;
        particle->position = state->ship.position;
        particle->velocity = az_vpolar(20.0 + 50.0 * az_random(),
                                       az_random() * AZ_TWO_PI);
        particle->angle = 0.0;
        particle->lifetime = 2.0;
      } else break;
    }
  }
}

/*===========================================================================*/
