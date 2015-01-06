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

#include "azimuth/tick/baddie_nocturne.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>

#include "azimuth/state/baddie.h"
#include "azimuth/state/space.h"
#include "azimuth/tick/baddie_util.h"
#include "azimuth/tick/object.h"
#include "azimuth/util/audio.h"
#include "azimuth/util/random.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

// For the Nocturne, baddie->param stores opacity (0.0 is fully phased out, and
// 1.0 is fully phased in).  When baddie->state == 0, we will also have
// baddie->param == 0.0, and the Nocturne will be incorporeal and invisible.

#define PHASED_OUT_STATE 0
#define ROAM_AROUND_STATE 1
#define PHASING_OUT_STATE 2

#define MAX_MINIONS_TO_PHASE_IN 2
#define MIN_MINIONS_TO_PHASE_OUT 6

// How long it takes (in seconds) to phase in/out:
#define PHASE_IN_TIME 1.2
#define PHASE_OUT_TIME 0.8

// If baddie->param is less than this, the Nocturne is incorporeal:
#define MIN_PARAM_TO_BE_CORPOREAL 0.6

/*===========================================================================*/

static int get_num_minions(const az_space_state_t *state,
                           const az_baddie_t *master) {
  int num_minions = 0;
  AZ_ARRAY_LOOP(baddie, state->baddies) {
    if (baddie->kind == AZ_BAD_NOTHING) continue;
    if (baddie == master) continue;
    if (az_baddie_has_flag(baddie, AZ_BADF_INCORPOREAL)) continue;
    ++num_minions;
  }
  return num_minions;
}

static bool spot_is_clear(az_space_state_t *state, az_vector_t position) {
  const double radius =
    az_get_baddie_data(AZ_BAD_NOCTURNE)->overall_bounding_radius;
  az_impact_t impact;
  az_circle_impact(state, radius, position, AZ_VZERO,
                   AZ_IMPF_BADDIE, AZ_NULL_UID, &impact);
  return (impact.type == AZ_IMP_NOTHING);
}

/*===========================================================================*/

void az_tick_bad_nocturne(az_space_state_t *state, az_baddie_t *baddie,
                          double time) {
  assert(baddie->kind == AZ_BAD_NOCTURNE);
  assert(baddie->param >= 0.0);
  assert(baddie->param <= 1.0);
  if (baddie->param < MIN_PARAM_TO_BE_CORPOREAL) {
    baddie->temp_properties |=
      AZ_BADF_INCORPOREAL | AZ_BADF_NO_HOMING_BEAM | AZ_BADF_NO_HOMING_PROJ;
  }
  const double hurt = 1.0 - baddie->health / baddie->data->max_health;
  switch (baddie->state) {
    case PHASED_OUT_STATE:
      assert(baddie->param == 0.0);
      assert(az_baddie_has_flag(baddie, AZ_BADF_INCORPOREAL));
      if (get_num_minions(state, baddie) > MAX_MINIONS_TO_PHASE_IN) {
        baddie->cooldown = 1.5;
      }
      if (baddie->cooldown <= 0.0) {
        // Try to teleport to a clear spot; if successful within a reasonable
        // number of tries, start phasing in (otherwise, we'll try again next
        // frame).
        const az_camera_bounds_t *bounds =
          &state->planet->rooms[state->ship.player.current_room].camera_bounds;
        const double camera_r = az_vnorm(state->camera.center);
        const double min_r =
          fmax(bounds->min_r, camera_r - AZ_SCREEN_HEIGHT/2);
        const double max_r =
          fmin(bounds->min_r + bounds->r_span, camera_r + AZ_SCREEN_HEIGHT/2);
        assert(min_r <= max_r);
        const double camera_theta = az_vtheta(state->camera.center);
        const double theta_step =
          (camera_r > 0.0 ? atan((AZ_SCREEN_WIDTH/2) / camera_r) : AZ_PI);
        assert(theta_step > 0.0);
        const double min_theta = bounds->min_theta +
          fmax(az_mod2pi(camera_theta - theta_step - bounds->min_theta), 0.0);
        const double max_theta = bounds->min_theta +
          fmin(az_mod2pi(camera_theta + theta_step - bounds->min_theta),
               bounds->theta_span);
        assert(min_theta <= max_theta);
        for (int i = 0; i < 20; ++i) {
          const az_vector_t position =
            az_vpolar(az_random(min_r, max_r),
                      az_random(min_theta, max_theta));
          if (spot_is_clear(state, position)) {
            baddie->position = position;
            baddie->angle = az_random(-AZ_PI, AZ_PI);
            baddie->state = ROAM_AROUND_STATE;
            break;
          }
        }
      }
      break;
    case ROAM_AROUND_STATE:
      // Phase in:
      baddie->param = fmin(1.0, baddie->param + time / PHASE_IN_TIME);
      // Fly around:
      if (az_ship_is_decloaked(&state->ship)) {
        // TODO: Actually fly around; don't just turn towards ship.
        baddie->angle = az_angle_towards(
            baddie->angle, AZ_DEG2RAD(60) * time,
            az_vtheta(az_vsub(state->ship.position, baddie->position)));
      }
      if (baddie->param == 1.0 && baddie->cooldown <= 0.0) {
        // If we've spawned enough minions, start phasing out.
        if (get_num_minions(state, baddie) >= MIN_MINIONS_TO_PHASE_OUT) {
          baddie->state = PHASING_OUT_STATE;
          if (hurt >= 0.20) {
            for (int i = 0; i < 360; i += 36) {
              az_fire_baddie_projectile(
                  state, baddie, AZ_PROJ_BOUNCING_FIREBALL,
                  20, AZ_DEG2RAD(i), 0);
            }
            az_play_sound(&state->soundboard, AZ_SND_EXPLODE_FIREBALL_LARGE);
          }
        } else {
          // Otherwise, fire some seeds.
          // TODO: Fire other kinds of seeds too (chomper plants?).
          az_fire_baddie_projectile(state, baddie, AZ_PROJ_NIGHTSEED,
                                    45, 0, 0);
          baddie->cooldown = 0.5;
          // TODO: Attacks should get more intense as the boss takes damage.
          // TODO: Should have other attacks too.  One possibility: we should
          // grab the ship with a tongue and start pulling it in, and whack it
          // with our arms when the ship gets close.  If the ship stays away
          // long enough, give up.
        }
      }
      break;
    case PHASING_OUT_STATE:
      // Phase out:
      baddie->param = fmax(0.0, baddie->param - time / PHASE_OUT_TIME);
      if (baddie->param == 0.0) {
        baddie->state = PHASED_OUT_STATE;
      }
      break;
    default:
      baddie->state = ROAM_AROUND_STATE;
      break;
  }
}

/*===========================================================================*/
