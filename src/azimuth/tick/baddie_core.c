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
#include <stdbool.h>
#include <stdint.h>

#include "azimuth/state/baddie.h"
#include "azimuth/state/sound.h"
#include "azimuth/state/space.h"
#include "azimuth/tick/baddie_util.h"
#include "azimuth/tick/object.h"
#include "azimuth/util/random.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

// Primary states:
#define INITIAL_STATE 0
#define DORMANT_STATE 1
#define RAINBOW_BEAM_1_STATE 2
#define RAINBOW_BEAM_2_STATE 3
#define PILLBOX_1_STATE 4
#define PILLBOX_2_STATE 5
#define PRISMATIC_1_STATE 6
#define PRISMATIC_2_STATE 7
#define BUZZSAW_1_STATE 8
#define BUZZSAW_2_STATE 9
#define STARBURST_1_STATE 10
#define STARBURST_2_STATE 11
#define STARBURST_3_STATE 12

// Rainbow beam secondary states:
#define BEAM_CHARGE_FORWARD 0
#define BEAM_CHARGE_BACKWARD 1
#define BEAM_FIRE_FORWARD 2
#define BEAM_FIRE_BACKWARD 3
#define BEAM_COOLDOWN 4

// Warning time, in seconds, to give before firing a prismatic wall:
#define PRISMATIC_CHARGE_TIME 0.75
// Time, in seconds, to spend pumping before firing a prismatic wall:
#define PRISMATIC_PUMP_TIME 0.05
// Distance, in pixels that barriers are pumped:
#define PRISMATIC_PUMP_DIST 60.0

// Distance from baddie center that projectiles start; that is, the outer
// radius of the inner octagon:
#define FIRE_RADIUS 90.0

/*===========================================================================*/

static const struct {
  int next;
  double frac;
} transition_table[] = {
  [DORMANT_STATE]        = { .next = RAINBOW_BEAM_1_STATE, .frac = 1 },
  [RAINBOW_BEAM_1_STATE] = { .next = PILLBOX_1_STATE,      .frac = 0.9 },
  [PILLBOX_1_STATE]      = { .next = BUZZSAW_1_STATE,      .frac = 0.775 },
  [BUZZSAW_1_STATE]      = { .next = STARBURST_1_STATE,    .frac = 0.6 },
  [STARBURST_1_STATE]    = { .next = PRISMATIC_1_STATE,    .frac = 1 },
  [PRISMATIC_1_STATE]    = { .next = RAINBOW_BEAM_2_STATE, .frac = 0.5 },
  [RAINBOW_BEAM_2_STATE] = { .next = BUZZSAW_2_STATE,      .frac = 0.4 },
  [BUZZSAW_2_STATE]      = { .next = STARBURST_2_STATE,    .frac = 0.225 },
  [STARBURST_2_STATE]    = { .next = PILLBOX_2_STATE,      .frac = 1 },
  [PILLBOX_2_STATE]      = { .next = STARBURST_3_STATE,    .frac = 0.1 },
  [STARBURST_3_STATE]    = { .next = PRISMATIC_2_STATE,    .frac = 1 },
  [PRISMATIC_2_STATE]    = { .next = PRISMATIC_2_STATE,    .frac = 0 }
};

#define MAX_PRISMATIC_PATTERN_LENGTH 8
#define NUM_PRISMATIC_PATTERNS 8
static const uint8_t prismatic_1_patterns[][MAX_PRISMATIC_PATTERN_LENGTH] = {
  {0x11, 0x22, 0x44, 0x88, 0x11, 0x22, 0x44, 0x88},
  {0x88, 0x44, 0x22, 0x11, 0x88, 0x44, 0x22, 0x11},
  {0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA},
  {0x22, 0x44, 0x88, 0x11, 0x88, 0x44, 0x22, 0x11},
  {0x33, 0xCC, 0x33, 0xCC, 0x33, 0xCC},
  {0x77, 0xBB, 0xDD, 0xEE},
  {0x22, 0x11, 0x88, 0x44, 0x88, 0x11, 0x22, 0x44},
  {0xEE, 0xDD, 0xBB, 0x77},
};
AZ_STATIC_ASSERT(AZ_ARRAY_SIZE(prismatic_1_patterns) ==
                 NUM_PRISMATIC_PATTERNS);
