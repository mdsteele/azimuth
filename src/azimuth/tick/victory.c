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

#include "azimuth/tick/victory.h"

#include "azimuth/state/baddie.h"
#include "azimuth/state/baddie_oth.h"
#include "azimuth/state/victory.h"
#include "azimuth/tick/baddie_oth.h"
#include "azimuth/tick/baddie_util.h"
#include "azimuth/tick/particle.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

static void fly_towards(az_baddie_t *baddie, az_vector_t goal, double time,
                        double turn_rate, double max_speed,
                        double forward_accel, double lateral_decel_rate) {
  const double backward_accel = 80.0;
  const az_vector_t delta = az_vsub(goal, baddie->position);
  baddie->angle =
    az_angle_towards(baddie->angle, turn_rate * time, az_vtheta(delta));
  const az_vector_t unit = az_vpolar(1, baddie->angle);
  const az_vector_t lateral = az_vflatten(baddie->velocity, unit);
  const az_vector_t dvel =
    az_vadd(az_vcaplen(az_vneg(lateral), lateral_decel_rate * time),
            (az_vdot(unit, delta) >= 0.0 ?
             az_vmul(unit, forward_accel * time) :
             az_vmul(unit, -backward_accel * time)));
  baddie->velocity = az_vcaplen(az_vadd(baddie->velocity, dvel), max_speed);
}

static void tick_baddies(az_victory_state_t *state, double time) {
  AZ_ARRAY_LOOP(baddie, state->baddies) {
    if (baddie->kind == AZ_BAD_NOTHING) continue;
    const double old_angle = baddie->angle;
    az_vpluseq(&baddie->position, az_vmul(baddie->velocity, time));
    switch (baddie->kind) {
      case AZ_BAD_OTH_SNAPDRAGON: {
        az_vector_t goal = {75, -50};
        if (baddie->state == 1) goal = (az_vector_t){-400, -10};
        if (baddie->state == 2) goal = (az_vector_t){100, -400};
        fly_towards(baddie, goal, time, 5.0, 300, 300, 200);
        az_tick_oth_tendrils(baddie, &AZ_OTH_SNAPDRAGON_TENDRILS, old_angle,
                             time, state->step_timer);
      } break;
      case AZ_BAD_OTH_RAZOR_1: {
        baddie->velocity = baddie->velocity = az_vpolar(300, baddie->angle);
        az_tick_oth_tendrils(baddie, &AZ_OTH_RAZOR_TENDRILS, old_angle, time,
                             state->step_timer);
      } break;
      case AZ_BAD_ROCKWYRM: {
        // TODO: Open jaws and shoot
        const az_vector_t goal = az_vadd((az_vector_t){-360, -180},
                                         az_vmul((az_vector_t){208, 120},
                                                 state->step_timer));
        az_snake_towards(NULL, baddie, time, 2, 240.0, 50.0, goal, true);
      } break;
      default: break;
    }
  }
}

/*===========================================================================*/

static void tick_particles(az_victory_state_t *state, double time) {
  AZ_ARRAY_LOOP(particle, state->particles) {
    az_tick_particle(particle, time);
  }
}

/*===========================================================================*/

static void projectile_special_logic(
    az_victory_state_t *state, az_projectile_t *proj, double time) {
  switch (proj->kind) {
    case AZ_PROJ_OTH_ROCKET:
      az_victory_add_particle(state, AZ_PAR_OTH_FRAGMENT, AZ_WHITE,
                              proj->position, AZ_VZERO, proj->angle, 0.5,
                              9.0, AZ_DEG2RAD(720));
      break;
    default: break;
  }
}

static void tick_projectiles(az_victory_state_t *state, double time) {
  AZ_ARRAY_LOOP(proj, state->projectiles) {
    if (proj->kind == AZ_PROJ_NOTHING) continue;
    proj->age += time;
    projectile_special_logic(state, proj, time);
    if (proj->age > proj->data->lifetime) proj->kind = AZ_PROJ_NOTHING;
    else az_vpluseq(&proj->position, az_vmul(proj->velocity, time));
  }
}

/*===========================================================================*/

static bool timer_at(const az_victory_state_t *state, double mark,
                     double time) {
  return (state->step_timer >= mark && state->step_timer - time < mark);
}

static void next_step_at(az_victory_state_t *state, double mark) {
  if (state->step_timer >= mark) {
    ++state->step;
    state->step_timer = 0.0;
    az_victory_clear_objects(state);
  }
}

void az_tick_victory_state(az_victory_state_t *state, double time) {
  ++state->clock;
  tick_particles(state, time);
  tick_projectiles(state, time);
  tick_baddies(state, time);
  state->step_timer += time;
  az_baddie_t *boss = &state->baddies[0];
  switch (state->step) {
    case AZ_VS_START: {
      next_step_at(state, 1.0);
    } break;
    case AZ_VS_SNAPDRAGON: {
      if (timer_at(state, 0.5, time)) {
        az_init_baddie(boss, AZ_BAD_OTH_SNAPDRAGON,
                       (az_vector_t){-360, -180}, AZ_DEG2RAD(30));
        boss->velocity = az_vpolar(300, boss->angle);
      }
      if (timer_at(state, 1.7, time)) boss->state = 1;
      if (timer_at(state, 2.5, time)) {
        az_victory_add_projectile(state, AZ_PROJ_OTH_ROCKET,
                                  az_vadd(boss->position,
                                          az_vpolar(30.0, boss->angle)),
                                  boss->angle);
        az_play_sound(&state->soundboard, AZ_SND_FIRE_OTH_ROCKET);
      }
      if (timer_at(state, 3.0, time)) boss->state = 2;
      if (timer_at(state, 3.5, time)) {
        for (int i = -1; i <= 1; ++i) {
          az_victory_add_baddie(state, AZ_BAD_OTH_RAZOR_1, boss->position,
                                boss->angle + AZ_PI + i * AZ_DEG2RAD(45));
        }
        az_play_sound(&state->soundboard, AZ_SND_LAUNCH_OTH_RAZORS);
      }
      next_step_at(state, 6.0);
    } break;
    case AZ_VS_ROCKWYRM: {
      if (timer_at(state, 0.1, time)) {
        az_init_baddie(boss, AZ_BAD_ROCKWYRM,
                       (az_vector_t){-360, -180}, AZ_DEG2RAD(30));
      }
      next_step_at(state, 6.0);
    } break;
    case AZ_VS_GUNSHIP: {
      next_step_at(state, 6.0);
    } break;
    case AZ_VS_FORCEFIEND: {
      next_step_at(state, 6.0);
    } break;
    case AZ_VS_KILOFUGE: {
      next_step_at(state, 6.0);
    } break;
    case AZ_VS_NOCTURNE: {
      next_step_at(state, 6.0);
    } break;
    case AZ_VS_MAGBEEST: {
      next_step_at(state, 6.0);
    } break;
    case AZ_VS_SUPERGUNSHIP: {
      next_step_at(state, 6.0);
    } break;
    case AZ_VS_CORE: {
      next_step_at(state, 6.0);
    } break;
    case AZ_VS_DONE: break;
  }
}

/*===========================================================================*/
