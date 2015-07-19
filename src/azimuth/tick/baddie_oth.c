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

#include "azimuth/tick/baddie_oth.h"

#include <assert.h>
#include <math.h>

#include "azimuth/state/baddie.h"
#include "azimuth/state/baddie_oth.h"
#include "azimuth/state/object.h"
#include "azimuth/state/sound.h"
#include "azimuth/state/space.h"
#include "azimuth/tick/baddie_util.h"
#include "azimuth/util/random.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

static void tick_tendrils(az_space_state_t *state, az_baddie_t *baddie,
                          const az_oth_tendrils_data_t *tendrils,
                          double old_angle, double time) {
  assert(tendrils->num_tendrils <= AZ_MAX_BADDIE_COMPONENTS);
  const double dtheta = baddie->angle - old_angle;
  for (int i = 0; i < tendrils->num_tendrils; ++i) {
    az_component_t *tip =
      &baddie->components[AZ_MAX_BADDIE_COMPONENTS - 1 - i];
    const az_vector_t dpos = az_vmul(baddie->velocity, time);
    tip->position = az_vadd(az_vrotate(tip->position, -dtheta),
                            az_vrotate(dpos, baddie->angle));
    tip->angle -= dtheta;
    const az_vector_t base = tendrils->tendril_bases[i];
    const az_vector_t goal_pos =
      az_vadd(az_vwithlen(base, az_vnorm(base) + tendrils->length),
              az_vpolar(tendrils->drift, AZ_DEG2RAD(120) * i +
                        AZ_DEG2RAD(120) * state->ship.player.total_time));
    const double goal_angle = az_vtheta(goal_pos);
    const double tracking_factor = 1.0 - pow(tendrils->tracking_base, time);
    az_vpluseq(&tip->position, az_vmul(az_vsub(goal_pos, tip->position),
                                       tracking_factor));
    tip->angle = az_mod2pi(tip->angle +
        az_mod2pi(goal_angle - tip->angle) * tracking_factor);
  }
}

/*===========================================================================*/

void az_tick_bad_oth_brawler(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_OTH_BRAWLER);
  const double old_angle = baddie->angle;
  const double turn_rate = AZ_DEG2RAD(180);
  const az_vector_t ship_delta =
    az_vsub(state->ship.position, baddie->position);
  const bool aimed =
    fabs(az_mod2pi(az_vtheta(ship_delta) - baddie->angle)) <= AZ_DEG2RAD(10);
  // Figure out if we have a clear shot at the ship (ignoring walls that can be
  // destroyed by rockets, because we can just punch through those).
  bool clear_shot = true;
  az_impact_t impact;
  az_ray_impact(state, baddie->position, ship_delta,
                (AZ_IMPF_BADDIE | AZ_IMPF_WALL), baddie->uid, &impact);
  if (impact.type != AZ_IMP_SHIP) clear_shot = false;
  else {
    AZ_ARRAY_LOOP(wall, state->walls) {
      if (wall->kind == AZ_WALL_NOTHING ||
          wall->kind == AZ_WALL_DESTRUCTIBLE_CHARGED ||
          wall->kind == AZ_WALL_DESTRUCTIBLE_ROCKET) continue;
      if (az_ray_hits_wall(wall, baddie->position, ship_delta, NULL, NULL)) {
        clear_shot = false;
        break;
      }
    }
  }
  // Fire weapons:
  bool strafe = false;
  if (baddie->state <= 0) {
    if (clear_shot) strafe = true;
    if (baddie->cooldown <= 0.0 && clear_shot && aimed) {
      baddie->state = 5;
    }
  }
  if (baddie->state > 0) {
    strafe = true;
    if (baddie->cooldown <= 0.0) {
      if (baddie->state == 1) {
        for (int i = -1; i <= 1; ++i) {
          az_fire_baddie_projectile(state, baddie, AZ_PROJ_OTH_SPRAY,
                                    35.0, AZ_DEG2RAD(39) * i,
                                    AZ_DEG2RAD(-20) * i);
        }
        az_play_sound(&state->soundboard, AZ_SND_FIRE_OTH_SPRAY);
      } else {
        const double angle = (baddie->state % 2 ? -1 : 1) * AZ_DEG2RAD(35);
        az_fire_baddie_projectile(state, baddie, AZ_PROJ_OTH_ROCKET,
                                  20.0, angle, -angle);
        az_play_sound(&state->soundboard, AZ_SND_FIRE_OTH_ROCKET);
      }
      --baddie->state;
      if (baddie->state > 0) baddie->cooldown = 0.3;
      else baddie->cooldown = 2.0;
    }
  }
  // Navigate:
  if (strafe) {
    const az_vector_t goal1 =
      az_vadd(az_vpolar(200.0, state->ship.angle - AZ_DEG2RAD(110)),
              state->ship.position);
    const az_vector_t goal2 =
      az_vadd(az_vpolar(200.0, state->ship.angle + AZ_DEG2RAD(110)),
              state->ship.position);
    const az_vector_t goal =
      (az_vdist(goal1, baddie->position) < az_vdist(goal2, baddie->position) ?
       goal1 : goal2);
    az_drift_towards_position(state, baddie, time, goal, 300.0, 500.0, 100.0);
    baddie->angle = az_angle_towards(baddie->angle, turn_rate * time,
                                     az_vtheta(az_vsub(state->ship.position,
                                                       baddie->position)));
  } else {
    az_fly_towards_ship(state, baddie, time,
                        turn_rate, 300.0, 100.0, 20.0, 100.0, 100.0);
  }
  tick_tendrils(state, baddie, &AZ_OTH_BRAWLER_TENDRILS, old_angle, time);
}