static const uint8_t prismatic_2_patterns[][MAX_PRISMATIC_PATTERN_LENGTH] = {
  {0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA},
  {0x33, 0x11, 0x99, 0x88, 0xCC, 0x44, 0x66, 0x22},
  {0x22, 0x66, 0x44, 0xCC, 0x88, 0x99, 0x11, 0x33},
  {0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA},
  {0x77, 0xBB, 0xDD, 0xEE, 0x77, 0xBB, 0xDD, 0xEE},
  {0x33, 0xCC, 0x33, 0xCC, 0x33, 0xCC},
  {0xEE, 0xDD, 0xBB, 0x77, 0xEE, 0xDD, 0xBB, 0x77},
  {0x66, 0x99, 0x66, 0x99, 0x66, 0x99},
};
AZ_STATIC_ASSERT(AZ_ARRAY_SIZE(prismatic_2_patterns) ==
                 NUM_PRISMATIC_PATTERNS);

/*===========================================================================*/

static int get_primary_state(az_baddie_t *baddie) {
  return baddie->state & 0xff;
}

static int get_secondary_state(az_baddie_t *baddie) {
  return (baddie->state >> 8) & 0xff;
}

static int get_tertiary_state(az_baddie_t *baddie) {
  return (baddie->state >> 16) & 0xff;
}

static void set_primary_state(az_baddie_t *baddie, int primary) {
  assert(primary >= 0 && primary <= 0xff);
  baddie->state = primary;
}

static void set_secondary_state(az_baddie_t *baddie, int secondary) {
  assert(secondary >= 0 && secondary <= 0xff);
  baddie->state = (baddie->state & 0xff) | (secondary << 8);
}

static void set_tertiary_state(az_baddie_t *baddie, int tertiary) {
  assert(tertiary >= 0 && tertiary <= 0xff);
  baddie->state = (baddie->state & 0xffff) | (tertiary << 16);
}

/*===========================================================================*/

static void move_component_towards(az_baddie_t *baddie, double time, int idx,
                                   az_vector_t goal_position,
                                   double goal_angle) {
  az_component_t *component = &baddie->components[idx];
  az_vpluseq(&component->position,
             az_vcaplen(az_vsub(goal_position, component->position),
                        30.0 * time));
  component->angle = az_angle_towards(component->angle, AZ_DEG2RAD(45) * time,
                                      goal_angle);
}

void az_zenith_core_adjust_to_closed_configuration(
    az_baddie_t *baddie, double time) {
  for (int i = 0; i < 8; ++i) {
    move_component_towards(baddie, time, i, AZ_VZERO, i * AZ_DEG2RAD(45));
  }
}

void az_zenith_core_adjust_to_beam_configuration(
    az_baddie_t *baddie, double time) {
  for (int i = 0; i < 8; ++i) {
    const az_vector_t goal = {0, (i < 4 ? 20 : -20)};
    move_component_towards(baddie, time, i, goal, i * AZ_DEG2RAD(45));
  }
}

static void adjust_to_pillbox_configuration(az_baddie_t *baddie, double time) {
  for (int i = 0; i < 8; ++i) {
    const az_vector_t goal =
      az_vpolar(40, AZ_DEG2RAD(22.5) + i * AZ_DEG2RAD(45));
    move_component_towards(baddie, time, i, goal, i * AZ_DEG2RAD(45));
  }
}

static void adjust_to_buzzsaw_configuration(az_baddie_t *baddie,
                                            double time, bool reverse) {
  const double theta_offset = (reverse ? AZ_DEG2RAD(45) : 0);
  const double angle_offset = (reverse ? AZ_DEG2RAD(-85) : AZ_DEG2RAD(85));
  for (int i = 0; i < 8; ++i) {
    const az_vector_t goal_position =
      az_vpolar(110, theta_offset + i * AZ_DEG2RAD(45));
    const double goal_angle = az_mod2pi(angle_offset + i * AZ_DEG2RAD(45));
    move_component_towards(baddie, time, i, goal_position, goal_angle);
  }
}

