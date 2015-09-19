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

#include "azimuth/tick/door.h"

#include <assert.h>
#include <math.h>

#include "azimuth/state/door.h"
#include "azimuth/state/space.h"
#include "azimuth/tick/script.h"
#include "azimuth/util/audio.h"
#include "azimuth/util/misc.h"

/*===========================================================================*/

void az_try_open_door(az_space_state_t *state, az_door_t *door,
                      az_damage_flags_t damage_kind) {
  assert(door->kind != AZ_DOOR_NOTHING);
  if (!door->is_open && az_can_open_door(door->kind, damage_kind)) {
    assert(door->kind != AZ_DOOR_PASSAGE);
    assert(door->kind != AZ_DOOR_ALWAYS_OPEN);
    door->is_open = true;
    az_play_sound(&state->soundboard, AZ_SND_DOOR_OPEN);
    az_schedule_script(state, door->on_open);
  }
}

/*===========================================================================*/

// How long a door takes to open/close, in seconds:
#define DOOR_OPEN_TIME 0.5

static void tick_door(az_space_state_t *state, az_door_t *door, double time) {
  const double delta = (1.0 / DOOR_OPEN_TIME) * time;
  if (door->is_open) {
    door->openness = fmin(1.0, door->openness + delta);
  } else {
    assert(door->kind != AZ_DOOR_ALWAYS_OPEN);
    const double old_openness = door->openness;
    door->openness = fmax(0.0, door->openness - delta);
    // Don't allow the door to completely close while the ship is still in it;
    // if the reduction to door->openness we just made causes the door
    // collision functions to count the ship as suddently now intersecting the
    // door, then revert the change we just made to door->openness.
    if (az_ship_is_alive(&state->ship) &&
        az_circle_touches_door_outside(door, AZ_SHIP_DEFLECTOR_RADIUS,
                                       state->ship.position)) {
      door->openness = old_openness;
    }
  }
  if (door->kind == AZ_DOOR_LOCKED && !door->is_open) {
    door->lockedness = fmin(1.0, door->lockedness + delta);
  } else {
    door->lockedness = fmax(0.0, door->lockedness - delta);
  }
}

void az_tick_doors(az_space_state_t *state, double time) {
  AZ_ARRAY_LOOP(door, state->doors) {
    if (door->kind == AZ_DOOR_NOTHING) continue;
    if (door->kind == AZ_DOOR_PASSAGE) continue;
    tick_door(state, door, time);
  }
}

/*===========================================================================*/
