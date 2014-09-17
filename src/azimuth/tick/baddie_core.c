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

#include "azimuth/tick/baddie_core.h"

#include <assert.h>
#include <math.h>

#include "azimuth/state/baddie.h"
#include "azimuth/state/sound.h"
#include "azimuth/state/space.h"
#include "azimuth/tick/baddie_util.h"
#include "azimuth/tick/object.h"
#include "azimuth/util/random.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

#define INITIAL_STATE 0
#define START_DORMANCY_STATE 1
#define DORMANT_STATE 2
#define RAINBOW_BEAM_FORWARD_STATE 3
#define RAINBOW_BEAM_BACKWARD_STATE 4

/*===========================================================================*/

static void fire_rainbow_beam(az_space_state_t *state, az_baddie_t *baddie,
                              double angle_offset, double time) {
  // Fire a beam, piercing through the ship and other baddies.
  const double beam_angle = baddie->angle + angle_offset;
  const az_vector_t beam_start =
    az_vadd(baddie->position, az_vpolar(120, beam_angle));
  az_impact_t impact;
  az_ray_impact(state, beam_start, az_vpolar(1000, beam_angle),
                (AZ_IMPF_BADDIE | AZ_IMPF_SHIP), baddie->uid, &impact);
  const az_vector_t beam_delta = az_vsub(impact.position, beam_start);
  const double beam_damage = 200.0 * time;
  // Damage the ship and any baddies within the beam.
  if (az_ship_is_alive(&state->ship) &&
      az_ray_hits_ship(&state->ship, beam_start, beam_delta,
                       NULL, NULL)) {
    az_damage_ship(state, beam_damage, false);
    az_loop_sound(&state->soundboard, AZ_SND_BEAM_NORMAL);
    az_loop_sound(&state->soundboard, AZ_SND_BEAM_PHASE);
  }
  AZ_ARRAY_LOOP(other, state->baddies) {
    if (other->kind == AZ_BAD_NOTHING || other == baddie) continue;
    const az_component_data_t *component;
    if (az_ray_hits_baddie(other, beam_start, beam_delta,
                           NULL, NULL, &component)) {
      az_try_damage_baddie(state, other, component,
                           (AZ_DMGF_HYPER_ROCKET | AZ_DMGF_BEAM), beam_damage);
    }
  }
  // Add particles for the beam.
  const az_color_t beam_color = {
    (az_clock_mod(6, 1, state->clock) < 3 ? 255 : 64),
    (az_clock_mod(6, 1, state->clock + 2) < 3 ? 255 : 64),
    (az_clock_mod(6, 1, state->clock + 4) < 3 ? 255 : 64), 192};
  az_add_beam(state, beam_color, beam_start, impact.position, 0.0,
              6 + az_clock_zigzag(6, 1, state->clock));
  for (int i = 0; i < 5; ++i) {
    az_add_speck(state, beam_color, 1.0, impact.position,
                 az_vpolar(az_random(20.0, 70.0),
                           az_vtheta(impact.normal) +
                           az_random(-AZ_HALF_PI, AZ_HALF_PI)));
  }
}

void az_tick_bad_zenith_core(az_space_state_t *state, az_baddie_t *baddie,
                             double time) {
  assert(baddie->kind == AZ_BAD_ZENITH_CORE);
  switch (baddie->state) {
    case INITIAL_STATE:
      // Do nothing yet.
      break;
    case START_DORMANCY_STATE:
      baddie->cooldown = 10.0;
      baddie->state = DORMANT_STATE;
      break;
    case DORMANT_STATE:
      if (baddie->cooldown <= 0.0) {
        baddie->cooldown = 1.0;
        baddie->state = RAINBOW_BEAM_FORWARD_STATE;
      }
      break;
    case RAINBOW_BEAM_FORWARD_STATE:
      baddie->angle = az_angle_towards(
          baddie->angle, AZ_DEG2RAD(30) * time,
          az_vtheta(az_vsub(state->ship.position, baddie->position)));
      fire_rainbow_beam(state, baddie, 0, time);
      break;
    case RAINBOW_BEAM_BACKWARD_STATE:
      baddie->angle = az_angle_towards(
          baddie->angle, AZ_DEG2RAD(30) * time,
          az_vtheta(az_vsub(state->ship.position, baddie->position)));
      fire_rainbow_beam(state, baddie, AZ_PI, time);
      break;
    default:
      baddie->state = DORMANT_STATE;
      break;
  }
}

/*===========================================================================*/