void az_tick_bad_oth_crab_1(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_OTH_CRAB_1);
  const double old_angle = baddie->angle;
  az_fly_towards_ship(state, baddie, time,
                      2.0, 40.0, 100.0, 20.0, 100.0, 100.0);
  if (baddie->cooldown <= 0.0 &&
      az_ship_within_angle(state, baddie, 0, AZ_DEG2RAD(6)) &&
      az_can_see_ship(state, baddie)) {
    if (az_fire_baddie_projectile(state, baddie, AZ_PROJ_OTH_ROCKET,
                                  15.0, 0.0, 0.0) != NULL) {
      az_play_sound(&state->soundboard, AZ_SND_FIRE_OTH_ROCKET);
      baddie->cooldown = 2.0;
    }
  }
  tick_tendrils(state, baddie, &AZ_OTH_CRAB_TENDRILS, old_angle, time);
}

void az_tick_bad_oth_crab_2(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_OTH_CRAB_2);
  const double old_angle = baddie->angle;
  az_fly_towards_ship(state, baddie, time,
                      2.0, 100.0, 200.0, 50.0, 100.0, 100.0);
  baddie->param = fmax(0.0, baddie->param - time);
  if (baddie->cooldown <= 0.0 &&
      az_ship_within_angle(state, baddie, 0, AZ_DEG2RAD(6))) {
    if (az_fire_baddie_projectile(state, baddie, AZ_PROJ_OTH_ROCKET,
                                  15.0, 0.0, 0.0) != NULL) {
      az_play_sound(&state->soundboard, AZ_SND_FIRE_OTH_ROCKET);
      baddie->cooldown = 1.5;
    }
  }
  if (baddie->param <= 0.0 &&
      az_ship_within_angle(state, baddie, 0, AZ_DEG2RAD(8)) &&
      az_can_see_ship(state, baddie)) {
    for (int i = -1; i <= 1; i += 2) {
      az_fire_baddie_projectile(state, baddie, AZ_PROJ_OTH_SPRAY,
                                15.0, AZ_DEG2RAD(14 * i), 0.0);
    }
    az_play_sound(&state->soundboard, AZ_SND_FIRE_OTH_SPRAY);
    baddie->param = 0.9;
  }
  tick_tendrils(state, baddie, &AZ_OTH_CRAB_TENDRILS, old_angle, time);
}