static void adjust_to_starburst_configuration(az_baddie_t *baddie,
                                              double time) {
  for (int i = 0; i < 8; ++i) {
    const double goal_rho = (i % 2 == 0 ? 0 : 70);
    const az_vector_t goal_pos =
      az_vpolar(goal_rho, AZ_DEG2RAD(22.5) + i * AZ_DEG2RAD(45));
    move_component_towards(baddie, time, i, goal_pos, i * AZ_DEG2RAD(45));
  }
}

/*===========================================================================*/

// Turn the angle_offset side of the baddie towards the ship.
static void turn_offset_towards_ship(
    az_space_state_t *state, az_baddie_t *baddie, double time,
    double angle_offset) {
  baddie->angle = az_angle_towards(
      baddie->angle, AZ_DEG2RAD(30) * time,
      az_mod2pi(az_vtheta(az_vsub(state->ship.position, baddie->position)) +
                angle_offset));
}

static bool double_towards(double *value, double delta, double goal) {
  assert(delta >= 0.0);
  if (*value < goal - delta) *value += delta;
  else if (*value > goal + delta) *value -= delta;
  else {
    *value = goal;
    return true;
  }
  return false;
}

static bool adjust_gravity(az_space_state_t *state, double time,
                           double goal, bool rotational) {
  const double delta = 165.0 * time;
  az_gravfield_t *pull_grav = NULL;
  az_gravfield_t *spin_grav = NULL;
  AZ_ARRAY_LOOP(gravfield, state->gravfields) {
    if (gravfield->kind == AZ_GRAV_SECTOR_PULL) pull_grav = gravfield;
    else if (gravfield->kind == AZ_GRAV_SECTOR_SPIN) spin_grav = gravfield;
  }
  az_gravfield_t *reduce_grav = (rotational ? pull_grav : spin_grav);
  az_gravfield_t *adjust_grav = (rotational ? spin_grav : pull_grav);
  if (reduce_grav != NULL &&
      !double_towards(&reduce_grav->strength, delta, 0.0)) return false;
  if (adjust_grav != NULL &&
      !double_towards(&adjust_grav->strength, delta, goal)) return false;
  return true;
}

static void spin_gravity_back_and_forth(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  const double goal = 600.0 * baddie->param;
  if (adjust_gravity(state, time, goal, true)) baddie->param = -baddie->param;
}

static void pull_gravity_in_and_out(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  const double goal = 500.0 * baddie->param;
  if (adjust_gravity(state, time, goal, false)) baddie->param = -baddie->param;
}

/*===========================================================================*/

static void init_rainbow_beam(az_space_state_t *state, az_baddie_t *baddie,
                              bool lay_mines) {
  assert(baddie->kind == AZ_BAD_ZENITH_CORE);
  set_primary_state(baddie, (lay_mines ? RAINBOW_BEAM_2_STATE :
                             RAINBOW_BEAM_1_STATE));
  set_secondary_state(baddie,
                      (az_vdot(az_vsub(state->ship.position, baddie->position),
                               az_vpolar(1.0, baddie->angle)) >= 0.0 ?
                       BEAM_CHARGE_FORWARD : BEAM_CHARGE_BACKWARD));
  baddie->cooldown = 1.0;
  az_play_sound(&state->soundboard, AZ_SND_CORE_BEAM_CHARGE);
  if (lay_mines) {
    baddie->param = 1.0;
    for (int i = 0; i < 360; i += 45) {
      const double abs_angle =
        AZ_DEG2RAD(22.5) + AZ_DEG2RAD(i) + baddie->angle;
      bool mine_exists = false;
      AZ_ARRAY_LOOP(other, state->baddies) {
        if (other->kind != AZ_BAD_PROXY_MINE) continue;
        const double other_angle =
          az_vtheta(az_vsub(other->position, baddie->position));
        if (fabs(az_mod2pi(other_angle - abs_angle)) <= AZ_DEG2RAD(22)) {
          mine_exists = true;
          break;
        }
      }
      if (!mine_exists) {
        az_baddie_t *mine = az_add_baddie(
            state, AZ_BAD_PROXY_MINE,
            az_vadd(az_vpolar(FIRE_RADIUS, abs_angle), baddie->position),
            az_random(-AZ_PI, AZ_PI));
        if (mine != NULL) {
          mine->velocity = az_vpolar(az_random(400, 900), abs_angle);
        }
      }
    }
  }
}

