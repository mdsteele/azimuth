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

#include "azimuth/tick/baddie_bouncer.h"

#include <assert.h>
#include <math.h>

#include "azimuth/state/baddie.h"
#include "azimuth/state/space.h"
#include "azimuth/tick/baddie_util.h"
#include "azimuth/tick/object.h"
#include "azimuth/util/audio.h"
#include "azimuth/util/bezier.h"
#include "azimuth/util/random.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

static bool times_per_second(const az_space_state_t *state, double per_second,
                             double time) {
  return (ceil(per_second * state->ship.player.total_time) >
          ceil(per_second * (state->ship.player.total_time - time)));
}

/*===========================================================================*/

void az_tick_bad_bouncer(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_BOUNCER);
  if (az_vnonzero(baddie->velocity)) {
    baddie->angle = az_vtheta(baddie->velocity);
  }
  baddie->velocity = az_vpolar(150.0, baddie->angle);
}

void az_tick_bad_bouncer_90(
    az_space_state_t *state, az_baddie_t *baddie, bool bounced) {
  assert(baddie->kind == AZ_BAD_BOUNCER_90);
  if (az_vnonzero(baddie->velocity)) {
    if (bounced) {
      baddie->velocity =
        az_vflatten(baddie->velocity, az_vpolar(1, baddie->angle));
    }
    baddie->angle = az_vtheta(baddie->velocity);
  }
  baddie->velocity = az_vpolar(170.0, baddie->angle);
}

void az_tick_bad_fast_bouncer(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_FAST_BOUNCER);
  if (az_vnonzero(baddie->velocity)) {
    baddie->angle = az_vtheta(baddie->velocity);
  }
  baddie->velocity = az_vpolar(385.0, baddie->angle);
}

/*===========================================================================*/

void az_tick_bad_atom(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_ATOM);
  az_drift_towards_ship(state, baddie, time, 70, 100, 100);
  // Move electrons:
  for (int i = 0; i < baddie->data->num_components; ++i) {
    az_component_t *component = &baddie->components[i];
    component->angle = az_mod2pi(component->angle + 3.5 * time);
    component->position = az_vpolar(4.0, component->angle);
    component->position.x *= 5.0;
    component->position = az_vrotate(component->position, i * AZ_DEG2RAD(120));
  }
}

void az_tick_bad_red_atom(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_RED_ATOM);
  az_drift_towards_ship(state, baddie, time, 70, 100, 100);
  // Shoot exploding fireballs:
  if (baddie->cooldown <= 0.0 && az_ship_in_range(state, baddie, 300) &&
      az_can_see_ship(state, baddie)) {
    az_fire_baddie_projectile(
        state, baddie, AZ_PROJ_TRINE_TORPEDO_FIREBALL, 0.0,
        az_mod2pi(az_vtheta(az_vsub(state->ship.position, baddie->position)) -
                  baddie->angle),
        0.0);
    az_play_sound(&state->soundboard, AZ_SND_FIRE_FIREBALL);
    baddie->cooldown = 2.0;
  }
  // Move electrons, and have them leave a flame trail:
  for (int i = 0; i < baddie->data->num_components; ++i) {
    az_component_t *component = &baddie->components[i];
    az_particle_t *particle;
    if (times_per_second(state, 20, time) &&
        az_insert_particle(state, &particle)) {
      particle->kind = AZ_PAR_EMBER;
      particle->color = (az_color_t){255, 64, 0, 128};
      particle->position =
        az_vadd(az_vrotate(component->position, baddie->angle),
                baddie->position);
      particle->velocity = AZ_VZERO;
      particle->angle = baddie->angle + component->angle;
      particle->lifetime = 0.3;
      particle->param1 = 1.5 * baddie->data->components[i].bounding_radius;
    }
    component->angle = az_mod2pi(component->angle + 5.0 * time);
    component->position = az_vpolar(5.0, component->angle);
    component->position.x *= 5.0;
    component->position = az_vrotate(component->position, i * AZ_DEG2RAD(120));
  }
}

/*===========================================================================*/