void az_tick_bad_oth_crawler(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_OTH_CRAWLER);
  const double old_angle = baddie->angle;
  az_crawl_around(state, baddie, time, true, 3.0, 40.0, 100.0);
  if (baddie->cooldown <= 0.0 && az_can_see_ship(state, baddie)) {
    az_vector_t rel_impact;
    if (az_lead_target(az_vsub(state->ship.position, baddie->position),
                       state->ship.velocity, 1000.0, &rel_impact)) {
      az_fire_baddie_projectile(
          state, baddie, AZ_PROJ_OTH_MINIROCKET,
          0.0, az_vtheta(rel_impact) - baddie->angle, 0.0);
      az_play_sound(&state->soundboard, AZ_SND_FIRE_OTH_ROCKET);
      baddie->cooldown = az_random(1.0, 2.0);
    }
  }
  tick_tendrils(state, baddie, &AZ_OTH_CRAWLER_TENDRILS, old_angle, time);
}

void az_tick_bad_oth_orb_1(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_OTH_ORB_1);
  const double old_angle = baddie->angle;
  az_drift_towards_ship(state, baddie, time, 80, 20, 100);
  if (baddie->cooldown <= 0.0 && az_ship_in_range(state, baddie, 500)) {
    for (int i = 0; i < 360; i += 20) {
      az_fire_baddie_projectile(
          state, baddie, AZ_PROJ_OTH_SPRAY,
          baddie->data->main_body.bounding_radius, AZ_DEG2RAD(i), 0.0);
    }
    az_play_sound(&state->soundboard, AZ_SND_FIRE_OTH_SPRAY);
    baddie->cooldown = 2.0;
  }
  tick_tendrils(state, baddie, &AZ_OTH_ORB_TENDRILS, old_angle, time);
}

void az_tick_bad_oth_orb_2(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_OTH_ORB_2);
  const double old_angle = baddie->angle;
  az_drift_towards_ship(state, baddie, time, 80, 20, 100);
  if (baddie->cooldown <= 0.0 && az_ship_in_range(state, baddie, 500) &&
      az_can_see_ship(state, baddie)) {
    az_fire_baddie_projectile(state, baddie, AZ_PROJ_OTH_HOMING,
                              baddie->data->main_body.bounding_radius,
                              az_random(-AZ_PI, AZ_PI), 0.0);
    baddie->cooldown = 0.1;
  }
  tick_tendrils(state, baddie, &AZ_OTH_ORB_TENDRILS, old_angle, time);
}

void az_tick_bad_oth_razor(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_OTH_RAZOR);
  const double old_angle = baddie->angle;
  if (baddie->state == 0) {
    baddie->velocity = az_vpolar(az_random(300, 500), baddie->angle);
    baddie->state = 2;
  } else if (baddie->state == 1) {
    baddie->velocity = az_vpolar(300, baddie->angle);
    baddie->state = 3;
  }
  if (baddie->state == 2) {
    az_drift_towards_ship(state, baddie, time, 400, 500, 100);
  } else if (baddie->state == 3) {
    baddie->velocity = az_vwithlen(baddie->velocity, 300.0);
  } else baddie->state = 0;
  baddie->angle = az_angle_towards(baddie->angle, AZ_DEG2RAD(180) * time,
                                   az_vtheta(baddie->velocity));
  tick_tendrils(state, baddie, &AZ_OTH_RAZOR_TENDRILS, old_angle, time);
}

/*===========================================================================*/