static void init_buzzsaw(az_baddie_t *baddie, bool reverse) {
  assert(baddie->kind == AZ_BAD_ZENITH_CORE);
  set_primary_state(baddie, (reverse ? BUZZSAW_2_STATE : BUZZSAW_1_STATE));
  baddie->param = 0.0;
  baddie->cooldown = 2.0;
}

static void init_pillbox(az_baddie_t *baddie, bool super) {
  assert(baddie->kind == AZ_BAD_ZENITH_CORE);
  set_primary_state(baddie, (super ? PILLBOX_2_STATE : PILLBOX_1_STATE));
  baddie->param = 1.0;
  baddie->cooldown = 1.0;
}

static void init_prismatic(az_baddie_t *baddie, bool super) {
  assert(baddie->kind == AZ_BAD_ZENITH_CORE);
  set_primary_state(baddie, (super ? PRISMATIC_2_STATE : PRISMATIC_1_STATE));
  baddie->param = 0.0;
  baddie->cooldown = 3.0;
}

static void init_starburst(az_baddie_t *baddie, int num_blasts) {
  assert(baddie->kind == AZ_BAD_ZENITH_CORE);
  assert(num_blasts >= 1 && num_blasts <= 3);
  set_primary_state(baddie, (num_blasts == 1 ? STARBURST_1_STATE :
                             num_blasts == 2 ? STARBURST_2_STATE :
                             STARBURST_3_STATE));
  set_secondary_state(baddie, num_blasts);
  baddie->param = 0.0;
  baddie->cooldown = 0.0;
}

static bool try_transition(az_space_state_t *state, az_baddie_t *baddie) {
  assert(baddie->kind == AZ_BAD_ZENITH_CORE);
  const int primary = get_primary_state(baddie);
  if (baddie->health <=
      transition_table[primary].frac * baddie->data->max_health) {
    switch (transition_table[primary].next) {
      case RAINBOW_BEAM_1_STATE:
        init_rainbow_beam(state, baddie, false);
        return true;
      case RAINBOW_BEAM_2_STATE:
        init_rainbow_beam(state, baddie, true);
        return true;
      case PILLBOX_1_STATE:
        init_pillbox(baddie, false);
        return true;
      case PILLBOX_2_STATE:
        init_pillbox(baddie, true);
        return true;
      case BUZZSAW_1_STATE:
        init_buzzsaw(baddie, false);
        return true;
      case BUZZSAW_2_STATE:
        init_buzzsaw(baddie, true);
        return true;
      case PRISMATIC_1_STATE:
        init_prismatic(baddie, false);
        return true;
      case PRISMATIC_2_STATE:
        init_prismatic(baddie, true);
        return true;
      case STARBURST_1_STATE:
        init_starburst(baddie, 1);
        return true;
      case STARBURST_2_STATE:
        init_starburst(baddie, 2);
        return true;
      case STARBURST_3_STATE:
        init_starburst(baddie, 3);
        return true;
    }
  }
  return false;
}

/*===========================================================================*/

static void charge_rainbow_beam(
    az_space_state_t *state, az_baddie_t *baddie, double time,
    double angle_offset, int next_secondary_state) {
  const double particle_lifetime = 0.5;
  const double particle_distance = 50.0;
  if (baddie->cooldown >= particle_lifetime) {
    const az_vector_t center =
      az_vadd(baddie->position,
              az_vpolar(FIRE_RADIUS, baddie->angle + angle_offset));
    const double angle = 145.0 * baddie->cooldown;
    az_particle_t *particle;
    if (az_insert_particle(state, &particle)) {
      particle->kind = AZ_PAR_EMBER;
      particle->color = az_hsva_color(2.0 * angle, 0.5, 1.0, 0.75);
      particle->position =
        az_vadd(center, az_vpolar(particle_distance, angle));
      particle->velocity =
        az_vpolar(-particle_distance / particle_lifetime, angle);
      particle->angle = angle;
      particle->lifetime = particle_lifetime;
      particle->param1 = 7.0;
    }
  }
  if (baddie->cooldown <= 0.0) {
    set_secondary_state(baddie, next_secondary_state);
    baddie->cooldown = 5.0;
  }
}

