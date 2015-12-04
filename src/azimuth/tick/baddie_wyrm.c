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

#include "azimuth/tick/baddie_wyrm.h"

#include <assert.h>
#include <math.h>

#include "azimuth/state/baddie.h"
#include "azimuth/state/space.h"
#include "azimuth/tick/baddie_util.h"
#include "azimuth/tick/object.h"
#include "azimuth/util/audio.h"
#include "azimuth/util/random.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

#define ROCKWYRM_STATE_NORMAL 0
#define ROCKWYRM_STATE_SCREAM 1
#define ROCKWYRM_STATE_EGGS 2
#define ROCKWYRM_STATE_AIM_SPRAY 21

#define EGG_STATE_WAIT_FOR_SHIP 0
#define EGG_STATE_HATCH_AFTER_COOLDOWN 1

/*===========================================================================*/

void az_rockwyrm_move_jaws(az_baddie_t *baddie, double time, bool open) {
  assert(baddie->kind == AZ_BAD_ROCKWYRM);
  if (open) {
    const double limit = AZ_DEG2RAD(45);
    const double delta = AZ_DEG2RAD(180) * time;
    baddie->components[0].angle =
      fmin(limit, baddie->components[0].angle + delta);
    baddie->components[1].angle =
      fmax(-limit, baddie->components[1].angle - delta);
  } else {
    const double delta = AZ_DEG2RAD(45) * time;
    baddie->components[0].angle =
      fmax(0, baddie->components[0].angle - delta);
    baddie->components[1].angle =
      fmin(0, baddie->components[1].angle + delta);
  }
}