void az_tick_bad_oth_snapdragon(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_OTH_SNAPDRAGON);
  const double old_angle = baddie->angle;
  az_fly_towards_ship(state, baddie, time,
                      5.0, 300.0, 300.0, 200.0, 0.0, 100.0);
  if (baddie->cooldown <= 0.0) {
    const bool crazy = (baddie->health <= 0.15 * baddie->data->max_health);
    if (crazy) baddie->state = 0;
    // The Snapdragon proceeds in a 9-state cycle; each state continues to the
    // next until it loops back around to zero.
    switch (baddie->state) {
      // States 0, 1, 2, 4, and 6: Fire an Oth Rocket.
      case 0: case 1: case 2: case 4: case 6:
        if (crazy || az_ship_within_angle(state, baddie, 0, AZ_DEG2RAD(6))) {
          az_projectile_t *proj = az_fire_baddie_projectile(
              state, baddie, AZ_PROJ_OTH_ROCKET, 30.0, 0.0, 0.0);
          if (proj != NULL) {
            proj->power = (crazy ? 0.3 : 0.5);
            az_play_sound(&state->soundboard, AZ_SND_FIRE_OTH_ROCKET);
            baddie->cooldown = (crazy ? 0.75 : 2.0);
            ++baddie->state;
          }
        }
        break;
      // States 3 and 7: Fire a spray of homing projectiles.
      case 3: case 7:
        for (int i = 0; i < 360; i += 15) {
          az_fire_baddie_projectile(
              state, baddie, AZ_PROJ_OTH_SPRAY,
              baddie->data->main_body.bounding_radius, AZ_DEG2RAD(i), 0.0);
        }
        az_play_sound(&state->soundboard, AZ_SND_FIRE_OTH_SPRAY);
        baddie->cooldown = 2.0;
        ++baddie->state;
        break;
      // State 5: Launch three Oth Razors that home in on the ship.
      case 5:
        for (int i = -1; i <= 1; ++i) {
          az_baddie_t *razor = az_add_baddie(
              state, AZ_BAD_OTH_RAZOR, baddie->position,
              baddie->angle + AZ_PI + i * AZ_DEG2RAD(45));
          if (razor == NULL) break;
        }
        az_play_sound(&state->soundboard, AZ_SND_LAUNCH_OTH_RAZORS);
        baddie->cooldown = az_random(2.0, 4.0);
        ++baddie->state;
        break;
      // State 8: Launch four Oth Razors that bounce aimlessly.
      case 8:
        for (int i = 0; i < 4; ++i) {
          az_baddie_t *razor = az_add_baddie(
              state, AZ_BAD_OTH_RAZOR, baddie->position,
              baddie->angle + AZ_DEG2RAD(45) + i * AZ_DEG2RAD(90));
          if (razor != NULL) razor->state = 1;
          else break;
        }
        az_play_sound(&state->soundboard, AZ_SND_LAUNCH_OTH_RAZORS);
        baddie->cooldown = az_random(2.0, 4.0);
        baddie->state = 0;
        break;
      default:
        baddie->state = 0;
        break;
    }
  }
  tick_tendrils(state, baddie, &AZ_OTH_SNAPDRAGON_TENDRILS, old_angle, time);
}

/*===========================================================================*/

#define TRACTOR_MIN_LENGTH 100.0
#define TRACTOR_MAX_LENGTH 200.0
#define TRACTOR_COMPONENT_INDEX 6

static bool tractor_locked_on(
    const az_baddie_t *baddie, az_vector_t *tractor_pos_out,
    double *tractor_length_out) {
  const az_component_t *component =
    &baddie->components[TRACTOR_COMPONENT_INDEX];
  const double tractor_length = component->angle;
  if (tractor_length == 0) return false;
  assert(tractor_length >= TRACTOR_MIN_LENGTH &&
         tractor_length <= TRACTOR_MAX_LENGTH);
  if (tractor_pos_out != NULL) *tractor_pos_out = component->position;
  if (tractor_length_out != NULL) *tractor_length_out = tractor_length;
  return true;
}

static void set_tractor_node(az_baddie_t *baddie, const az_node_t *node) {
  assert(baddie->kind == AZ_BAD_OTH_GUNSHIP);
  if (node == NULL) {
    baddie->components[TRACTOR_COMPONENT_INDEX].angle = 0;
    return;
  }
  assert(node->kind == AZ_NODE_TRACTOR);
  const double tractor_length = az_vdist(baddie->position, node->position);
  assert(tractor_length >= TRACTOR_MIN_LENGTH &&
         tractor_length <= TRACTOR_MAX_LENGTH);
  baddie->components[TRACTOR_COMPONENT_INDEX].position = node->position;
  baddie->components[TRACTOR_COMPONENT_INDEX].angle = tractor_length;
}