static void fire_rainbow_beam(az_space_state_t *state, az_baddie_t *baddie,
                              double time, double angle_offset) {
  turn_offset_towards_ship(state, baddie, time, angle_offset);
  if (baddie->param == 0.0 && baddie->cooldown < 4.5) {
    baddie->param = -copysign(1.0, az_mod2pi(
        az_vtheta(az_vsub(state->ship.position, baddie->position)) -
        (baddie->angle + angle_offset)));
  }

  // Fire a beam, piercing through the ship and other baddies.
  const double beam_angle = baddie->angle + angle_offset;
  const az_vector_t beam_start =
    az_vadd(baddie->position, az_vpolar(FIRE_RADIUS, beam_angle));
  az_impact_t impact;
  az_ray_impact(state, beam_start, az_vpolar(1000, beam_angle),
                (AZ_IMPF_BADDIE | AZ_IMPF_SHIP), baddie->uid, &impact);
  const az_vector_t beam_delta = az_vsub(impact.position, beam_start);
  const double beam_damage = 100.0 * time;
  // Damage the ship and any baddies within the beam.
  if (az_ship_is_alive(&state->ship) &&
      az_ray_hits_ship(&state->ship, beam_start, beam_delta,
                       NULL, NULL)) {
    az_damage_ship(state, beam_damage, false);
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
  az_particle_t *particle;
  if (az_clock_mod(2, 1, state->clock) == 0 &&
      az_insert_particle(state, &particle)) {
    particle->kind = AZ_PAR_EXPLOSION;
    particle->color = beam_color;
    particle->position = impact.position;
    particle->velocity = AZ_VZERO;
    particle->angle = beam_angle;
    particle->lifetime = 0.5;
    particle->param1 = 8.0;
  }
  az_loop_sound(&state->soundboard, AZ_SND_CORE_BEAM_FIRE);

  if (baddie->cooldown <= 0.0) {
    set_secondary_state(baddie, BEAM_COOLDOWN);
    baddie->cooldown = 0.5;
  }
}

static void do_rainbow_beam(az_space_state_t *state, az_baddie_t *baddie,
                            double time, bool lay_mines) {
  assert(baddie->kind == AZ_BAD_ZENITH_CORE);
  az_zenith_core_adjust_to_beam_configuration(baddie, time);
  spin_gravity_back_and_forth(state, baddie, time);
  switch (get_secondary_state(baddie)) {
    case BEAM_CHARGE_FORWARD:
      charge_rainbow_beam(state, baddie, time, 0, BEAM_FIRE_FORWARD);
      break;
    case BEAM_CHARGE_BACKWARD:
      charge_rainbow_beam(state, baddie, time, AZ_PI, BEAM_FIRE_BACKWARD);
      break;
    case BEAM_FIRE_FORWARD:
      fire_rainbow_beam(state, baddie, time, 0);
      break;
    case BEAM_FIRE_BACKWARD:
      fire_rainbow_beam(state, baddie, time, AZ_PI);
      break;
    case BEAM_COOLDOWN:
      if (baddie->cooldown <= 0.0) {
        if (!try_transition(state, baddie)) {
          init_rainbow_beam(state, baddie, lay_mines);
        }
      }
      break;
  }
}

static void do_pillbox_1(az_space_state_t *state, az_baddie_t *baddie,
                         double time) {
  assert(baddie->kind == AZ_BAD_ZENITH_CORE);
  adjust_to_pillbox_configuration(baddie, time);
  pull_gravity_in_and_out(state, baddie, time);
  const int secondary = get_secondary_state(baddie);
  const double turn_rate = (secondary == 1 ? -1 : 1) * AZ_DEG2RAD(30);
  baddie->angle = az_mod2pi(baddie->angle + turn_rate * time);
  if (baddie->cooldown <= 0.0) {
    const double angle = AZ_DEG2RAD(45) *
      round((az_vtheta(az_vsub(state->ship.position, baddie->position)) -
             baddie->angle) / AZ_DEG2RAD(45));
    az_fire_baddie_projectile(state, baddie, AZ_PROJ_ROCKET,
                              FIRE_RADIUS, angle, 0.0);
    az_play_sound(&state->soundboard, AZ_SND_FIRE_ROCKET);
    if (!try_transition(state, baddie)) {
      const int remaining = get_tertiary_state(baddie) - 1;
      if (remaining > 0) {
        set_tertiary_state(baddie, remaining);
      } else {
        set_secondary_state(baddie, (secondary == 0 ? 1 : 0));
        set_tertiary_state(baddie, az_randint(3, 9));
      }
      baddie->cooldown = 0.5;
    }
  }
}

static void do_pillbox_2(az_space_state_t *state, az_baddie_t *baddie,
                         double time) {
  assert(baddie->kind == AZ_BAD_ZENITH_CORE);
  adjust_to_pillbox_configuration(baddie, time);
  pull_gravity_in_and_out(state, baddie, time);
  const int secondary = get_secondary_state(baddie);
  if (secondary == 0) {
    if (baddie->cooldown <= 0.0 && !try_transition(state, baddie)) {
      set_secondary_state(baddie, az_randint(1, 2));
      baddie->cooldown = 0.3;
    }
  } else {
    const double turn_rate = (secondary == 1 ? -1 : 1) * AZ_DEG2RAD(60);
    baddie->angle = az_mod2pi(baddie->angle + turn_rate * time);
    if (baddie->cooldown <= 0.0) {
      const double angle = AZ_DEG2RAD(45) *
        round((az_vtheta(az_vsub(state->ship.position, baddie->position)) -
               baddie->angle) / AZ_DEG2RAD(45));
      for (int i = -1; i <= 1; ++i) {
        az_fire_baddie_projectile(state, baddie, AZ_PROJ_ROCKET,
                                  FIRE_RADIUS, angle, i * AZ_DEG2RAD(10));
      }
      az_play_sound(&state->soundboard, AZ_SND_FIRE_ROCKET);
      set_secondary_state(baddie, 0);
      baddie->cooldown = 0.3;
    }
  }
}

static bool advance_prismatic(az_space_state_t *state, az_baddie_t *baddie,
                              bool super) {
  const int pattern_index =
    get_secondary_state(baddie) % NUM_PRISMATIC_PATTERNS;
  const int mask_index = get_tertiary_state(baddie);
  const uint8_t mask = (mask_index >= MAX_PRISMATIC_PATTERN_LENGTH ? 0 :
                        (super ? prismatic_2_patterns :
                         prismatic_1_patterns)[pattern_index][mask_index]);
  if (mask == 0) return (baddie->cooldown <= 0.0);
  if (baddie->param == 0.0 && baddie->cooldown <= PRISMATIC_CHARGE_TIME) {
    baddie->param = 1.0;
    baddie->param = 1.0;
    for (int i = 0; i < 8; ++i) {
      if (!(mask & (1 << i))) continue;
      const double angle =
        baddie->angle + AZ_DEG2RAD(22.5) + i * AZ_DEG2RAD(45);
      const az_vector_t base = az_vpolar(90.0, angle);
      const az_vector_t perp = az_vpolar(7.0, angle + AZ_HALF_PI);
      for (int j = 0; j < 12; ++j) {
        az_particle_t *particle;
        if (az_insert_particle(state, &particle)) {
          particle->kind = AZ_PAR_EMBER;
          particle->color = az_hsva_color(j * AZ_DEG2RAD(60), 0.5, 1.0, 0.75);
          particle->position = az_vadd(base, az_vmul(perp, j - 5.5));
          particle->velocity = AZ_VZERO;
          particle->angle = angle;
          particle->lifetime = PRISMATIC_CHARGE_TIME - PRISMATIC_PUMP_TIME;
          particle->param1 = 20.0;
        }
      }
    }
    az_play_sound(&state->soundboard, AZ_SND_PRISMATIC_CHARGE);
  }
  if (baddie->cooldown <= PRISMATIC_PUMP_TIME) {
    for (int i = 0; i < 8; ++i) {
      if (!(mask & (1 << i))) continue;
      baddie->components[i].position =
        az_vpolar(PRISMATIC_PUMP_DIST *
                  (1.0 - baddie->cooldown / PRISMATIC_PUMP_TIME),
                  AZ_DEG2RAD(22.5) + i * AZ_DEG2RAD(45));
    }
  }
  if (baddie->cooldown <= 0.0) {
    baddie->param = 0.0;
    int num_walls = 0;
    for (int i = 0; i < 8; ++i) {
      if (!(mask & (1 << i))) continue;
      az_fire_baddie_projectile(state, baddie, AZ_PROJ_PRISMATIC_WALL, 90.0,
                                AZ_DEG2RAD(22.5) + i * AZ_DEG2RAD(45), 0.0);
      ++num_walls;
    }
    az_play_sound(&state->soundboard, AZ_SND_PRISMATIC_FIRE);
    set_tertiary_state(baddie, mask_index + 1);
    baddie->cooldown = (super ? 1.0 : 0.4 * num_walls);
  }
  return false;
}

static void do_prismatic(az_space_state_t *state, az_baddie_t *baddie,
                         double time, bool super) {
  assert(baddie->kind == AZ_BAD_ZENITH_CORE);
  az_zenith_core_adjust_to_closed_configuration(baddie, time);
  adjust_gravity(state, time, 300.0, false);
  baddie->angle = az_angle_towards(baddie->angle, AZ_DEG2RAD(90) * time,
                                   AZ_DEG2RAD(22.5));
  const bool pattern_done = advance_prismatic(state, baddie, super);
  if (pattern_done && !try_transition(state, baddie)) {
    set_secondary_state(baddie, ((get_secondary_state(baddie) + 1) %
                                 NUM_PRISMATIC_PATTERNS));
    baddie->cooldown = 2.5;
  }
}

static void do_starburst(az_space_state_t *state, az_baddie_t *baddie,
                         double time) {
  assert(baddie->kind == AZ_BAD_ZENITH_CORE);
  adjust_to_starburst_configuration(baddie, time);
  adjust_gravity(state, time, -400.0, false);
  const double max_turn_rate = AZ_DEG2RAD(720);
  const double turn_accel = AZ_DEG2RAD(200);
  switch (get_tertiary_state(baddie)) {
    case 0:
      // Accelerate spinning:
      baddie->param = fmin(baddie->param + turn_accel * time, max_turn_rate);
      baddie->angle = az_mod2pi(baddie->angle + baddie->param * time);
      if (baddie->param >= max_turn_rate) {
        set_tertiary_state(baddie, 1);
        baddie->cooldown = az_random(0.5, 2.0);
      }
      break;
    case 1: {
      // Wait until we're lined up with ship:
      baddie->angle = az_mod2pi(baddie->angle + max_turn_rate * time);
      if (baddie->cooldown <= 0.0) {
        double angle_delta = 0.0;
        if (az_ship_is_decloaked(&state->ship)) {
          angle_delta =
            az_mod2pi((az_vtheta(az_vsub(state->ship.position,
                                         baddie->position)) -
                       (baddie->angle + AZ_DEG2RAD(22.5))) * 4) * 0.25;
        }
        if (fabs(angle_delta) <= AZ_DEG2RAD(8)) {
          baddie->angle = az_mod2pi(baddie->angle + angle_delta);
          set_tertiary_state(baddie, 2);
          baddie->cooldown = 0.1;
        }
      }
    } break;
    case 2:
      // Fire starburst shots (after a momentary pause):
      if (baddie->cooldown <= 0.0) {
        for (int i = 0; i < 360; i += 90) {
          az_fire_baddie_projectile(state, baddie, AZ_PROJ_STARBURST_BLAST,
                                    112.0, AZ_DEG2RAD(22.5) + AZ_DEG2RAD(i),
                                    0.0);
        }
        az_play_sound(&state->soundboard, AZ_SND_FIRE_HYPER_ROCKET);
        set_tertiary_state(baddie, 3);
        baddie->cooldown = 0.5;
      }
      break;
    case 3:
      // Proceed to next state (after a brief pause):
      if (baddie->cooldown <= 0.0) {
        const int blasts_remaining = get_secondary_state(baddie) - 1;
        if (blasts_remaining > 0) {
          set_secondary_state(baddie, blasts_remaining);
          set_tertiary_state(baddie, 1);
        } else if (!try_transition(state, baddie)) {
          init_starburst(baddie, 1);
        }
      }
      break;
  }
}

static void do_buzzsaw(az_space_state_t *state, az_baddie_t *baddie,
                       double time, double reverse) {
  assert(baddie->kind == AZ_BAD_ZENITH_CORE);
  const double turn_rate = (reverse ? AZ_DEG2RAD(-90) : AZ_DEG2RAD(60));
  adjust_to_buzzsaw_configuration(baddie, time, reverse);
  adjust_gravity(state, time, 400.0, false);
  baddie->angle = az_mod2pi(baddie->angle + turn_rate * time);
  if (baddie->cooldown <= 0.0) {
    const int secondary = get_secondary_state(baddie);
    const double angle = (reverse ?
                          az_random(AZ_DEG2RAD(60), AZ_DEG2RAD(80)) :
                          az_random(AZ_DEG2RAD(-80), AZ_DEG2RAD(-60)));
    az_fire_baddie_projectile(state, baddie, AZ_PROJ_ORBITAL_TORPEDO,
                              120.0, AZ_DEG2RAD(45) * secondary, angle);
    az_play_sound(&state->soundboard, AZ_SND_FIRE_FIREBALL);
    set_secondary_state(baddie, az_modulo(secondary - 3, 8));
    if (!try_transition(state, baddie)) {
      baddie->cooldown = az_random(1.0, 1.5) * (reverse ? 0.4 : 0.5);
    }
  }
}

/*===========================================================================*/

void az_tick_bad_zenith_core(az_space_state_t *state, az_baddie_t *baddie,
                             double time) {
  assert(baddie->kind == AZ_BAD_ZENITH_CORE);
  // If the ship accidentally gets caught inside the baddie, knock it away.
  if (az_ship_is_alive(&state->ship) &&
      az_circle_touches_baddie(baddie, AZ_SHIP_DEFLECTOR_RADIUS,
                               state->ship.position, NULL)) {
    const az_vector_t unit =
      az_vunit(az_vsub(state->ship.position, baddie->position));
    if (az_vdot(state->ship.velocity, unit) < 100.0) {
      az_vpluseq(&state->ship.velocity, az_vmul(unit, 800.0));
    }
  }
  switch (get_primary_state(baddie)) {
    case INITIAL_STATE:
      baddie->temp_properties |= AZ_BADF_INVINCIBLE;
      break;
    case DORMANT_STATE:
      baddie->temp_properties |= AZ_BADF_INVINCIBLE;
      if (get_secondary_state(baddie) == 0) {
        set_secondary_state(baddie, 1);
        baddie->cooldown = 10.0;
      } else if (baddie->cooldown <= 0.0) {
        try_transition(state, baddie);
      }
      break;
    case RAINBOW_BEAM_1_STATE:
      do_rainbow_beam(state, baddie, time, false);
      break;
    case RAINBOW_BEAM_2_STATE:
      do_rainbow_beam(state, baddie, time, true);
      break;
    case PILLBOX_1_STATE:
      do_pillbox_1(state, baddie, time);
      break;
    case PILLBOX_2_STATE:
      do_pillbox_2(state, baddie, time);
      break;
    case BUZZSAW_1_STATE:
      do_buzzsaw(state, baddie, time, false);
      break;
    case BUZZSAW_2_STATE:
      do_buzzsaw(state, baddie, time, true);
      break;
    case PRISMATIC_1_STATE:
      do_prismatic(state, baddie, time, false);
      break;
    case PRISMATIC_2_STATE:
      do_prismatic(state, baddie, time, true);
      break;
    case STARBURST_1_STATE:
    case STARBURST_2_STATE:
    case STARBURST_3_STATE:
      do_starburst(state, baddie, time);
      break;
    default:
      set_primary_state(baddie, DORMANT_STATE);
      break;
  }
}

/*===========================================================================*/
