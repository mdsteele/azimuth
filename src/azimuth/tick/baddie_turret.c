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

#include "azimuth/tick/baddie_turret.h"

#include <assert.h>
#include <math.h>

#include "azimuth/state/baddie.h"
#include "azimuth/state/space.h"
#include "azimuth/tick/baddie_util.h"
#include "azimuth/tick/object.h"
#include "azimuth/util/audio.h"
#include "azimuth/util/random.h"

/*===========================================================================*/

static void aim_towards_ship(
    az_space_state_t *state, az_baddie_t *baddie, double time,
    double max_angle, double turn_rate) {
  if (!az_ship_is_decloaked(&state->ship)) return;
  baddie->components[0].angle =
    fmax(-max_angle, fmin(max_angle, az_mod2pi(az_angle_towards(
      baddie->angle + baddie->components[0].angle, turn_rate * time,
      az_vtheta(az_vsub(state->ship.position, baddie->position))) -
                               baddie->angle)));
}

static void recenter_barrel(az_baddie_t *baddie, double time,
                            double turn_rate) {
  baddie->components[0].angle = az_angle_towards(
      baddie->components[0].angle, turn_rate * time, 0.0);
}

void az_tick_bad_turret(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_NORMAL_TURRET ||
         baddie->kind == AZ_BAD_ARMORED_TURRET);
  aim_towards_ship(state, baddie, time, AZ_DEG2RAD(57), AZ_DEG2RAD(120));
  // Fire:
  if (baddie->cooldown <= 0.0 &&
      az_ship_within_angle(state, baddie, baddie->components[0].angle,
                           AZ_DEG2RAD(6)) &&
      az_can_see_ship(state, baddie)) {
    az_fire_baddie_projectile(state, baddie, AZ_PROJ_LASER_PULSE, 20.0,
                              baddie->components[0].angle, 0.0);
    baddie->cooldown = 1.5;
    az_play_sound(&state->soundboard, AZ_SND_FIRE_LASER_PULSE);
  }
}

void az_tick_bad_beam_turret(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_BEAM_TURRET);
  const double max_angle = AZ_DEG2RAD(57);
  const double barrel_turn_rate = AZ_DEG2RAD(30);
  const bool can_see_ship = az_can_see_ship(state, baddie);
  if (can_see_ship || baddie->cooldown > 0.0) {
    aim_towards_ship(state, baddie, time, max_angle, barrel_turn_rate);
  } else recenter_barrel(baddie, time, barrel_turn_rate);
  // If we can see the ship, turn on the beam:
  if (can_see_ship &&
      az_ship_within_angle(state, baddie, baddie->components[0].angle,
                           AZ_DEG2RAD(20))) {
    baddie->cooldown = 0.5;
  }
  // See what the beam would hit:
  const double beam_theta = baddie->angle + baddie->components[0].angle;
  const az_vector_t beam_start =
    az_vadd(baddie->position, az_vpolar(30, beam_theta));
  az_impact_t impact;
  az_ray_impact(state, beam_start, az_vpolar(10000, beam_theta),
                (baddie->cooldown > 0.0 || az_ship_is_decloaked(&state->ship) ?
                 AZ_IMPF_NONE : AZ_IMPF_SHIP), baddie->uid, &impact);
  // If beam is still turned on, fire:
  if (baddie->cooldown > 0.0) {
    const double beam_damage = 40.0 * time;
    if (impact.type == AZ_IMP_BADDIE) {
      az_try_damage_baddie(state, impact.target.baddie.baddie,
                           impact.target.baddie.component, AZ_DMGF_BEAM,
                           beam_damage);
    } else if (impact.type == AZ_IMP_SHIP) {
      az_damage_ship(state, beam_damage, false);
    }
    // Add particles for the beam.
    const uint8_t alt = 32 * az_clock_zigzag(6, 1, state->clock);
    const az_color_t beam_color = {alt/2, 255, alt, 192};
    az_add_beam(state, beam_color, beam_start, impact.position, 0.0,
                2.0 + 0.5 * az_clock_zigzag(8, 1, state->clock));
    az_add_speck(state, AZ_WHITE, 1.0, impact.position,
                 az_vpolar(az_random(20.0, 70.0),
                           az_vtheta(impact.normal) +
                           az_random(-AZ_HALF_PI, AZ_HALF_PI)));
    az_loop_sound(&state->soundboard, AZ_SND_BEAM_NORMAL);
  }
  // Otherwise, draw a laser-sight.
  else if (az_clock_mod(2, 2, state->clock)) {
    const az_color_t beam_color = {255, 128, 128, 48};
    az_add_beam(state, beam_color, beam_start, impact.position, 0.0, 1.0);
  }
}