static void try_lock_on_tractor(az_space_state_t *state, az_baddie_t *baddie) {
  if (tractor_locked_on(baddie, NULL, NULL)) return;
  AZ_ARRAY_LOOP(node, state->nodes) {
    if (node->kind != AZ_NODE_TRACTOR) continue;
    const double dist = az_vdist(baddie->position, node->position);
    if (dist < TRACTOR_MIN_LENGTH || dist > TRACTOR_MAX_LENGTH) continue;
    set_tractor_node(baddie, node);
    break;
  }
}

void az_tick_bad_oth_gunship(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_OTH_GUNSHIP);
  const double old_angle = baddie->angle;
  const double hurt = 1.0 - baddie->health / baddie->data->max_health;

  // Handle tractor beam:
  az_vector_t tractor_position;
  double tractor_length;
  if (tractor_locked_on(baddie, &tractor_position, &tractor_length)) {
    baddie->position = az_vadd(az_vwithlen(az_vsub(baddie->position,
                                                   tractor_position),
                                           tractor_length),
                               tractor_position);
    baddie->velocity = az_vflatten(baddie->velocity,
        az_vsub(baddie->position, tractor_position));
    az_loop_sound(&state->soundboard, AZ_SND_TRACTOR_BEAM);
  }

  switch (baddie->state) {
    // State 0: Intro.
    case 0:
      baddie->cooldown = 10.0;
      baddie->state = 3; // TODO
      break;
    // State 1: Flee from ship.
    case 1: {
      // Out of all marker nodes the Oth Gunship can see, pick the one farthest
      // from the ship.
      double best_dist = 0.0;
      az_vector_t target = baddie->position;
      AZ_ARRAY_LOOP(node, state->nodes) {
        if (node->kind != AZ_NODE_MARKER) continue;
        if (az_baddie_has_clear_path_to_position(state, baddie,
                                                 node->position)) {
          const double dist = az_vdist(node->position, state->ship.position);
          if (dist > best_dist) {
            best_dist = dist;
            target = node->position;
          }
        }
      }
      // If we've reached the target position (or if the cooldown expires), go
      // to state 2.  Otherwise, fly towards the target position.
      if (baddie->cooldown <= 0.0 ||
          az_vwithin(baddie->position, target, 50.0)) {
        if (!az_ship_in_range(state, baddie, 800)) {
          baddie->cooldown = 3.0;
          baddie->state = 3;
        } else baddie->state = 2;
      } else {
        az_fly_towards_position(state, baddie, time, target,
                                5.0, 300, 300, 200, 100);
      }
    } break;
    // State 2: Pursue the ship.
    case 2: {
      az_fly_towards_ship(state, baddie, time, 5.0, 300, 300, 200, 200, 100);
      if (az_ship_in_range(state, baddie, 300)) {
        baddie->state = 5;
      }
    } break;
    // State 3: Line up for C-plus dash.
    case 3: {
      baddie->velocity = AZ_VZERO;
      az_vector_t rel_impact;
      if (az_lead_target(az_vsub(state->ship.position, baddie->position),
                         state->ship.velocity, 1000.0, &rel_impact)) {
        const double goal_theta = az_vtheta(rel_impact);
        baddie->angle =
          az_angle_towards(baddie->angle, AZ_DEG2RAD(360) * time, goal_theta);
        if (fabs(az_mod2pi(baddie->angle - goal_theta)) <= AZ_DEG2RAD(1) &&
            az_baddie_has_clear_path_to_position(
                state, baddie, az_vadd(baddie->position, rel_impact))) {
          baddie->velocity = az_vpolar(1000.0, baddie->angle);
          baddie->state = 4;
          break;
        }
      }
      if (baddie->cooldown <= 0.0) baddie->state = 2;
    } break;
    // State 4: Use C-plus drive.
    case 4:
      if (fabs(az_mod2pi(az_vtheta(baddie->velocity) -
                         baddie->angle)) > AZ_DEG2RAD(5)) {
        baddie->state = 1;
      } else {
        // TODO: do another az_lead_target, and steer towards new position
        az_particle_t *particle;
        if (az_insert_particle(state, &particle)) {
          particle->kind = AZ_PAR_EMBER;
          particle->color = (az_color_t){64, 255, 64, 255};
          particle->position =
            az_vadd(baddie->position, az_vpolar(-15.0, baddie->angle));
          particle->velocity = AZ_VZERO;
          particle->lifetime = 0.3;
          particle->param1 = 20;
        }
      }
      break;
    // State 5: Dogfight.
    case 5: {
      try_lock_on_tractor(state, baddie); // TODO
      az_fly_towards_ship(state, baddie, time, 5.0, 300, 300, 200, 200, 100);
      if (baddie->cooldown <= 0.0 &&
          az_ship_within_angle(state, baddie, 0, AZ_DEG2RAD(3)) &&
          az_can_see_ship(state, baddie)) {
        az_fire_baddie_projectile(state, baddie, AZ_PROJ_GUN_NORMAL,
                                  20.0, 0.0, 0.0);
        az_play_sound(&state->soundboard, AZ_SND_FIRE_GUN_NORMAL);
        baddie->cooldown = 0.1;
        if (az_random(0, 1) < 0.1) {
          set_tractor_node(baddie, NULL);
          baddie->state = 6 + az_randint(0, az_imin(3, (int)(4.0 * hurt)));
        }
      }
    } break;
    // State 6: Fire a charged triple shot.
    case 6:
      az_fly_towards_ship(state, baddie, time, 5.0, 300, 300, 200, 200, 100);
      if (baddie->cooldown <= 0.0 &&
          az_ship_within_angle(state, baddie, 0, AZ_DEG2RAD(8)) &&
          az_can_see_ship(state, baddie)) {
        for (int i = -1; i <= 1; ++i) {
          az_fire_baddie_projectile(state, baddie, AZ_PROJ_GUN_CHARGED_TRIPLE,
                                    20.0, 0.0, i * AZ_DEG2RAD(10));
        }
        az_play_sound(&state->soundboard, AZ_SND_FIRE_GUN_NORMAL);
        baddie->cooldown = 6.0;
        baddie->state = 1;
      }
      break;
    // State 7: Fire a hyper rocket.
    case 7:
      az_fly_towards_ship(state, baddie, time, 5.0, 300, 300, 200, 200, 100);
      if (baddie->cooldown <= 0.0 &&
          az_ship_within_angle(state, baddie, 0, AZ_DEG2RAD(3)) &&
          az_can_see_ship(state, baddie)) {
        az_fire_baddie_projectile(state, baddie, AZ_PROJ_HYPER_ROCKET,
                                  20.0, 0.0, 0.0);
        az_play_sound(&state->soundboard, AZ_SND_FIRE_HYPER_ROCKET);
        baddie->cooldown = 6.0;
        baddie->state = 1;
      }
      break;
    // State 8: Fire a charged homing shot.
    case 8:
      az_fly_towards_ship(state, baddie, time, 5.0, 300, 300, 200, 200, 100);
      if (baddie->cooldown <= 0.0 && az_can_see_ship(state, baddie)) {
        for (int i = 0; i < 4; ++i) {
          az_fire_baddie_projectile(
              state, baddie, AZ_PROJ_GUN_CHARGED_HOMING,
              20.0, 0.0, AZ_DEG2RAD(45) + i * AZ_DEG2RAD(90));
        }
        az_play_sound(&state->soundboard, AZ_SND_FIRE_GUN_NORMAL);
        baddie->cooldown = 6.0;
        baddie->state = 1;
      }
      break;
    // State 9: Fire a missile barrage.
    case 9:
      az_fly_towards_ship(state, baddie, time, 5.0, 300, 300, 200, 200, 100);
      if (baddie->cooldown <= 0.0 &&
          az_ship_within_angle(state, baddie, 0, AZ_DEG2RAD(8)) &&
          az_can_see_ship(state, baddie)) {
        az_fire_baddie_projectile(state, baddie, AZ_PROJ_MISSILE_BARRAGE,
                                  20.0, 0.0, 0.0);
        baddie->cooldown = 6.0;
        baddie->state = 1;
      }
      break;
    default:
      baddie->state = 0;
      break;
  }
  tick_tendrils(state, baddie, &AZ_OTH_GUNSHIP_TENDRILS, old_angle, time);
}

/*===========================================================================*/
