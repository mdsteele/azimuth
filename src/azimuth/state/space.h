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
#ifndef AZIMUTH_STATE_SPACE_H_
#define AZIMUTH_STATE_SPACE_H_

#include <stdbool.h>
#include <stdint.h>

#include "azimuth/state/baddie.h"
#include "azimuth/state/camera.h"
#include "azimuth/state/door.h"
#include "azimuth/state/gravfield.h"
#include "azimuth/state/node.h"
#include "azimuth/state/particle.h"
#include "azimuth/state/pickup.h"
#include "azimuth/state/planet.h"
#include "azimuth/state/player.h"
#include "azimuth/state/projectile.h"
#include "azimuth/state/room.h"
#include "azimuth/state/ship.h"
#include "azimuth/state/uid.h"
#include "azimuth/state/wall.h"
#include "azimuth/util/audio.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

typedef struct {
  const char *string; // not necessarily NUL-terminated
  unsigned int length; // length of string
  double time_remaining; // seconds
} az_message_t;

typedef struct {
  bool is_active;
  double active_for;
  double time_remaining; // seconds
} az_timer_t;

typedef struct {
  const az_planet_t *planet;
  int save_file_index;
  az_clock_t clock;
  az_camera_t camera;
  az_ship_t ship;
  az_message_t message;
  az_timer_t timer;
  az_soundboard_t soundboard;

  // Mode information:
  enum {
    AZ_MODE_NORMAL, // flying around; the normal mode of gameplay
    AZ_MODE_DOORWAY, // waiting while we pass through a door
    AZ_MODE_GAME_OVER, // showing the game over animation
    AZ_MODE_PAUSING, // entering the pause screen
    AZ_MODE_RESUMING, // leaving the pause screen
    AZ_MODE_SAVING, // using a save point
    AZ_MODE_UPGRADE // receiving an upgrade
  } mode;
  union {
    struct {
      enum { AZ_DWS_FADE_OUT, AZ_DWS_SHIFT, AZ_DWS_FADE_IN } step;
      double progress; // 0.0 to 1.0
      const az_door_t *door;
    } doorway;
    struct {
      enum { AZ_GOS_ASPLODE, AZ_GOS_FADE_OUT } step;
      double progress; // 0.0 to 1.0
    } game_over;
    struct {
      double progress; // 0.0 to 1.0
    } pause; // used for both PAUSING and RESUMING mode
    struct {
      enum { AZ_UGS_OPEN, AZ_UGS_MESSAGE, AZ_UGS_CLOSE } step;
      double progress; // 0.0 to 1.0
      az_upgrade_t upgrade;
    } upgrade;
  } mode_data;

  // Space objects (these all get cleared out when we exit a room):
  az_baddie_t baddies[100];
  az_door_t doors[20];
  az_gravfield_t gravfields[50];
  az_node_t nodes[50];
  az_particle_t particles[500];
  az_pickup_t pickups[100];
  az_projectile_t projectiles[250];
  az_wall_t walls[250];
  az_uuid_t uuids[AZ_NUM_UUID_SLOTS];
} az_space_state_t;

/*===========================================================================*/

// Remove all objects (baddies, doors, etc.), but leave other fields unchanged.
void az_clear_space(az_space_state_t *state);

// Add all room objects to the space state, on top of whatever objects are
// already there.  You may want to call az_clear_space first to ensure that
// there is room for the new objects.  Note that this function does not make
// any changes to the ship or any other fields.
void az_enter_room(az_space_state_t *state, const az_room_t *room);

bool az_lookup_baddie(az_space_state_t *state, az_uid_t uid,
                      az_baddie_t **baddie_out);
bool az_insert_baddie(az_space_state_t *state, az_baddie_t **baddie_out);

bool az_lookup_door(az_space_state_t *state, az_uid_t uid,
                    az_door_t **door_out);
bool az_insert_door(az_space_state_t *state, az_door_t **door_out);

bool az_insert_gravfield(az_space_state_t *state,
                         az_gravfield_t **gravfield_out);

bool az_lookup_node(az_space_state_t *state, az_uid_t uid,
                    az_node_t **node_out);
bool az_insert_node(az_space_state_t *state, az_node_t **node_out);

bool az_insert_particle(az_space_state_t *state, az_particle_t **particle_out);

void az_add_speck(az_space_state_t *state, az_color_t color, double lifetime,
                  az_vector_t position, az_vector_t velocity);

bool az_insert_projectile(az_space_state_t *state, az_projectile_t **proj_out);

bool az_insert_wall(az_space_state_t *state, az_wall_t **wall_out);

void az_add_random_pickup(az_space_state_t *state,
                          az_pickup_flags_t potential_pickups,
                          az_vector_t position);

/*===========================================================================*/

// Reduce the ship's shields by the given amount.  If this is enough to destroy
// the ship, change to game-over mode.
void az_damage_ship(az_space_state_t *state, double damage,
                    bool induce_temp_invincibility);

// Try to destroy the wall with the given kind of damage, and return true iff
// the wall was destroyed.
bool az_try_break_wall(az_space_state_t *state, az_wall_t *wall,
                       az_damage_flags_t damage_kind);

/*===========================================================================*/

typedef enum {
  AZ_IMP_NOTHING = 0,
  AZ_IMP_BADDIE,
  AZ_IMP_DOOR_INSIDE,
  AZ_IMP_DOOR_OUTSIDE,
  AZ_IMP_SHIP,
  AZ_IMP_WALL
} az_impact_type_t;

typedef uint_fast8_t az_impact_flags_t;
#define AZ_IMPF_BADDIE       ((az_impact_flags_t)(1u << AZ_IMP_BADDIE))
#define AZ_IMPF_DOOR_INSIDE  ((az_impact_flags_t)(1u << AZ_IMP_DOOR_INSIDE))
#define AZ_IMPF_DOOR_OUTSIDE ((az_impact_flags_t)(1u << AZ_IMP_DOOR_OUTSIDE))
#define AZ_IMPF_SHIP         ((az_impact_flags_t)(1u << AZ_IMP_SHIP))
#define AZ_IMPF_WALL         ((az_impact_flags_t)(1u << AZ_IMP_WALL))

typedef struct {
  az_impact_type_t type;
  union {
    struct {
      az_baddie_t *baddie;
      const az_component_data_t *component;
    } baddie;
    az_door_t *door;
    az_wall_t *wall;
  } target;
  double angle; // only used for the az_arc_*_impact functions
  az_vector_t position;
  // Normal vector.  For ray impacts, the length is not guaranteed, but for
  // circle impacts it will point from position to the point of contact.
  az_vector_t normal;
} az_impact_t;

void az_ray_impact(
    az_space_state_t *state, az_vector_t start, az_vector_t delta,
    az_impact_flags_t skip_types, az_uid_t skip_uid, az_impact_t *impact_out);

void az_circle_impact(
    az_space_state_t *state, double circle_radius,
    az_vector_t start, az_vector_t delta,
    az_impact_flags_t skip_types, az_uid_t skip_uid, az_impact_t *impact_out);

void az_arc_circle_impact(
    az_space_state_t *state, double circle_radius,
    az_vector_t start, az_vector_t spin_center, double spin_angle,
    az_impact_flags_t skip_types, az_uid_t skip_uid, az_impact_t *impact_out);

/*===========================================================================*/

#endif // AZIMUTH_STATE_SPACE_H_
