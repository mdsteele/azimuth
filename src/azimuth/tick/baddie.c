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

#include "azimuth/tick/baddie.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>

#include "azimuth/constants.h"
#include "azimuth/state/baddie.h"
#include "azimuth/state/projectile.h"
#include "azimuth/tick/baddie_chomper.h"
#include "azimuth/tick/baddie_crawler.h"
#include "azimuth/tick/baddie_forcefiend.h"
#include "azimuth/tick/baddie_kilofuge.h"
#include "azimuth/tick/baddie_myco.h"
#include "azimuth/tick/baddie_nocturne.h"
#include "azimuth/tick/baddie_oth.h"
#include "azimuth/tick/baddie_turret.h"
#include "azimuth/tick/baddie_util.h"
#include "azimuth/tick/baddie_wyrm.h"
#include "azimuth/tick/baddie_zipper.h"
#include "azimuth/tick/object.h"
#include "azimuth/tick/script.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/random.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

static bool perch_on_wall_ahead(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  const double max_speed = 500.0;
  const double accel = 200.0;
  const double lateral_decel_rate = 250.0;
  // If we kept going straight, what's the distance until we hit a wall?
  az_impact_t impact;
  az_circle_impact(state, baddie->data->main_body.bounding_radius,
                   baddie->position, az_vpolar(1000, baddie->angle),
                   (AZ_IMPF_BADDIE | AZ_IMPF_SHIP), baddie->uid, &impact);
  const double dist = az_vdist(baddie->position, impact.position);
  // If we're there, stop.
  if (dist <= 2.0) {
    baddie->velocity = AZ_VZERO;
    baddie->angle = az_vtheta(impact.normal);
    return true;
  }
  // Otherwise, steady ourselves sideways, and accel/decel to an appropriate
  // speed depending on how far we are from the perch.
  const az_vector_t unit = az_vpolar(1, baddie->angle);
  const az_vector_t lateral = az_vflatten(baddie->velocity, unit);
  const double forward_speed = az_vdot(baddie->velocity, unit);
  const double desired_speed = 0.8 * dist + 90.0;
  const az_vector_t dvel =
    az_vadd(az_vcaplen(az_vneg(lateral), lateral_decel_rate * time),
            (forward_speed < desired_speed ?
             az_vmul(unit, accel * time) :
             az_vmul(unit, -accel * time)));
  baddie->velocity = az_vcaplen(az_vadd(baddie->velocity, dvel), max_speed);
  return false;
}

static bool perch_on_ceiling(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  const double turn_rate = 4.0;
  // Pick a ceiling to perch on.
  const double baddie_norm = az_vnorm(baddie->position);
  const double baddie_theta = az_vtheta(baddie->position);
  az_vector_t goal = AZ_VZERO;
  double best_dist = INFINITY;
  AZ_ARRAY_LOOP(wall, state->walls) {
    if (wall->kind == AZ_WALL_NOTHING) continue;
    if (az_vnorm(wall->position) < baddie_norm) continue;
    const az_vector_t delta = az_vsub(wall->position, baddie->position);
    const double dist = az_vnorm(delta) +
      150.0 * fabs(az_mod2pi(az_vtheta(delta) - baddie_theta));
    if (dist < best_dist) {
      goal = wall->position;
      best_dist = dist;
    }
  }
  // Turn towards that wall.
  const double goal_theta = az_vtheta(az_vsub(goal, baddie->position));
  baddie->angle =
    az_angle_towards(baddie->angle, turn_rate * time, goal_theta);
  // Try to perch.
  return perch_on_wall_ahead(state, baddie, time);
}

/*===========================================================================*/