void az_tick_bad_broken_turret(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_BROKEN_TURRET);
  if (baddie->state == 2) {
    baddie->components[0].angle -= 5.0 * time;
    if (baddie->components[0].angle <= -1.0) {
      baddie->components[0].angle = -1.0;
      baddie->state = 0;
    }
  } else if (baddie->state == 1) {
    baddie->components[0].angle += 5.0 * time;
    if (baddie->components[0].angle >= 1.0) {
      baddie->components[0].angle = 1.0;
      baddie->state = 0;
    }
  } else {
    // Try to aim gun (but sometimes twitch randomly):
    const int aim = az_randint(-1, 1);
    baddie->components[0].angle = fmax(-1.0, fmin(1.0, az_mod2pi(
        (aim == 0 ?
         az_angle_towards(
            baddie->angle + baddie->components[0].angle, 2 * time,
            az_vtheta(az_vsub(state->ship.position, baddie->position))) -
           baddie->angle :
         baddie->components[0].angle + aim * 2 * time))));
    // Fire:
    if (baddie->cooldown <= 0.0 &&
        az_ship_within_angle(state, baddie, baddie->components[0].angle,
                             AZ_DEG2RAD(6)) &&
        az_can_see_ship(state, baddie)) {
      az_fire_baddie_projectile(state, baddie, AZ_PROJ_LASER_PULSE, 20.0,
                                baddie->components[0].angle, 0.0);
      az_play_sound(&state->soundboard, AZ_SND_FIRE_LASER_PULSE);
      baddie->cooldown = 1.0;
    }
    // Randomly go crazy:
    if (az_random(0.0, 2.5) < time) {
      baddie->state = az_randint(1, 2);
      const az_vector_t spark_start =
        az_vadd(baddie->position,
                az_vpolar(20, baddie->angle +
                          baddie->components[0].angle));
      const double spark_angle =
        baddie->angle + baddie->components[0].angle +
        (baddie->state == 1 ? -AZ_DEG2RAD(65) : AZ_DEG2RAD(65));
      for (int i = 0; i < 8; ++i) {
        const double theta =
          spark_angle + az_random(-AZ_DEG2RAD(25), AZ_DEG2RAD(25));
        az_add_speck(state, (az_color_t){255, 200, 100, 255}, 4.0,
                     spark_start, az_vpolar(az_random(10, 70), theta));
      }
    }
  }
}

void az_tick_bad_heavy_turret(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_HEAVY_TURRET);
  aim_towards_ship(state, baddie, time, AZ_DEG2RAD(57), AZ_DEG2RAD(120));
  // State 0: cooling off for next salvo:
  if (baddie->state == 0 && baddie->cooldown <= 0.0 &&
      az_ship_within_angle(state, baddie, baddie->components[0].angle,
                           AZ_DEG2RAD(6)) &&
      az_can_see_ship(state, baddie)) {
    baddie->state = 4;
  }
  // State N: firing salvo, N shots left until next cooldown.
  if (baddie->state > 0 && baddie->cooldown <= 0.0) {
    const double offset =
      (baddie->state % 2 ? AZ_DEG2RAD(-12) : AZ_DEG2RAD(12));
    az_fire_baddie_projectile(state, baddie, AZ_PROJ_LASER_PULSE, 28.6,
                              baddie->components[0].angle - offset, offset);
    az_play_sound(&state->soundboard, AZ_SND_FIRE_LASER_PULSE);
    --baddie->state;
    baddie->cooldown = (baddie->state > 0 ? 0.1 : 1.5);
  }
}

void az_tick_bad_rocket_turret(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_ROCKET_TURRET);
  const double max_angle = AZ_DEG2RAD(57);
  const double barrel_turn_rate = AZ_DEG2RAD(114);
  const bool can_see_ship = az_can_see_ship(state, baddie);
  if (can_see_ship) {
    aim_towards_ship(state, baddie, time, max_angle, barrel_turn_rate);
  }
  // Fire:
  if (baddie->cooldown <= 0.0 && can_see_ship &&
      az_ship_within_angle(state, baddie, baddie->components[0].angle,
                           AZ_DEG2RAD(6))) {
    az_fire_baddie_projectile(state, baddie, AZ_PROJ_HYPER_ROCKET, 20.0,
                              baddie->components[0].angle, 0.0);
    baddie->cooldown = 1.5;
    az_play_sound(&state->soundboard, AZ_SND_FIRE_HYPER_ROCKET);
  }
}

void az_tick_bad_crawling_turret(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_CRAWLING_TURRET);
  if (az_ship_is_decloaked(&state->ship)) {
    baddie->state = (az_vcross(az_vsub(state->ship.position, baddie->position),
                               az_vpolar(1.0, baddie->angle)) > 0.0 ? 1 : 0);
  }
  az_crawl_around(state, baddie, time, (baddie->state != 0), 1.0, 20.0, 100.0);
  aim_towards_ship(state, baddie, time, AZ_DEG2RAD(85), AZ_DEG2RAD(100));
  // Fire:
  if (baddie->cooldown <= 0.0 &&
      az_ship_within_angle(state, baddie, baddie->components[0].angle,
                           AZ_DEG2RAD(6)) &&
      az_can_see_ship(state, baddie)) {
    az_fire_baddie_projectile(state, baddie, AZ_PROJ_LASER_PULSE, 20.0,
                              baddie->components[0].angle, 0.0);
    az_play_sound(&state->soundboard, AZ_SND_FIRE_LASER_PULSE);
    baddie->cooldown = 1.5;
  }
}

