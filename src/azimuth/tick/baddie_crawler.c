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

#include "azimuth/tick/baddie_crawler.h"

#include <assert.h>
#include <math.h>

#include "azimuth/state/baddie.h"
#include "azimuth/state/space.h"
#include "azimuth/tick/baddie_util.h"
#include "azimuth/tick/object.h"
#include "azimuth/util/random.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

void az_tick_bad_cave_crawler(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_CAVE_CRAWLER);
  az_crawl_around(state, baddie, time, true, 3.0, 40.0, 100.0);
}

void az_tick_bad_spined_crawler(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_SPINED_CRAWLER);
  if (baddie->state == 0) {
    baddie->param = baddie->angle;
    baddie->state = az_randint(1, 2);
  }
  if (baddie->state == 1 || baddie->state == 2) {
    const double angle = fabs(az_mod2pi(baddie->angle - baddie->param));
    az_crawl_around(state, baddie, time, (baddie->state == 1), 3.0,
                    50.0 - 40.0 * fmin(1.0, angle / AZ_HALF_PI), 100.0);
    if (angle > AZ_DEG2RAD(80) && baddie->cooldown <= 0.0) {
      baddie->state = (baddie->state == 1 ? 2 : 1);
      baddie->cooldown = 1.0;
    }
    if (baddie->cooldown <= 0.0 && az_ship_in_range(state, baddie, 90) &&
        az_can_see_ship(state, baddie)) {
      const az_vector_t center =
        az_vadd(baddie->position, az_vpolar(-13, baddie->angle));
      for (int i = -2; i <= 2; ++i) {
        const double theta = baddie->angle + i * AZ_DEG2RAD(41);
        az_add_projectile(
            state, AZ_PROJ_STINGER,
            az_vadd(center, az_vpolar(10.0, theta)),
            theta + az_random(-1, 1) * AZ_DEG2RAD(20), 1.0, baddie->uid);
      }
      az_play_sound(&state->soundboard, AZ_SND_FIRE_STINGER);
      baddie->velocity = AZ_VZERO;
      baddie->state = 3;
      baddie->cooldown = 2.5;
    }
  } else if (baddie->state == 3) {
    baddie->velocity = AZ_VZERO;
    if (baddie->cooldown <= 0.0) baddie->state = az_randint(1, 2);
  } else baddie->state = 0;
}

void az_tick_bad_crab_crawler(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_CRAB_CRAWLER);
  const double claw_max_angle = AZ_DEG2RAD(90);
  const double claw_open_time = 0.75;
  const double claw_open_rate = claw_max_angle / claw_open_time;
  const double claw_close_rate = claw_max_angle / 0.1;
  bool open_claws = true;
  if (baddie->state == 1 || baddie->state == 2) {
    az_crawl_around(state, baddie, time, (baddie->state == 1),
                    3.0, 25.0, 100.0);
    const az_vector_t rel_ship_position =
      az_vsub(state->ship.position, baddie->position);
    // Fire at the ship when the ship is facing away:
    if (baddie->cooldown <= 0.0 && az_ship_in_range(state, baddie, 200.0) &&
        fabs(az_mod2pi(state->ship.angle -
                       az_vtheta(rel_ship_position))) < AZ_DEG2RAD(60) &&
        az_can_see_ship(state, baddie)) {
      double fire_angle = az_vtheta(rel_ship_position);
      az_vector_t rel_impact_position;
      if (az_lead_target(rel_ship_position, state->ship.velocity,
                         550.0, &rel_impact_position)) {
        fire_angle = az_vtheta(rel_impact_position);
      }
      az_fire_baddie_projectile(state, baddie, AZ_PROJ_FIREBALL_FAST,
                                5.0, fire_angle - baddie->angle, 0.0);
      az_play_sound(&state->soundboard, AZ_SND_FIRE_FIREBALL);
      baddie->cooldown = 1.8;
    }
    // Reverse direction if we get near a marker node:
    AZ_ARRAY_LOOP(node, state->nodes) {
      if (node->kind != AZ_NODE_MARKER) continue;
      if (!az_vwithin(node->position, baddie->position, 50.0)) continue;
      const double rel_angle =
        az_mod2pi(az_vtheta(az_vsub(node->position, baddie->position)) -
                  baddie->angle);
      if (baddie->state == 1 && rel_angle < 0.0) baddie->state = 2;
      else if (baddie->state == 2 && rel_angle > 0.0) baddie->state = 1;
    }
  } else if (baddie->state == 3) {
    if (baddie->cooldown <= 0.0) baddie->state = az_randint(1, 2);
    else if (baddie->cooldown > claw_open_time) open_claws = false;
  } else {
    baddie->state = az_randint(1, 2);
  }
  // Open/close claws:
  const double old_claw_angle = baddie->components[1].angle;
  const double new_claw_angle =
    (open_claws ?
     az_angle_towards(old_claw_angle, claw_open_rate * time, 0.0) :
     az_angle_towards(old_claw_angle, claw_close_rate * time, claw_max_angle));
  baddie->components[0].angle = -new_claw_angle;
  baddie->components[1].angle = new_claw_angle;
}

void az_on_crab_crawler_damaged(
    az_space_state_t *state, az_baddie_t *baddie, double amount,
    az_damage_flags_t damage_kind) {
  assert(baddie->kind == AZ_BAD_CRAB_CRAWLER);
  baddie->velocity = AZ_VZERO;
  baddie->state = 3;
  baddie->cooldown = 2.5;
}

void az_tick_bad_ice_crawler(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_ICE_CRAWLER);
  az_crawl_around(state, baddie, time, false, 3.0, 30.0, 100.0);
}

void az_tick_bad_fire_crawler(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_FIRE_CRAWLER);
  az_crawl_around(state, baddie, time, true, 3.0, 40.0, 100.0);
  if (baddie->state <= 0) {
    baddie->state = az_randint(2, 5);
    baddie->cooldown = 2.0;
  } else if (baddie->cooldown <= 0.0 &&
             az_ship_in_range(state, baddie, 300) &&
             az_can_see_ship(state, baddie)) {
    az_fire_baddie_projectile(
        state, baddie, AZ_PROJ_FIREBALL_SLOW, 0.0, 0.0,
        az_vtheta(az_vsub(state->ship.position, baddie->position)) +
        AZ_DEG2RAD(5) * az_random(-1, 1) - baddie->angle);
    az_play_sound(&state->soundboard, AZ_SND_FIRE_FIREBALL);
    baddie->cooldown = 0.1;
    --baddie->state;
  }
}

void az_tick_bad_jungle_crawler(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_JUNGLE_CRAWLER);
  az_crawl_around(state, baddie, time, false, 3.0, 40.0, 100.0);
}

void az_on_jungle_crawler_killed(
    az_space_state_t *state, az_vector_t position, double angle) {
  for (int i = -100; i <= 100; i += 20) {
    const double theta = az_mod2pi(angle + AZ_DEG2RAD(i));
    az_add_projectile(state, AZ_PROJ_SPINE,
                      az_vadd(az_vadd(position, az_vpolar(-6, angle)),
                              az_vpolar(18, theta)),
                      theta, 1.0, AZ_NULL_UID);
  }
}

/*===========================================================================*/