static void tick_boss_door(az_space_state_t *state, az_baddie_t *baddie,
                           double time) {
  static const double max_angle = AZ_DEG2RAD(50);
  static const double beam_duration = 0.75;
  // Aim eye towards ship:
  if (baddie->state != 2) {
    const double turn_rate =
      (baddie->state == 3 ? AZ_DEG2RAD(40) : AZ_DEG2RAD(120));
    baddie->components[0].angle =
      fmax(-max_angle, fmin(max_angle, az_mod2pi(az_angle_towards(
        baddie->angle + baddie->components[0].angle, turn_rate * time,
        az_vtheta(az_vsub(state->ship.position, baddie->position))) -
                                                 baddie->angle)));
  }
  // States 0 and 1: Wait for an opportunity to fire at ship.
  if (baddie->state == 0 || baddie->state == 1) {
    const bool can_see_ship = az_ship_is_decloaked(&state->ship) &&
      (fabs(az_mod2pi(az_vtheta(az_vsub(
           state->ship.position, baddie->position)) -
                      baddie->angle)) <= max_angle) &&
      az_can_see_ship(state, baddie);
    const double eyelid_turn = AZ_DEG2RAD(360) * time;
    // State 0: Wait for ship to get in sight.
    if (baddie->state == 0) {
      baddie->components[1].angle =
        az_angle_towards(baddie->components[1].angle, eyelid_turn, 0);
      baddie->param = fmax(0.0, baddie->param - 2.0 * time);
      if (can_see_ship && az_ship_in_range(state, baddie, 200)) {
        baddie->state = 1;
      }
    }
    // State 1: Open eyelids and charge up beam.
    else {
      baddie->components[1].angle =
        az_angle_towards(baddie->components[1].angle, eyelid_turn,
                         max_angle + AZ_DEG2RAD(10));
      if (baddie->cooldown <= 0.0) {
        baddie->param = fmin(1.0, baddie->param + time);
      }
      if (!(can_see_ship && az_ship_in_range(state, baddie, 300))) {
        baddie->state = 0;
      } else if (baddie->param >= 1.0 &&
                 baddie->components[1].angle >= max_angle) {
        baddie->state = 2;
        baddie->cooldown = 0.35;
        az_play_sound(&state->soundboard, AZ_SND_CHARGED_MISSILE_BEAM);
      }
    }
    baddie->components[2].angle = -AZ_HALF_PI - baddie->components[1].angle;
  }
  // State 2: Pause for a short time before firing.
  else if (baddie->state == 2) {
    if (baddie->cooldown <= 0.0) {
      baddie->state = 3;
      baddie->cooldown = beam_duration;
      az_play_sound(&state->soundboard, AZ_SND_FIRE_MISSILE_BEAM);
    }
  }
  // State 3: Fire beam until cooldown expires.
  else {
    baddie->param = baddie->cooldown / beam_duration;
    // Fire a beam.
    const double beam_damage = 25.0 * time / beam_duration;
    const double beam_angle = baddie->angle + baddie->components[0].angle;
    const az_vector_t beam_start =
      az_vadd(baddie->position, az_vpolar(15, beam_angle));
    az_impact_t impact;
    az_ray_impact(state, beam_start, az_vpolar(5000, beam_angle),
                  AZ_IMPF_NONE, baddie->uid, &impact);
    if (impact.type == AZ_IMP_BADDIE) {
      az_try_damage_baddie(state, impact.target.baddie.baddie,
          impact.target.baddie.component, AZ_DMGF_BEAM, beam_damage);
    } else if (impact.type == AZ_IMP_SHIP) {
      az_damage_ship(state, beam_damage, false);
    }
    // Add particles for the beam.
    const uint8_t alt = 32 * az_clock_zigzag(6, 1, state->clock);
    const az_color_t beam_color = {255, alt, 128, 192};
    az_add_beam(state, beam_color, beam_start, impact.position, 0.0,
                (6.0 + 0.5 * az_clock_zigzag(8, 1, state->clock)) *
                baddie->cooldown);
    az_add_speck(state, (impact.type == AZ_IMP_WALL ?
                         impact.target.wall->data->color1 :
                         AZ_WHITE), 1.0, impact.position,
                 az_vpolar(az_random(20.0, 70.0),
                           az_vtheta(impact.normal) +
                           az_random(-AZ_HALF_PI, AZ_HALF_PI)));
    // When the cooldown timer reachers zero, stop firing the beam.
    if (baddie->cooldown <= 0.0) {
      baddie->state = 1;
      baddie->cooldown = 1.0;
    }
  }
}

/*===========================================================================*/

// How long it takes a baddie's armor flare to die down, in seconds:
#define AZ_BADDIE_ARMOR_FLARE_TIME 0.3
// How long it takes a baddie to unfreeze, in seconds.
#define AZ_BADDIE_THAW_TIME 8.0

