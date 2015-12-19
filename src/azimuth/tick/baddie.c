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
#include "azimuth/tick/baddie_bouncer.h"
#include "azimuth/tick/baddie_chomper.h"
#include "azimuth/tick/baddie_clam.h"
#include "azimuth/tick/baddie_core.h"
#include "azimuth/tick/baddie_crawler.h"
#include "azimuth/tick/baddie_forcefiend.h"
#include "azimuth/tick/baddie_kilofuge.h"
#include "azimuth/tick/baddie_machine.h"
#include "azimuth/tick/baddie_magbeest.h"
#include "azimuth/tick/baddie_myco.h"
#include "azimuth/tick/baddie_nocturne.h"
#include "azimuth/tick/baddie_oth.h"
#include "azimuth/tick/baddie_oth_gunship.h"
#include "azimuth/tick/baddie_oth_sgs.h"
#include "azimuth/tick/baddie_spiner.h"
#include "azimuth/tick/baddie_turret.h"
#include "azimuth/tick/baddie_util.h"
#include "azimuth/tick/baddie_vehicle.h"
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
    az_impact_flags_t skip_types = AZ_IMPF_BADDIE | AZ_IMPF_SHIP;
    if (az_baddie_has_flag(baddie, AZ_BADF_WATER_BOUNCE)) {
      skip_types |= AZ_IMPF_NOT_LIQUID;
    }
    az_impact_t impact;
    az_circle_impact(state, baddie->data->main_body.bounding_radius,
                     baddie->position, az_vmul(baddie->velocity, time),
                     skip_types, baddie->uid, &impact);
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
      az_tick_bad_bouncer(state, baddie, time);
      break;
    case AZ_BAD_ATOM:
      az_tick_bad_atom(state, baddie, time);
      break;
    case AZ_BAD_SPINER:
      az_tick_bad_spiner(state, baddie, time);
      break;
    case AZ_BAD_BOX: break; // Do nothing.
    case AZ_BAD_ARMORED_BOX: break; // Do nothing.
    case AZ_BAD_CLAM:
      az_tick_bad_clam(state, baddie, time);
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
          for (int i = -1; i <= 1; i += 2) {
            az_fire_baddie_projectile(state, baddie, AZ_PROJ_NIGHTBLADE,
                                      20.0, 0.0, AZ_DEG2RAD(10 * i));
          }
          baddie->cooldown = 5.0;
          baddie->state = 0;
        }
      } else baddie->state = 0;
      break;
    case AZ_BAD_SPINE_MINE:
      az_tick_bad_spine_mine(state, baddie, time);
      break;
    case AZ_BAD_BROKEN_TURRET:
      az_tick_bad_broken_turret(state, baddie, time);
      break;
    case AZ_BAD_ZENITH_CORE:
      az_tick_bad_zenith_core(state, baddie, time);
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
      az_tick_bad_beam_sensor(state, baddie, time);
      break;
    case AZ_BAD_BEAM_SENSOR_INV:
      az_tick_bad_beam_sensor_inv(state, baddie, time);
      break;
    case AZ_BAD_GUN_SENSOR:
      az_tick_bad_gun_sensor(state, baddie, time);
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
      if (baddie->state == 0 &&
          !az_ship_in_range(state, baddie,
                            baddie->data->overall_bounding_radius +
                            AZ_SHIP_DEFLECTOR_RADIUS)) {
        baddie->components[0].angle =
          az_angle_towards(baddie->components[0].angle, AZ_DEG2RAD(600) * time,
                           (az_ship_in_range(state, baddie, 210) ?
                            AZ_DEG2RAD(90) : 0));
      }
      break;
    case AZ_BAD_CAVE_SWOOPER:
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
    case AZ_BAD_OTH_RAZOR_1:
    case AZ_BAD_OTH_RAZOR_2:
      az_tick_bad_oth_razor(state, baddie, time);
      break;
    case AZ_BAD_SECURITY_DRONE:
      az_tick_bad_security_drone(state, baddie, time);
      break;
    case AZ_BAD_SMALL_TRUCK:
      az_tick_bad_small_truck(state, baddie, time);
      break;
    case AZ_BAD_HEAT_RAY:
      az_tick_bad_heat_ray(state, baddie, time);
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
      az_tick_bad_copter_horz(state, baddie, time);
      break;
    case AZ_BAD_COPTER_VERT:
      az_tick_bad_copter_vert(state, baddie, time);
      break;
    case AZ_BAD_URCHIN:
      az_tick_bad_urchin(state, baddie, time);
      break;
    case AZ_BAD_BOSS_DOOR:
      az_tick_bad_boss_door(state, baddie, time);
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
      az_tick_bad_death_ray(state, baddie, time);
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
      az_tick_bad_bouncer_90(state, baddie, bounced);
      break;
    case AZ_BAD_PISTON:
    case AZ_BAD_ARMORED_PISTON:
    case AZ_BAD_ARMORED_PISTON_EXT:
    case AZ_BAD_INCORPOREAL_PISTON:
    case AZ_BAD_INCORPOREAL_PISTON_EXT: {
      if (baddie->state != (int)baddie->param) {
        az_play_sound(&state->soundboard, AZ_SND_PISTON_MOVEMENT);
        baddie->param = baddie->state;
      }
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
      az_tick_bad_super_spiner(state, baddie, time);
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
      az_tick_bad_fast_bouncer(state, baddie, time);
      break;
    case AZ_BAD_PROXY_MINE:
      if (az_vnorm(baddie->velocity) < 1.0) {
        baddie->velocity = AZ_VZERO;
      } else {
        const double tracking_base = 0.03; // smaller = faster tracking
        az_vpluseq(&baddie->velocity, az_vmul(baddie->velocity,
                                              pow(tracking_base, time) - 1.0));
      }
      baddie->angle = az_mod2pi(baddie->angle - AZ_DEG2RAD(120) * time);
      // State 0: Wait for ship.
      if (baddie->state == 0) {
        if (az_ship_in_range(state, baddie, 80) &&
            az_can_see_ship(state, baddie)) {
          baddie->state = 1;
          baddie->cooldown = 0.5;
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
      az_tick_bad_small_fish(state, baddie, time);
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
    case AZ_BAD_FORCE_EGG:
      az_tick_bad_force_egg(state, baddie, bounced, time);
      break;
    case AZ_BAD_FORCELING:
      az_tick_bad_forceling(state, baddie, time);
      break;
    case AZ_BAD_JUNGLE_CHOMPER:
      az_tick_bad_jungle_chomper(state, baddie, time);
      break;
    case AZ_BAD_SMALL_AUV:
      az_tick_bad_small_auv(state, baddie, time);
      break;
    case AZ_BAD_SENSOR_LASER:
      az_tick_bad_sensor_laser(state, baddie, time);
      break;
    case AZ_BAD_ERUPTION:
      if (baddie->state == 0) {
        baddie->cooldown = az_random(1, 2);
        baddie->state = 1;
      } else if (baddie->cooldown <= 0.0) {
        const az_projectile_t *proj =
          az_fire_baddie_projectile(state, baddie, AZ_PROJ_ERUPTION, 0, 0, 0);
        if (proj != NULL) {
          if (az_ray_intersects_camera_rectangle(
                  &state->camera, baddie->position,
                  az_vmul(proj->velocity, proj->data->lifetime))) {
            az_play_sound(&state->soundboard, AZ_SND_ERUPTION);
          } else {
            az_play_sound_with_volume(&state->soundboard, AZ_SND_ERUPTION,
                                      0.27);
          }
        }
        baddie->state = 0;
      }
      break;
    case AZ_BAD_PYROFLAKKER:
      az_tick_bad_pyroflakker(state, baddie, time);
      break;
    case AZ_BAD_PYROSTALKER:
      az_tick_bad_pyrostalker(state, baddie, time);
      break;
    case AZ_BAD_DEMON_SWOOPER:
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
              az_ship_in_range(state, baddie, 300) &&
              az_ship_within_angle(state, baddie, 0, AZ_DEG2RAD(10))) {
            for (int i = -1; i <= 1; ++i) {
              az_fire_baddie_projectile(state, baddie, AZ_PROJ_FIREBALL_FAST,
                                        8, 0, i * AZ_DEG2RAD(10));
            }
            az_play_sound(&state->soundboard, AZ_SND_FIRE_FIREBALL);
            baddie->cooldown = 2.0;
          }
          az_fly_towards_ship(state, baddie, time, AZ_DEG2RAD(150),
                              250.0, 300.0, 250.0, 100.0, 100.0);
          baddie->param = fmax(0.0, baddie->param - time);
        } else baddie->param = 0.0;
        if (baddie->param <= 0.0) baddie->state = 0;
      } else baddie->state = 0;
      az_trail_tail_behind(baddie, 0, AZ_PI, old_baddie_position,
                           old_baddie_angle);
      break;
    case AZ_BAD_FIRE_CHOMPER:
      az_tick_bad_fire_chomper(state, baddie, time);
      break;
    case AZ_BAD_GRABBER_PLANT:
      az_tick_bad_grabber_plant(state, baddie, time);
      break;
    case AZ_BAD_POP_OPEN_TURRET:
      az_tick_bad_pop_open_turret(state, baddie, time);
      break;
    case AZ_BAD_GNAT:
      az_tick_bad_gnat(state, baddie, time);
      break;
    case AZ_BAD_CREEPY_EYE:
      az_tick_bad_creepy_eye(state, baddie, time);
      break;
    case AZ_BAD_BOMB_SENSOR:
      az_tick_bad_bomb_sensor(state, baddie, time);
      break;
    case AZ_BAD_ROCKET_SENSOR:
      az_tick_bad_rocket_sensor(state, baddie, time);
      break;
    case AZ_BAD_SPIKED_VINE:
      az_tick_bad_spiked_vine(state, baddie, time);
      break;
    case AZ_BAD_MAGBEEST_HEAD:
      az_tick_bad_magbeest_head(state, baddie, time);
      break;
    case AZ_BAD_MAGBEEST_LEGS_L:
      az_tick_bad_magbeest_legs_l(state, baddie, time);
      break;
    case AZ_BAD_MAGBEEST_LEGS_R:
      az_tick_bad_magbeest_legs_r(state, baddie, time);
      break;
    case AZ_BAD_MAGMA_BOMB:
      az_tick_bad_magma_bomb(state, baddie, time);
      break;
    case AZ_BAD_OTH_BRAWLER:
      az_tick_bad_oth_brawler(state, baddie, time);
      break;
    case AZ_BAD_LARGE_FISH:
      az_tick_bad_large_fish(state, baddie, time);
      break;
    case AZ_BAD_CRAB_CRAWLER:
      az_tick_bad_crab_crawler(state, baddie, time);
      break;
    case AZ_BAD_SCRAP_METAL: break; // Do nothing.
    case AZ_BAD_RED_ATOM:
      az_tick_bad_red_atom(state, baddie, time);
      break;
    case AZ_BAD_REFLECTION:
      az_tick_bad_reflection(state, baddie, time);
      break;
    case AZ_BAD_OTH_MINICRAB:
      az_tick_bad_oth_minicrab(state, baddie, time);
      break;
    case AZ_BAD_OTH_SUPERGUNSHIP:
      az_tick_bad_oth_supergunship(state, baddie, time);
      break;
    case AZ_BAD_OTH_DECOY:
      az_tick_bad_oth_decoy(state, baddie, time);
      break;
    case AZ_BAD_CENTRAL_NETWORK_NODE: break; // Do nothing.
  }

  // Move cargo with the baddie (unless the baddie killed itself).
  const az_vector_t position_delta =
    az_vsub(baddie->position, old_baddie_position);
  if (baddie->kind != AZ_BAD_NOTHING &&
      az_baddie_has_flag(baddie, AZ_BADF_CARRIES_CARGO)) {
    az_move_baddie_cargo(
        state, baddie, position_delta,
        az_mod2pi(baddie->angle - old_baddie_angle));
  }

  // If we're entering or exiting a body of water, make a splash.
  AZ_ARRAY_LOOP(gravfield, state->gravfields) {
    if (!az_is_liquid(gravfield->kind)) continue;
    az_vector_t position, normal;
    if (az_ray_hits_liquid_surface(gravfield, old_baddie_position,
                                   position_delta, &position, &normal)) {
      az_add_sploosh(state, gravfield, position, normal,
                     az_vdiv(position_delta, time),
                     baddie->data->main_body.bounding_radius);
      az_play_sound(&state->soundboard, AZ_SND_SPLASH);
    }
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

void az_on_baddie_damaged(az_space_state_t *state, az_baddie_t *baddie,
                          double amount, az_damage_flags_t damage_kind) {
  switch (baddie->kind) {
    case AZ_BAD_OTH_GUNSHIP:
      az_on_oth_gunship_damaged(state, baddie, amount, damage_kind);
      break;
    case AZ_BAD_KILOFUGE:
      az_on_kilofuge_damaged(state, baddie, amount, damage_kind);
      break;
    case AZ_BAD_GRABBER_PLANT:
      az_on_grabber_plant_damaged(state, baddie, amount, damage_kind);
      break;
    case AZ_BAD_CREEPY_EYE:
      az_on_creepy_eye_damaged(state, baddie, amount, damage_kind);
      break;
    case AZ_BAD_CRAB_CRAWLER:
      az_on_crab_crawler_damaged(state, baddie, amount, damage_kind);
      break;
    case AZ_BAD_OTH_SUPERGUNSHIP:
      az_on_oth_supergunship_damaged(state, baddie, amount, damage_kind);
      break;
    default: break;
  }
}

void az_on_baddie_killed(az_space_state_t *state, az_baddie_kind_t kind,
                         az_vector_t position, double angle) {
  switch (kind) {
    case AZ_BAD_SUPER_SPINER:
      az_on_super_spiner_killed(state, position, angle);
      break;
    case AZ_BAD_JUNGLE_CRAWLER:
      az_on_jungle_crawler_killed(state, position, angle);
      break;
    default: break;
  }
}

/*===========================================================================*/