void az_tick_bad_crawling_mortar(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_CRAWLING_MORTAR);
  if (az_ship_is_decloaked(&state->ship)) {
    baddie->state = (az_vcross(az_vsub(state->ship.position, baddie->position),
                               az_vpolar(1.0, baddie->angle)) > 0.0 ? 1 : 0);
  }
  az_crawl_around(state, baddie, time, (baddie->state != 0), 1.0, 20.0, 100.0);
  aim_towards_ship(state, baddie, time, AZ_DEG2RAD(85), AZ_DEG2RAD(120));
  // Fire:
  if (baddie->cooldown <= 0.0 &&
      az_ship_within_angle(state, baddie, baddie->components[0].angle,
                           AZ_DEG2RAD(6)) &&
      az_can_see_ship(state, baddie)) {
    az_fire_baddie_projectile(state, baddie, AZ_PROJ_GRENADE, 30.0,
                              baddie->components[0].angle, 0.0);
    az_play_sound(&state->soundboard, AZ_SND_FIRE_ROCKET);
    baddie->cooldown = 1.5;
  }
}

void az_tick_bad_security_drone(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_SECURITY_DRONE);
  // Chase ship:
  az_drift_towards_ship(
      state, baddie, time, 300,
      (az_ship_in_range(state, baddie, 200) ? -300 : 300), 100);
  // Orient body:
  baddie->angle = az_vtheta(baddie->position);
  // Aim gun:
  baddie->components[0].angle = az_mod2pi(az_angle_towards(
      baddie->angle + baddie->components[0].angle, AZ_DEG2RAD(120) * time,
      az_vtheta(az_vsub(state->ship.position,
                        baddie->position))) - baddie->angle);
  // State 0: cooling off for next salvo:
  if (baddie->state == 0 && baddie->cooldown <= 0.0 &&
      az_ship_within_angle(state, baddie, baddie->components[0].angle,
                           AZ_DEG2RAD(6)) &&
      az_can_see_ship(state, baddie)) {
    baddie->state = 10;
  }
  // State N: firing salvo, N shots left until next cooldown.
  if (baddie->state > 0 && baddie->cooldown <= 0.0) {
    const double offset =
      (baddie->state % 2 ? AZ_DEG2RAD(-12) : AZ_DEG2RAD(12));
    az_fire_baddie_projectile(
        state, baddie, AZ_PROJ_LASER_PULSE, 28.6,
        baddie->components[0].angle - offset, offset);
    az_play_sound(&state->soundboard, AZ_SND_FIRE_LASER_PULSE);
    --baddie->state;
    baddie->cooldown = (baddie->state > 0 ? 0.1 : 1.5);
  }
}

void az_tick_bad_pop_open_turret(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_POP_OPEN_TURRET);
  const double max_angle = AZ_DEG2RAD(50);
  const double barrel_turn_rate = AZ_DEG2RAD(40);
  const bool open_shell =
    (baddie->state > 0 && az_ship_in_range(state, baddie, 500));
  // Aim barrel:
  if (open_shell) {
    aim_towards_ship(state, baddie, time, max_angle, barrel_turn_rate);
  } else recenter_barrel(baddie, time, barrel_turn_rate);
  // Open/close shell:
  baddie->components[1].angle =
    az_angle_towards(baddie->components[1].angle, AZ_DEG2RAD(90) * time,
                     (open_shell ? max_angle + AZ_DEG2RAD(10) : 0.0));
  baddie->components[2].angle = -AZ_HALF_PI - baddie->components[1].angle;
  // Fire:
  if (open_shell && baddie->cooldown <= 0.0 &&
      baddie->components[1].angle >
      fabs(baddie->components[0].angle) + AZ_DEG2RAD(5) &&
      az_ship_within_angle(state, baddie, baddie->components[0].angle,
                           AZ_DEG2RAD(6))) {
    az_fire_baddie_projectile(state, baddie, AZ_PROJ_LASER_PULSE, 26.0,
                              baddie->components[0].angle, 0.0);
    az_play_sound(&state->soundboard, AZ_SND_FIRE_LASER_PULSE);
    ++baddie->state;
    if (baddie->state > 3) {
      baddie->state = 1;
      baddie->cooldown = 1.0;
    } else baddie->cooldown = 0.15;
  }
}

/*===========================================================================*/