static void tick_baddie(az_space_state_t *state, az_baddie_t *baddie,
                        double time) {
  // Reset the baddie's temporary properties.
  baddie->temp_properties = 0;

  // Allow the armor flare to die down a bit.
  assert(baddie->armor_flare >= 0.0);
  assert(baddie->armor_flare <= 1.0);
  baddie->armor_flare =
    fmax(0.0, baddie->armor_flare - time / AZ_BADDIE_ARMOR_FLARE_TIME);

  // Allow the baddie to thaw a bit.  If we're in a superheated room, thaw
  // faster than usual.
  assert(baddie->frozen >= 0.0);
  assert(baddie->frozen <= 1.0);
  const double thaw_rate =
    (state->planet->rooms[state->ship.player.current_room].properties &
     AZ_ROOMF_HEATED) ? 3.0 / AZ_BADDIE_THAW_TIME : 1.0 / AZ_BADDIE_THAW_TIME;
  baddie->frozen = fmax(0.0, baddie->frozen - thaw_rate * time);
  if (baddie->frozen > 0.0) {
    baddie->velocity = AZ_VZERO;
    return;
  }

  // Cool down the baddie's weapon.
  baddie->cooldown = fmax(0.0, baddie->cooldown - time);

  // Apply velocity.
  const az_vector_t old_baddie_position = baddie->position;
  const double old_baddie_angle = baddie->angle;
  bool bounced = false;
  if (az_vnonzero(baddie->velocity)) {
    az_impact_t impact;
    az_circle_impact(state, baddie->data->main_body.bounding_radius,
                     baddie->position, az_vmul(baddie->velocity, time),
                     (AZ_IMPF_BADDIE | AZ_IMPF_SHIP), baddie->uid, &impact);
    baddie->position = impact.position;
    if (impact.type != AZ_IMP_NOTHING) {
      // Baddies with the BOUNCE_PERP flag always bounce perfectly backwards
      // (as if they hit the wall dead-on), even if they hit a wall at an
      // oblique angle.
      if (az_baddie_has_flag(baddie, AZ_BADF_BOUNCE_PERP)) {
        impact.normal = az_vproj(impact.normal, baddie->velocity);
      }
      // Push the baddie slightly away from the impact point (so that we're
      // hopefully no longer in contact with the object we hit).
      az_vpluseq(&baddie->position, az_vwithlen(impact.normal, 0.5));
      // Bounce the baddie off the object.
      baddie->velocity =
        az_vsub(baddie->velocity,
                az_vmul(az_vproj(baddie->velocity, impact.normal), 1.5));
      bounced = true;
    }
  }

  // Perform kind-specific logic.
  switch (baddie->kind) {
    case AZ_BAD_NOTHING: AZ_ASSERT_UNREACHABLE();
    case AZ_BAD_MARKER: break; // Do nothing.
    case AZ_BAD_NORMAL_TURRET:
    case AZ_BAD_ARMORED_TURRET:
      az_tick_bad_turret(state, baddie, time);
      break;
    case AZ_BAD_ZIPPER:
    case AZ_BAD_ARMORED_ZIPPER:
    case AZ_BAD_MINI_ARMORED_ZIPPER:
      az_tick_bad_zipper(state, baddie, bounced);
      break;
    case AZ_BAD_BOUNCER:
      if (az_vnonzero(baddie->velocity)) {
        baddie->angle = az_vtheta(baddie->velocity);
      }
      baddie->velocity = az_vpolar(150.0, baddie->angle);
      break;
    case AZ_BAD_ATOM:
      az_drift_towards_ship(state, baddie, time, 70, 100, 100);
      for (int i = 0; i < baddie->data->num_components; ++i) {
        az_component_t *component = &baddie->components[i];
        component->angle = az_mod2pi(component->angle + 3.5 * time);
        component->position = az_vpolar(4.0, component->angle);
        component->position.x *= 5.0;
        component->position =
          az_vrotate(component->position, i * AZ_DEG2RAD(120));
      }
      break;
    case AZ_BAD_SPINER:
      az_drift_towards_ship(state, baddie, time, 40, 10, 100);
      baddie->angle = az_mod2pi(baddie->angle + 0.4 * time);
      if (baddie->cooldown <= 0.0 && az_ship_in_range(state, baddie, 200) &&
          az_can_see_ship(state, baddie)) {
        for (int i = 0; i < 360; i += 45) {
          az_fire_baddie_projectile(
              state, baddie, AZ_PROJ_SPINE,
              baddie->data->main_body.bounding_radius, AZ_DEG2RAD(i), 0.0);
        }
        baddie->cooldown = 2.0;
      }
      break;
    case AZ_BAD_BOX: break; // Do nothing.
    case AZ_BAD_ARMORED_BOX: break; // Do nothing.
    case AZ_BAD_CLAM:
      {
        const bool can_see = az_can_see_ship(state, baddie);
        const double ship_theta =
          az_vtheta(az_vsub(state->ship.position, baddie->position));
        if (can_see) {
          baddie->angle =
            az_angle_towards(baddie->angle, 0.2 * time, ship_theta);
        }
        const double max_angle = AZ_DEG2RAD(40);
        const double old_angle = baddie->components[0].angle;
        const double new_angle =
          (baddie->cooldown <= 0.0 && can_see &&
           fabs(az_mod2pi(baddie->angle - ship_theta)) < AZ_DEG2RAD(10) ?
           fmin(max_angle, max_angle -
                (max_angle - old_angle) * pow(0.00003, time)) :
           fmax(0.0, old_angle - 1.0 * time));
        baddie->components[0].angle = new_angle;
        baddie->components[1].angle = -new_angle;
        if (baddie->cooldown <= 0.0 && new_angle >= 0.95 * max_angle) {
          for (int i = -1; i <= 1; ++i) {
            az_fire_baddie_projectile(state, baddie,
                (i == 0 ? AZ_PROJ_FIREBALL_SLOW : AZ_PROJ_FIREBALL_FAST),
                0.5 * baddie->data->main_body.bounding_radius,
                AZ_DEG2RAD(i * 5), 0.0);
          }
          az_play_sound(&state->soundboard, AZ_SND_FIRE_FIREBALL);
          baddie->cooldown = 3.0;
        }
      }
      break;
    case AZ_BAD_NIGHTBUG:
      az_fly_towards_ship(state, baddie, time,
                          3.0, 40.0, 100.0, 20.0, 100.0, 100.0);
      if (baddie->state == 0) {
        baddie->param = fmax(0.0, baddie->param - time / 3.5);
        if (baddie->cooldown < 0.5 && az_ship_in_range(state, baddie, 250) &&
            az_ship_within_angle(state, baddie, 0, AZ_DEG2RAD(3)) &&
            az_can_see_ship(state, baddie)) {
          baddie->state = 1;
        }
      } else if (baddie->state == 1) {
        baddie->param = fmin(1.0, baddie->param + time / 0.5);
        if (baddie->cooldown <= 0.0 && baddie->param == 1.0) {
          for (int i = 0; i < 2; ++i) {
            az_fire_baddie_projectile(
                state, baddie, AZ_PROJ_FIREBALL_FAST, 20.0, 0.0,
                az_random(-AZ_DEG2RAD(10), AZ_DEG2RAD(10)));
          }
          baddie->cooldown = 5.0;
          baddie->state = 0;
        }
      } else baddie->state = 0;
      break;
    case AZ_BAD_SPINE_MINE:
      az_drift_towards_ship(state, baddie, time, 20, 20, 20);
      baddie->angle = az_mod2pi(baddie->angle - 0.5 * time);
      if (az_ship_in_range(state, baddie, 150) &&
          az_can_see_ship(state, baddie)) {
        for (int i = 0; i < 360; i += 20) {
          az_fire_baddie_projectile(state, baddie, AZ_PROJ_SPINE,
              baddie->data->main_body.bounding_radius,
              AZ_DEG2RAD(i) + az_random(AZ_DEG2RAD(-10), AZ_DEG2RAD(10)), 0.0);
        }
        az_play_sound(&state->soundboard, AZ_SND_KILL_BOUNCER);
        baddie->kind = AZ_BAD_NOTHING;
      }
      break;
    case AZ_BAD_BROKEN_TURRET:
      az_tick_bad_broken_turret(state, baddie, time);
      break;
    case AZ_BAD_ZENITH_CORE:
      // Lie dormant for 31 seconds.
      if (baddie->state == 0) {
        baddie->state = 1;
        baddie->cooldown = 31.0;
      }
      if (baddie->cooldown <= 0.0) {
        // Turn towards the ship.
        baddie->angle = az_angle_towards(
            baddie->angle, AZ_DEG2RAD(30) * time,
            az_vtheta(az_vsub(state->ship.position, baddie->position)));
        // Fire a beam, piercing through the ship and other baddies.
        const az_vector_t beam_start =
          az_vadd(baddie->position, az_vpolar(120, baddie->angle));
        az_impact_t impact;
        az_ray_impact(state, beam_start, az_vpolar(1000, baddie->angle),
                      (AZ_IMPF_BADDIE | AZ_IMPF_SHIP), baddie->uid, &impact);
        const az_vector_t beam_delta = az_vsub(impact.position, beam_start);
        const double beam_damage = 300.0 * time;
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
                                 (AZ_DMGF_PIERCE | AZ_DMGF_BEAM), beam_damage);
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
      break;
    case AZ_BAD_DRAGONFLY:
      az_tick_bad_dragonfly(state, baddie, time);
      break;
    case AZ_BAD_CAVE_CRAWLER:
      az_tick_bad_cave_crawler(state, baddie, time);
      break;
    case AZ_BAD_CRAWLING_TURRET:
      az_tick_bad_crawling_turret(state, baddie, time);
      break;
    case AZ_BAD_HORNET:
      az_tick_bad_hornet(state, baddie, time);
      break;
    case AZ_BAD_BEAM_SENSOR:
    case AZ_BAD_GUN_SENSOR:
      if (baddie->state == 0) {
        if (baddie->health < baddie->data->max_health) {
          baddie->state = 1;
          az_schedule_script(state, baddie->on_kill);
        }
      }
      baddie->health = baddie->data->max_health;
      break;
    case AZ_BAD_ROCKWYRM:
      az_tick_bad_rockwyrm(state, baddie, time);
      break;
    case AZ_BAD_WYRM_EGG:
      az_tick_bad_wyrm_egg(state, baddie, time);
      break;
    case AZ_BAD_WYRMLING:
      az_tick_bad_wyrmling(state, baddie, time);
      break;
    case AZ_BAD_TRAPDOOR:
      if (!az_ship_in_range(state, baddie,
                            baddie->data->overall_bounding_radius +
                            AZ_SHIP_DEFLECTOR_RADIUS)) {
        baddie->components[0].angle =
          az_angle_towards(baddie->components[0].angle, AZ_DEG2RAD(600) * time,
                           (az_ship_in_range(state, baddie, 210) ?
                            AZ_DEG2RAD(90) : 0));
      }
      break;
    case AZ_BAD_SWOOPER:
      // State 0: Perch on the nearest wall, then go to state 1.
      if (baddie->state == 0) {
        if (perch_on_ceiling(state, baddie, time)) {
          baddie->cooldown = 2.0;
          baddie->state = 1;
        }
      }
      // State 1: Sit and wait until the ship is nearby, then go to state 2.
      else if (baddie->state == 1) {
        if (baddie->cooldown <= 0.0 && az_ship_in_range(state, baddie, 250) &&
            az_can_see_ship(state, baddie)) {
          baddie->param = 3.5;
          baddie->state = 2;
        }
      }
      // State 2: Chase the ship for up to a few seconds, then go to state 0.
      else if (baddie->state == 2) {
        if (az_ship_is_decloaked(&state->ship)) {
          az_fly_towards_ship(state, baddie, time,
                              5.0, 500.0, 300.0, 250.0, 0.0, 100.0);
          baddie->param = fmax(0.0, baddie->param - time);
        } else baddie->param = 0.0;
        if (baddie->param <= 0.0) baddie->state = 0;
      } else baddie->state = 0;
      break;
    case AZ_BAD_ICE_CRAWLER:
      az_tick_bad_ice_crawler(state, baddie, time);
      break;
    case AZ_BAD_BEAM_TURRET:
      az_tick_bad_beam_turret(state, baddie, time);
      break;
    case AZ_BAD_OTH_CRAB_1:
      az_tick_bad_oth_crab_1(state, baddie, time);
      break;
    case AZ_BAD_OTH_ORB_1:
      az_tick_bad_oth_orb_1(state, baddie, time);
      break;
    case AZ_BAD_OTH_SNAPDRAGON:
      az_tick_bad_oth_snapdragon(state, baddie, time);
      break;
    case AZ_BAD_OTH_RAZOR:
      az_tick_bad_oth_razor(state, baddie, time);
      break;
    case AZ_BAD_SECURITY_DRONE:
      az_tick_bad_security_drone(state, baddie, time);
      break;
    case AZ_BAD_SMALL_TRUCK:
      // States 0 and 1: fly forward.
      if (baddie->state == 0 || baddie->state == 1) {
        const double max_speed = 100.0, accel = 50.0;
        const double min_dist = 100.0, max_dist = 200.0;
        const double dist =
          az_baddie_dist_to_wall(state, baddie, max_dist + 1.0, 0.0);
        const double fraction =
          fmin(1.0, fmax(0.0, dist - min_dist) / (max_dist - min_dist));
        const double speed_limit = max_speed * sqrt(fraction);
        const double speed =
          fmin(speed_limit, az_vnorm(baddie->velocity) + accel * time);
        baddie->state = (speed >= max_speed || speed < speed_limit ? 1 : 0);
        baddie->velocity = az_vpolar(speed, baddie->angle);
        if (fraction <= 0.0) {
          baddie->param = az_mod2pi(baddie->angle + AZ_HALF_PI);
          baddie->state = 2;
        }
      }
      // State 2: Turn.
      else if (baddie->state == 2) {
        baddie->velocity = AZ_VZERO;
        const double turn_rate = AZ_DEG2RAD(50);
        const double goal_angle = baddie->param;
        baddie->angle =
          az_angle_towards(baddie->angle, turn_rate * time, goal_angle);
        if (baddie->angle == goal_angle) baddie->state = 0;
      } else baddie->state = 0;
      break;
    case AZ_BAD_HEAT_RAY:
      // State 0: Fire beam until cooldown expires.
      if (baddie->state == 0) {
        if (baddie->cooldown > 0.0) {
          // Fire a beam.
          double beam_damage = 38.0 * time;
          const az_vector_t beam_start =
            az_vadd(baddie->position, az_vpolar(15, baddie->angle));
          az_impact_t impact;
          az_ray_impact(state, beam_start, az_vpolar(5000, baddie->angle),
                        AZ_IMPF_NONE, baddie->uid, &impact);
          if (impact.type == AZ_IMP_BADDIE) {
            if (impact.target.baddie.baddie->kind == AZ_BAD_BEAM_WALL) {
              beam_damage /= 3;
            }
            az_try_damage_baddie(state, impact.target.baddie.baddie,
                impact.target.baddie.component, AZ_DMGF_BEAM, beam_damage);
          } else if (impact.type == AZ_IMP_SHIP) {
            az_damage_ship(state, beam_damage, false);
          }
          // Add particles for the beam.
          const uint8_t alt = 32 * az_clock_zigzag(6, 1, state->clock);
          const az_color_t beam_color = {255, 128, alt, 192};
          az_add_beam(state, beam_color, beam_start, impact.position, 0.0,
                      4.0 + 0.5 * az_clock_zigzag(8, 1, state->clock));
          az_add_speck(state, (impact.type == AZ_IMP_WALL ?
                               impact.target.wall->data->color1 :
                               AZ_WHITE), 1.0, impact.position,
                       az_vpolar(az_random(20.0, 70.0),
                                 az_vtheta(impact.normal) +
                                 az_random(-AZ_HALF_PI, AZ_HALF_PI)));
          az_loop_sound(&state->soundboard, AZ_SND_BEAM_FREEZE);
        } else {
          baddie->state = 1;
          baddie->cooldown = az_random(0.5, 3.0);
        }
      }
      // State 1: Recharge until cooldown expires.
      else {
        if (baddie->cooldown <= 0.0) {
          baddie->state = 0;
          baddie->cooldown = 1.5;
        }
      }
      break;
    case AZ_BAD_NUCLEAR_MINE:
      baddie->angle = az_mod2pi(baddie->angle + AZ_DEG2RAD(90) * time);
      // State 0: Wait for ship.
      if (baddie->state == 0) {
        if (az_ship_in_range(state, baddie, 150) &&
            az_can_see_ship(state, baddie)) {
          baddie->state = 1;
          baddie->cooldown = 0.9;
          az_play_sound(&state->soundboard, AZ_SND_BLINK_MEGA_BOMB);
        }
      }
      // State 1: Explode when cooldown reaches zero.
      else {
        if (baddie->cooldown <= 0.0) {
          az_fire_baddie_projectile(state, baddie, AZ_PROJ_NUCLEAR_EXPLOSION,
                                    0.0, 0.0, 0.0);
          az_kill_baddie(state, baddie);
        }
      }
      break;
    case AZ_BAD_BEAM_WALL: break; // Do nothing.
    case AZ_BAD_SPARK:
      if (az_random(0, 1) < 10.0 * time) {
        az_add_speck(
            state, (az_color_t){0, 255, 0, 255}, 1.0, baddie->position,
            az_vpolar(az_random(20.0, 70.0), baddie->angle +
                      az_random(AZ_DEG2RAD(-120), AZ_DEG2RAD(120))));
      }
      if (az_random(0, 1) < time) {
        const double angle = az_random(AZ_DEG2RAD(-135), AZ_DEG2RAD(135));
        az_fire_baddie_projectile(state, baddie, AZ_PROJ_SPARK,
                                  0.0, 0.0, angle);
        for (int i = 0; i < 5; ++i) {
          az_add_speck(
              state, (az_color_t){0, 255, 0, 255}, 1.0, baddie->position,
              az_vpolar(az_random(20.0, 70.0), baddie->angle + angle +
                        az_random(AZ_DEG2RAD(-60), AZ_DEG2RAD(60))));
        }
      }
      break;
    case AZ_BAD_MOSQUITO:
      az_tick_bad_mosquito(state, baddie, time);
      break;
    case AZ_BAD_FORCEFIEND:
      az_tick_bad_forcefiend(state, baddie, time);
      break;
    case AZ_BAD_CHOMPER_PLANT:
      az_tick_bad_chomper_plant(state, baddie, time);
      break;
    case AZ_BAD_COPTER_HORZ:
    case AZ_BAD_COPTER_VERT:
      if (baddie->cooldown <= 0.0) {
        const double max_speed = 150.0, accel = 50.0, max_dist = 200.0;
        const double min_dist =
          (baddie->kind == AZ_BAD_COPTER_VERT ? 50.0 : 100.0);
        const double rel_angle =
          (baddie->kind == AZ_BAD_COPTER_VERT ?
           (baddie->state == 0 ? 0 : AZ_PI) :
           (baddie->state == 0 ? AZ_HALF_PI : -AZ_HALF_PI));
        const double dist =
          az_baddie_dist_to_wall(state, baddie, max_dist + 1.0, rel_angle);
        const double fraction =
          fmin(1.0, fmax(0.0, dist - min_dist) / (max_dist - min_dist));
        const double speed_limit = max_speed * sqrt(fraction);
        const double speed =
          fmin(speed_limit, az_vnorm(baddie->velocity) + accel * time);
        baddie->velocity = az_vpolar(speed, baddie->angle + rel_angle);
        if (fraction <= 0.0) {
          baddie->velocity = AZ_VZERO;
          baddie->state = (baddie->state == 0 ? 1 : 0);
          baddie->cooldown = 1.0;
        }
      }
      break;
    case AZ_BAD_URCHIN:
      az_drift_towards_ship(state, baddie, time, 250, 300, 500);
      baddie->angle = az_mod2pi(baddie->angle + AZ_DEG2RAD(50) * time);
      break;
    case AZ_BAD_BOSS_DOOR:
      tick_boss_door(state, baddie, time);
      break;
    case AZ_BAD_ROCKET_TURRET:
      az_tick_bad_rocket_turret(state, baddie, time);
      break;
    case AZ_BAD_OTH_CRAB_2:
      az_tick_bad_oth_crab_2(state, baddie, time);
      break;
    case AZ_BAD_SPINED_CRAWLER:
      az_tick_bad_spined_crawler(state, baddie, time);
      break;
    case AZ_BAD_DEATH_RAY:
      if (baddie->state == 0 || baddie->state == 1) {
        // Fire a beam.
        const az_vector_t beam_start =
          az_vadd(baddie->position, az_vpolar(15, baddie->angle));
        az_impact_t impact;
        az_ray_impact(state, beam_start, az_vpolar(5000, baddie->angle),
                      AZ_IMPF_NONE, baddie->uid, &impact);
        if (impact.type == AZ_IMP_SHIP) {
          baddie->state = 1;
          const double beam_damage = 200.0 * time;
          az_damage_ship(state, beam_damage, false);
          // Add particles for the beam.
          const uint8_t alt = 32 * az_clock_zigzag(4, 1, state->clock);
          const az_color_t beam_color = {255, 255, 128 + alt, 192};
          az_add_beam(state, beam_color, beam_start, impact.position, 0.0,
                      4.0 + 0.5 * az_clock_zigzag(8, 1, state->clock));
          az_add_speck(state, (impact.type == AZ_IMP_WALL ?
                               impact.target.wall->data->color1 :
                               AZ_WHITE), 1.0, impact.position,
                       az_vpolar(az_random(20.0, 70.0),
                                 az_vtheta(impact.normal) +
                                 az_random(-AZ_HALF_PI, AZ_HALF_PI)));
          az_loop_sound(&state->soundboard, AZ_SND_BEAM_PIERCE);
        } else {
          baddie->state = 0;
          if (az_clock_mod(2, 2, state->clock)) {
            const az_color_t beam_color = {255, 128, 128, 128};
            az_add_beam(state, beam_color, beam_start, impact.position,
                        0.0, 1.0);
          }
        }
      }
      break;
    case AZ_BAD_OTH_GUNSHIP:
      az_tick_bad_oth_gunship(state, baddie, time);
      break;
    case AZ_BAD_FIREBALL_MINE:
      if (baddie->state == 0) {
        if (az_ship_in_range(state, baddie, 200) &&
            az_can_see_ship(state, baddie)) {
          baddie->cooldown = 0.9;
          baddie->state = 1;
        }
      } else if (baddie->cooldown <= 0.0) {
        for (int i = 0; i < 360; i += 10) {
          az_fire_baddie_projectile(
              state, baddie, (az_randint(0, 1) ? AZ_PROJ_FIREBALL_SLOW :
                              AZ_PROJ_FIREBALL_FAST),
              baddie->data->main_body.bounding_radius, AZ_DEG2RAD(i), 0.0);
        }
        assert(!(baddie->data->main_body.immunities & AZ_DMGF_BOMB));
        az_try_damage_baddie(state, baddie, &baddie->data->main_body,
                             AZ_DMGF_BOMB, baddie->data->max_health);
        assert(baddie->kind == AZ_BAD_NOTHING);
      }
      break;
    case AZ_BAD_LEAPER:
      if (baddie->state == 0) {
        if (baddie->cooldown <= 0.0 && az_ship_in_range(state, baddie, 500)) {
          if (az_baddie_has_clear_path_to_position(state, baddie,
                                                   state->ship.position)) {
            baddie->angle =
              az_vtheta(az_vsub(state->ship.position, baddie->position));
            baddie->velocity = az_vpolar(500, baddie->angle);
            baddie->state = 1;
          } else {
            az_crawl_around(
                state, baddie, time, az_ship_is_decloaked(&state->ship) &&
                az_vcross(az_vsub(state->ship.position, baddie->position),
                          az_vpolar(1.0, baddie->angle)) > 0.0,
                1.0, 20.0, 100.0);
          }
        }
      } else {
        baddie->angle =
          az_angle_towards(baddie->angle, AZ_DEG2RAD(20) * time,
                           az_vtheta(az_vneg(baddie->position)));
        if (perch_on_wall_ahead(state, baddie, time)) {
          baddie->state = 0;
          baddie->cooldown = 1.0;
        }
      }
      break;
    case AZ_BAD_BOUNCER_90:
      if (az_vnonzero(baddie->velocity)) {
        if (bounced) {
          baddie->velocity =
            az_vflatten(baddie->velocity, az_vpolar(1, baddie->angle));
        }
        baddie->angle = az_vtheta(baddie->velocity);
      }
      baddie->velocity = az_vpolar(170.0, baddie->angle);
      break;
    case AZ_BAD_PISTON:
    case AZ_BAD_ARMORED_PISTON:
    case AZ_BAD_ARMORED_PISTON_EXT:
    case AZ_BAD_INCORPOREAL_PISTON:
    case AZ_BAD_INCORPOREAL_PISTON_EXT: {
      const az_vector_t base_pos =
        az_vadd(baddie->position, az_vrotate(baddie->components[2].position,
                                             baddie->angle));
      const double old_extension = fabs(baddie->components[2].position.x);
      const double max_extension = 90.0;
      // Change how extended the piston is:
      double goal_extension;
      if (baddie->state >= 0 && baddie->state <= 8) {
        goal_extension = max_extension * 0.125 * baddie->state;
        if (baddie->kind == AZ_BAD_ARMORED_PISTON_EXT ||
            baddie->kind == AZ_BAD_INCORPOREAL_PISTON_EXT) {
          goal_extension = max_extension - goal_extension;
        }
      } else goal_extension = old_extension;
      const double tracking_base = 0.05; // smaller = faster tracking
      const double change =
        (goal_extension - old_extension) * (1.0 - pow(tracking_base, time));
      const double new_extension =
        (fabs(change) < 0.001 ? goal_extension :
         fmin(fmax(0.0, old_extension + change), max_extension));
      // Update positions of segments:
      if (new_extension != old_extension) {
        const az_vector_t new_head_pos =
          az_vadd(base_pos, az_vpolar(new_extension, baddie->angle));
        for (int i = 0; i < 3; ++i) {
          baddie->components[i].position.x = -new_extension * (i + 1) / 3.0;
        }
        baddie->position = new_head_pos;
      }
      // If any of the piston's cargo is destroyed, the piston is destroyed:
      AZ_ARRAY_LOOP(uuid, baddie->cargo_uuids) {
        if (uuid->type == AZ_UUID_NOTHING) continue;
        az_object_t object;
        if (!az_lookup_object(state, *uuid, &object)) {
          az_kill_baddie(state, baddie);
          break;
        }
      }
    } break;
    case AZ_BAD_CRAWLING_MORTAR:
      az_tick_bad_crawling_mortar(state, baddie, time);
      break;
    case AZ_BAD_OTH_ORB_2:
      az_tick_bad_oth_orb_2(state, baddie, time);
      break;
    case AZ_BAD_FIRE_ZIPPER:
      az_tick_bad_fire_zipper(state, baddie, bounced);
      break;
    case AZ_BAD_SUPER_SPINER:
      az_drift_towards_ship(state, baddie, time, 50, 20, 100);
      baddie->angle = az_mod2pi(baddie->angle - 0.5 * time);
      if (baddie->cooldown <= 0.0 &&
          az_ship_in_range(state, baddie, 200) &&
          az_can_see_ship(state, baddie)) {
        for (int i = 0; i < 360; i += 45) {
          az_fire_baddie_projectile(
              state, baddie, AZ_PROJ_SPINE,
              baddie->data->main_body.bounding_radius, AZ_DEG2RAD(i), 0.0);
        }
        baddie->cooldown = 1.0;
        baddie->state = 1;
      }
      if (baddie->state == 1 && baddie->cooldown <= 0.75) {
        for (int i = 0; i < 360; i += 45) {
          az_fire_baddie_projectile(
              state, baddie, AZ_PROJ_SPINE,
              baddie->data->main_body.bounding_radius,
              AZ_DEG2RAD(i + 22.5), 0.0);
        }
        baddie->cooldown = 1.0;
        baddie->state = 0;
      }
      break;
    case AZ_BAD_HEAVY_TURRET:
      az_tick_bad_heavy_turret(state, baddie, time);
      break;
    case AZ_BAD_ECHO_SWOOPER:
      // State 0: Perch on the nearest wall, then go to state 1.
      if (baddie->state == 0) {
        if (perch_on_ceiling(state, baddie, time)) {
          baddie->cooldown = 2.0;
          baddie->state = 1;
        }
      }
      // State 1: Sit and wait until the ship is nearby, then go to state 2.
      else if (baddie->state == 1) {
        if (baddie->cooldown <= 0.0 && az_ship_in_range(state, baddie, 250) &&
            az_can_see_ship(state, baddie)) {
          baddie->param = 6.0;
          baddie->state = 2;
          baddie->cooldown = 0.5;
        }
      }
      // State 2: Chase the ship for up to a few seconds, then go to state 0.
      else if (baddie->state == 2) {
        if (az_ship_is_decloaked(&state->ship)) {
          if (baddie->cooldown <= 0.0 &&
              az_ship_in_range(state, baddie, 200) &&
              az_ship_within_angle(state, baddie, 0, AZ_DEG2RAD(10))) {
            double theta = 0.0;
            for (int i = 0; i < 25; ++i) {
              az_projectile_t *proj = az_fire_baddie_projectile(
                  state, baddie, AZ_PROJ_SONIC_WAVE, 8, 0, theta);
              if (proj == NULL) break;
              theta = -theta;
              if (i % 2 == 0) theta += AZ_DEG2RAD(1);
            }
            az_play_sound(&state->soundboard, AZ_SND_SONIC_SCREECH);
            baddie->cooldown = 2.0;
          }
          az_fly_towards_ship(state, baddie, time,
                              5.0, 350.0, 300.0, 250.0, 50.0, 100.0);
          baddie->param = fmax(0.0, baddie->param - time);
        } else baddie->param = 0.0;
        if (baddie->param <= 0.0) baddie->state = 0;
      } else baddie->state = 0;
      break;
    case AZ_BAD_SUPER_HORNET:
      az_tick_bad_super_hornet(state, baddie, time);
      break;
    case AZ_BAD_KILOFUGE:
      az_tick_bad_kilofuge(state, baddie, time);
      break;
    case AZ_BAD_ICE_CRYSTAL: break; // Do nothing.
    case AZ_BAD_SWITCHER:
      az_tick_bad_switcher(state, baddie, bounced);
      break;
    case AZ_BAD_FAST_BOUNCER:
      if (az_vnonzero(baddie->velocity)) {
        baddie->angle = az_vtheta(baddie->velocity);
      }
      baddie->velocity = az_vpolar(385.0, baddie->angle);
      break;
    case AZ_BAD_PROXY_MINE:
      baddie->angle = az_mod2pi(baddie->angle - AZ_DEG2RAD(120) * time);
      // State 0: Wait for ship.
      if (baddie->state == 0) {
        if (az_ship_in_range(state, baddie, 80) &&
            az_can_see_ship(state, baddie)) {
          baddie->state = 1;
          baddie->cooldown = 0.35;
          az_play_sound(&state->soundboard, AZ_SND_BLINK_MEGA_BOMB);
        }
      }
      // State 1: Explode when cooldown reaches zero.
      else {
        if (baddie->cooldown <= 0.0) {
          az_fire_baddie_projectile(state, baddie, AZ_PROJ_MEDIUM_EXPLOSION,
                                    0.0, 0.0, 0.0);
          az_kill_baddie(state, baddie);
        }
      }
      break;
    case AZ_BAD_NIGHTSHADE: {
      az_fly_towards_ship(state, baddie, time,
                          4.0, 100.0, 100.0, 80.0, 30.0, 100.0);
      double mandibles_turn_rate = AZ_DEG2RAD(30);
      double goal_mandibles_angle = AZ_DEG2RAD(80);
      if (baddie->state == 0) {
        baddie->param = fmax(0.0, baddie->param - time / 2.5);
        if (baddie->cooldown <= 1.0 && az_ship_in_range(state, baddie, 50) &&
            az_ship_within_angle(state, baddie, 0, AZ_DEG2RAD(6)) &&
            az_can_see_ship(state, baddie)) {
          baddie->state = 1;
        }
      } else if (baddie->state == 1) {
        baddie->param = fmin(1.0, baddie->param + time / 0.75);
        if (baddie->param > 0.5) {
          goal_mandibles_angle = AZ_DEG2RAD(0);
          mandibles_turn_rate = AZ_DEG2RAD(360);
        }
        if (baddie->cooldown <= 0.0 && baddie->param == 1.0) {
          baddie->cooldown = 5.0;
          baddie->state = 0;
        }
      } else baddie->state = 0;
      for (int i = 0; i < 2; ++i) {
        baddie->components[i].angle = az_angle_towards(
            baddie->components[i].angle, time * mandibles_turn_rate,
            goal_mandibles_angle);
        goal_mandibles_angle = -goal_mandibles_angle;
      }
    } break;
    case AZ_BAD_AQUATIC_CHOMPER:
      az_tick_bad_aquatic_chomper(state, baddie, time);
      break;
    case AZ_BAD_SMALL_FISH:
      // TODO(mdsteele): Prevent fish from leaving water.
      // If the ship is nearby and visible, chase it.
      if (az_ship_in_range(state, baddie, 180) &&
          az_can_see_ship(state, baddie)) {
        az_snake_towards(baddie, time, 0, 100.0, 150.0, state->ship.position);
      } else {
        // Otherwise, we'll head in one of eight directions, determined by the
        // baddie's current state.
        const double abs_angle = az_vtheta(baddie->position) +
          AZ_DEG2RAD(45) * az_modulo(baddie->state, 8);
        const double rel_angle = az_mod2pi(abs_angle - baddie->angle);
        const az_vector_t dest =
          az_vadd(baddie->position, az_vpolar(300.0, abs_angle));
        // Check if we're about to hit a wall.
        const double dist =
          az_baddie_dist_to_wall(state, baddie, 100.0, rel_angle);
        const az_camera_bounds_t *bounds =
          &state->planet->rooms[state->ship.player.current_room].camera_bounds;
        // If we've somehow ended up within a wall (or nearly so) or outside
        // the room, head back to the ship position to try to fix it.
        if (dist < 1.0 || !az_position_visible(bounds, baddie->position)) {
          az_snake_towards(baddie, time, 0, 70.0, 150.0, state->ship.position);
          baddie->state = az_randint(0, 7);
        } else {
          // Otherwise head towards our current destination.
          az_snake_towards(baddie, time, 0, 70.0, 150.0, dest);
          // If we're getting near a wall ahead, turn back.
          if (dist <= 90.0) {
            const int shift = (az_randint(0, 1) ? 3 : -3);
            baddie->state = az_modulo(baddie->state + shift, 8);
          }
        }
      }
      break;
    case AZ_BAD_NOCTURNE:
      az_tick_bad_nocturne(state, baddie, time);
      break;
    case AZ_BAD_MYCOFLAKKER:
      az_tick_bad_mycoflakker(state, baddie, time);
      break;
    case AZ_BAD_MYCOSTALKER:
      az_tick_bad_mycostalker(state, baddie, time);
      break;
    case AZ_BAD_OTH_CRAWLER:
      az_tick_bad_oth_crawler(state, baddie, time);
      break;
    case AZ_BAD_FIRE_CRAWLER:
      az_tick_bad_fire_crawler(state, baddie, time);
      break;
    case AZ_BAD_JUNGLE_CRAWLER:
      az_tick_bad_jungle_crawler(state, baddie, time);
      break;
  }

  // Move cargo with the baddie (unless the baddie killed itself).
  if (baddie->kind != AZ_BAD_NOTHING &&
      az_baddie_has_flag(baddie, AZ_BADF_CARRIES_CARGO)) {
    az_move_baddie_cargo(
        state, baddie,
        az_vsub(baddie->position, old_baddie_position),
        az_mod2pi(baddie->angle - old_baddie_angle));
  }
}

void az_tick_baddies(az_space_state_t *state, double time) {
  AZ_ARRAY_LOOP(baddie, state->baddies) {
    if (baddie->kind == AZ_BAD_NOTHING) continue;
    assert(baddie->health > 0.0);
    tick_baddie(state, baddie, time);
  }
}

/*===========================================================================*/

void az_on_baddie_killed(az_space_state_t *state, az_baddie_kind_t kind,
                         az_vector_t position, double angle) {
  switch (kind) {
    case AZ_BAD_SUPER_SPINER:
      for (int i = 0; i < 3; ++i) {
        const double theta = angle + i * AZ_DEG2RAD(120);
        az_add_baddie(state, AZ_BAD_URCHIN,
                      az_vadd(position, az_vpolar(10, theta)), theta);
      }
      break;
    case AZ_BAD_JUNGLE_CRAWLER:
      az_on_jungle_crawler_killed(state, position, angle);
      break;
    default: break;
  }
}

/*===========================================================================*/