void az_tick_bad_rockwyrm(az_space_state_t *state, az_baddie_t *baddie,
                          double time) {
  assert(baddie->kind == AZ_BAD_ROCKWYRM);
  const double hurt = 1.0 - baddie->health / baddie->data->max_health;
  // Open/close jaws:
  az_rockwyrm_move_jaws(baddie, time,
                        (baddie->state == ROCKWYRM_STATE_SCREAM ||
                         (baddie->cooldown < 0.5 &&
                          baddie->state != ROCKWYRM_STATE_EGGS)));
  // State NORMAL: Shoot a spread of bullets:
  if (baddie->state == ROCKWYRM_STATE_NORMAL) {
    if (baddie->cooldown <= 0.0 &&
        az_ship_within_angle(state, baddie, 0, AZ_DEG2RAD(20)) &&
        az_can_see_ship(state, baddie)) {
      if (hurt >= 0.35 && az_random(0, 1) < 0.25) {
        int num_wyrmlings = 0;
        AZ_ARRAY_LOOP(other, state->baddies) {
          if (other->kind == AZ_BAD_WYRMLING) ++num_wyrmlings;
        }
        if (num_wyrmlings <= 6) {
          az_play_sound(&state->soundboard, AZ_SND_ROCKWYRM_SCREAM);
          baddie->state = ROCKWYRM_STATE_SCREAM;
          baddie->cooldown = 1.0;
        }
      }
      if (baddie->state != ROCKWYRM_STATE_SCREAM) {
        // Fire bullets.  The lower our health, the more bullets we fire.
        const int num_projectiles = 3 + 2 * (int)floor(4 * hurt);
        double theta = 0.0;
        for (int i = 0; i < num_projectiles; ++i) {
          az_fire_baddie_projectile(
              state, baddie, AZ_PROJ_STINGER,
              baddie->data->main_body.bounding_radius, 0.0, theta);
          az_play_sound(&state->soundboard, AZ_SND_FIRE_STINGER);
          theta = -theta;
          if (i % 2 == 0) theta += AZ_DEG2RAD(10);
        }
        // Below 35% health, we have a 20% chance to go to state 20 (firing a
        // long spray of bullets).
        if (hurt >= 0.65 && az_random(0, 1) < 0.2) {
          baddie->state = ROCKWYRM_STATE_AIM_SPRAY;
          baddie->cooldown = 3.0;
        }
        // Otherwise, we have a 50/50 chance to stay in state 0, or to go to
        // state 2 (drop eggs).
        else if (az_random(0, 1) < 0.5) {
          baddie->state = ROCKWYRM_STATE_EGGS;
          baddie->cooldown = 1.0;
        } else baddie->cooldown = az_random(2.0, 4.0);
      }
    }
  }
  // State SCREAM: Scream and summon more wyrmlings:
  else if (baddie->state == ROCKWYRM_STATE_SCREAM) {
    az_shake_camera(&state->camera, 3.5, 7.0);
    if (baddie->cooldown <= 0.0) {
      AZ_ARRAY_LOOP(node, state->nodes) {
        if (node->kind != AZ_NODE_MARKER) continue;
        if (az_random(0, 1) > hurt) continue;
        az_add_baddie(state, AZ_BAD_WYRMLING, node->position,
                      az_vtheta(az_vsub(baddie->position, node->position)));
      }
      baddie->state = ROCKWYRM_STATE_NORMAL;
      baddie->cooldown = az_random(3.0, 5.0);
    }
  }
  // State EGGS: Drop eggs:
  else if (baddie->state == ROCKWYRM_STATE_EGGS) {
    if (baddie->cooldown <= 0.0) {
      // The lower our health, the more eggs we drop at once.
      const int num_eggs = 1 + (int)floor(4 * hurt);
      const double spread = AZ_DEG2RAD(num_eggs * 10);
      const az_component_t *tail =
        &baddie->components[baddie->data->num_components - 1];
      for (int i = 0; i < num_eggs; ++i) {
        az_baddie_t *egg = az_add_baddie(
            state, AZ_BAD_WYRM_EGG,
            az_vadd(baddie->position,
                    az_vrotate(tail->position, baddie->angle)),
            baddie->angle + tail->angle + AZ_PI + az_random(-spread, spread));
        if (egg != NULL) {
          egg->velocity = az_vpolar(az_random(50, 150), egg->angle);
          // Set the egg to hatch (without waiting for the ship to get
          // close first) after a random amount of time.
          egg->state = EGG_STATE_HATCH_AFTER_COOLDOWN;
          egg->cooldown = az_random(2.0, 2.5);
        } else break;
      }
      baddie->state = ROCKWYRM_STATE_NORMAL;
      baddie->cooldown = az_random(1.0, 3.0);
    }
  }
  // State AIM_SPRAY: Wait until we have line of sight:
  else if (baddie->state == ROCKWYRM_STATE_AIM_SPRAY) {
    if (baddie->cooldown <= 0.0 &&
        az_ship_within_angle(state, baddie, 0, AZ_DEG2RAD(8)) &&
        az_can_see_ship(state, baddie)) {
      --baddie->state;
    }
  }
  // Between states EGGS and AIM_SPRAY: Fire a steady spray of bullets:
  else if (ROCKWYRM_STATE_EGGS < baddie->state &&
           baddie->state < ROCKWYRM_STATE_AIM_SPRAY) {
    if (baddie->cooldown <= 0.0) {
      for (int i = -1; i <= 1; ++i) {
        az_fire_baddie_projectile(
            state, baddie, AZ_PROJ_STINGER,
            baddie->data->main_body.bounding_radius, 0.0,
            AZ_DEG2RAD(i * az_random(0, 10)));
        az_play_sound(&state->soundboard, AZ_SND_FIRE_STINGER);
      }
      baddie->cooldown = 0.1;
      --baddie->state;
    }
  } else baddie->state = ROCKWYRM_STATE_NORMAL;
  // Chase ship; get slightly slower as we get hurt.
  az_snake_towards(state, baddie, time, 2, 130.0 - 10.0 * hurt, 50.0,
                   state->ship.position, true);
}

/*===========================================================================*/

void az_tick_bad_wyrm_egg(az_space_state_t *state, az_baddie_t *baddie,
                          double time) {
  // Apply drag to the egg (if it's moving).
  if (az_vnonzero(baddie->velocity)) {
    const double drag_coeff = 1.0 / 400.0;
    const az_vector_t drag_force =
      az_vmul(baddie->velocity, -drag_coeff * az_vnorm(baddie->velocity));
    baddie->velocity =
      az_vadd(baddie->velocity, az_vmul(drag_force, time));
  }
  if (baddie->state == EGG_STATE_WAIT_FOR_SHIP) {
    if (az_ship_in_range(state, baddie, 200) &&
        az_can_see_ship(state, baddie)) {
      baddie->state = EGG_STATE_HATCH_AFTER_COOLDOWN;
      baddie->cooldown = 2.0;
    }
  } else {
    if (baddie->cooldown <= 0.0) {
      const az_vector_t position = baddie->position;
      const double angle = baddie->angle;
      az_kill_baddie(state, baddie);
      az_init_baddie(baddie, AZ_BAD_WYRMLING, position, angle);
    }
  }
}

void az_tick_bad_wyrmling(az_space_state_t *state, az_baddie_t *baddie,
                          double time) {
  az_snake_towards(state, baddie, time, 0, 180.0, 120.0, state->ship.position,
                   true);
}

/*===========================================================================*/
