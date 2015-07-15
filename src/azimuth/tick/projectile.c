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

#include "azimuth/tick/projectile.h"

#include <assert.h>
#include <math.h>

#include "azimuth/state/projectile.h"
#include "azimuth/state/space.h"
#include "azimuth/state/uid.h"
#include "azimuth/state/wall.h"
#include "azimuth/tick/baddie.h" // for az_on_baddie_damaged
#include "azimuth/tick/door.h" // for az_try_open_door
#include "azimuth/tick/object.h" // for az_try_damage_baddie
#include "azimuth/tick/script.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/random.h"

/*===========================================================================*/

static bool times_per_second(double per_second, const az_projectile_t *proj,
                             double time) {
  return (ceil(per_second * proj->age) >
          ceil(per_second * (proj->age - time)));
}

// Remove the projectile and create specks in its place.
void az_expire_projectile(az_space_state_t *state, az_projectile_t *proj) {
  assert(proj->kind != AZ_PROJ_NOTHING);
  if (!(proj->data->properties & AZ_PROJF_FEW_SPECKS)) {
    const double base_speed = 0.5 * az_vnorm(proj->velocity);
    for (int i = 0; i < 3; ++i) {
      az_add_speck(state, AZ_WHITE, 1.0, proj->position,
                   az_vpolar(base_speed + az_random(20, 70), proj->angle +
                             AZ_DEG2RAD(15) * az_random(-1, 1)));
    }
  }
  proj->kind = AZ_PROJ_NOTHING;
}

// Common projectile impact code, called by both on_projectile_hit_wall and
// on_projectile_hit_target.
static void on_projectile_impact(az_space_state_t *state,
                                 az_projectile_t *proj, az_vector_t normal) {
  assert(proj->kind != AZ_PROJ_NOTHING);
  const bool attuned =
    (proj->fired_by == AZ_SHIP_UID &&
     az_has_upgrade(&state->ship.player, AZ_UPG_ATTUNED_EXPLOSIVES));
  const double radius = proj->data->splash_radius *
    (attuned ? AZ_ATTUNED_EXPLOSIVES_RADIUS_FACTOR : 1.0);
  // If applicable, deal splash damage.
  if (radius > 0.0 && proj->data->splash_damage > 0.0) {
    // Damage the ship if it's within the blast (even if this projectile was
    // fired by the ship).
    if (az_ship_is_alive(&state->ship) &&
        az_vwithin(state->ship.position, proj->position, radius)) {
      double damage = proj->data->splash_damage * proj->power;
      // The Attuned Explosives upgrade reduces the splash damage the ship
      // takes from its own explosives.
      if (attuned) damage *= AZ_ATTUNED_EXPLOSIVES_DAMAGE_FACTOR;
      az_damage_ship(state, damage, false);
    }
    // Damage baddies that are within the blast (except that a baddie is not
    // damaged by its own projectiles).
    AZ_ARRAY_LOOP(baddie, state->baddies) {
      if (baddie->kind == AZ_BAD_NOTHING) continue;
      if (baddie->uid == proj->fired_by) continue;
      if (az_baddie_has_flag(baddie, AZ_BADF_INCORPOREAL)) continue;
      const az_component_data_t *component;
      if (az_circle_touches_baddie(baddie, radius, proj->position,
                                   &component)) {
        az_try_damage_baddie(state, baddie, component,
                             proj->data->damage_kind,
                             proj->data->splash_damage * proj->power);
        if (proj->kind == AZ_PROJ_MISSILE_FREEZE &&
            !(baddie->data->main_body.immunities & AZ_DMGF_FREEZE)) {
          baddie->frozen = 1.0;
          az_on_baddie_damaged(state, baddie, 0.0, AZ_DMGF_FREEZE);
        }
      }
    }
    // Open doors that are within the blast.
    if (proj->fired_by == AZ_SHIP_UID) {
      AZ_ARRAY_LOOP(door, state->doors) {
        if (door->kind == AZ_DOOR_NOTHING) continue;
        if (az_circle_touches_door_outside(door, radius, proj->position)) {
          az_try_open_door(state, door, proj->data->damage_kind);
        }
      }
    }
    // Destroy destructible walls within the blast.
    if ((proj->data->damage_kind & AZ_DMGF_WALL_FLARE) != 0) {
      AZ_ARRAY_LOOP(wall, state->walls) {
        if (wall->kind == AZ_WALL_NOTHING) continue;
        if (wall->kind == AZ_WALL_INDESTRUCTIBLE) continue;
        if (az_circle_touches_wall(wall, radius, proj->position)) {
          az_try_break_wall(state, wall, proj->data->damage_kind,
                            proj->position);
        }
      }
    }
  }

  // If applicable, burst into shrapnel.
  if (proj->data->shrapnel_kind != AZ_PROJ_NOTHING) {
    const double mid_theta = az_vtheta(normal);
    const int limit = (proj->kind == AZ_PROJ_GUN_PHASE_BURST ? 22 : 2);
    for (int i = -limit; i <= limit; ++i) {
      const double theta = mid_theta +
        (proj->kind == AZ_PROJ_GUN_PHASE_BURST ? AZ_DEG2RAD(i) :
         0.2 * AZ_PI * (i + az_random(-.5, .5)));
      az_projectile_t *shrapnel = az_add_projectile(
          state, proj->data->shrapnel_kind,
          az_vadd(proj->position, az_vpolar(0.1, theta)), theta, proj->power,
          proj->fired_by);
      if (shrapnel != NULL &&
          !(proj->data->properties & AZ_PROJF_FAST_SHRAPNEL)) {
        shrapnel->velocity = az_vmul(shrapnel->velocity, az_random(0.5, 1.0));
      }
    }
  }

  // Add particles.
  const bool splash = radius > 0.0;
  const bool few_specks = (proj->data->properties & AZ_PROJF_FEW_SPECKS);
  az_color_t speck_color = AZ_WHITE;
  if (splash || !few_specks) {
    az_particle_t *particle;
    if (az_insert_particle(state, &particle)) {
      if (proj->data->damage_kind & AZ_DMGF_FREEZE) {
        speck_color = (az_color_t){192, 224, 255, 255};
        if (splash) {
          particle->kind = AZ_PAR_ICE_BOOM;
          particle->color = (az_color_t){128, 224, 255, 255};
          particle->lifetime = 0.6;
        } else {
          particle->kind = AZ_PAR_BOOM;
          particle->color = speck_color;
          particle->lifetime = 0.3;
        }
      } else if (proj->data->damage_kind & AZ_DMGF_FLAME) {
        speck_color = (az_color_t){255, 128, 0, 192};
        particle->kind = AZ_PAR_FIRE_BOOM;
        particle->color = speck_color;
        particle->lifetime = fmax(0.3, 0.003 * proj->data->splash_radius);
      } else if (splash) {
        particle->kind = AZ_PAR_EXPLOSION;
        particle->color = (az_color_t){255, 240, 224, 192};
        particle->lifetime = 0.15 * cbrt(radius);
      } else {
        particle->kind = AZ_PAR_BOOM;
        particle->color = AZ_WHITE;
        particle->lifetime = 0.3;
      }
      particle->position = proj->position;
      particle->velocity = AZ_VZERO;
      particle->angle = proj->angle;
      particle->param1 = (splash ? radius : 10.0);
    }
  }
  if (few_specks) {
    az_add_speck(state, speck_color, 1.0, proj->position,
                 az_vpolar(az_random(20, 70), az_random(0, AZ_TWO_PI)));
  } else {
    for (int i = 0; i < 5; ++i) {
      az_add_speck(state, speck_color, 1.0, proj->position,
                   az_vpolar(az_random(20, 70), az_random(0, AZ_TWO_PI)));
    }
  }

  // Play sound.
  az_play_sound(&state->soundboard, proj->data->impact_sound);

  // Gravity torpedos are special: on impact, they create a temporary gravity
  // well that pulls the ship in.
  if (proj->kind == AZ_PROJ_GRAVITY_TORPEDO) {
    az_add_projectile(state, AZ_PROJ_GRAVITY_TORPEDO_WELL, proj->position,
                      proj->angle, proj->power, proj->fired_by);
  }
  // Ice torpedos are special: on impact, they create a number of ice crystals.
  else if (proj->kind == AZ_PROJ_ICE_TORPEDO) {
    const az_camera_bounds_t *bounds = az_current_camera_bounds(state);
    const double crystal_radius =
      az_get_baddie_data(AZ_BAD_ICE_CRYSTAL)->overall_bounding_radius;
    for (int i = 0; i < 20; ++i) {
      const az_vector_t position = az_vadd(proj->position, az_vpolar(
          az_random(0, proj->data->splash_radius), az_random(-AZ_PI, AZ_PI)));
      if (!az_position_visible(bounds, position)) continue;
      az_impact_t impact;
      az_circle_impact(state, crystal_radius, position, AZ_VZERO,
                       0, AZ_NULL_UID, &impact);
      if (impact.type != AZ_IMP_NOTHING) continue;
      const double angle = az_random(-AZ_PI, AZ_PI);
      az_add_baddie(state, AZ_BAD_ICE_CRYSTAL, position, angle);
    }
  }
  // Nightseeds are special: on impact, they spawn a Nightbug.
  else if (proj->kind == AZ_PROJ_NIGHTSEED) {
    const az_baddie_kind_t baddie_kind = AZ_BAD_NIGHTBUG;
    const double baddie_radius =
      az_get_baddie_data(baddie_kind)->overall_bounding_radius;
    const az_vector_t baddie_position =
      az_vadd(proj->position, az_vwithlen(normal, baddie_radius + 5.0));
    az_impact_t impact;
    az_circle_impact(state, baddie_radius, baddie_position, AZ_VZERO,
                     AZ_IMPF_BADDIE, AZ_NULL_UID, &impact);
    if (impact.type == AZ_IMP_NOTHING) {
      az_baddie_t *baddie =
        az_add_baddie(state, baddie_kind, baddie_position, proj->angle);
      if (baddie != NULL) baddie->param = 1.0;
    }
  }
}

// Called when a projectile hits a wall or a door.  This should never be called
// for phased projectiles.
static void on_projectile_hit_wall(az_space_state_t *state,
                                   az_projectile_t *proj, az_vector_t normal) {
  assert(proj->kind != AZ_PROJ_NOTHING);
  assert(!(proj->data->properties & AZ_PROJF_PHASED));
  // Bouncing fireballs are special: they bounce off walls the first few hits.
  if (proj->kind == AZ_PROJ_BOUNCING_FIREBALL && proj->param < 3) {
    ++proj->param;
    az_vpluseq(&proj->position, az_vwithlen(normal, 0.5));
    az_vpluseq(&proj->velocity, az_vmul(az_vproj(proj->velocity, normal), -2));
    proj->angle = az_vtheta(proj->velocity);
    az_play_sound(&state->soundboard, AZ_SND_BOUNCE_FIREBALL);
    return;
  }
  // Explode the projectile.
  on_projectile_impact(state, proj, (proj->kind == AZ_PROJ_GUN_PHASE_BURST ?
                                     az_vpolar(1, proj->angle) : normal));
  // Shake the screen.
  const double shake = proj->data->impact_shake;
  if (shake > 0.0) {
    az_shake_camera(&state->camera, shake, shake * 0.75);
  }
  // Remove the projectile.
  proj->kind = AZ_PROJ_NOTHING;
}

// Common projectile impact code, called by both on_projectile_hit_baddie and
// on_projectile_hit_ship.
static void on_projectile_hit_target(
    az_space_state_t *state, az_projectile_t *proj, az_vector_t normal) {
  assert(proj->kind != AZ_PROJ_NOTHING);
  on_projectile_impact(state, proj, normal);
  // Shake the screen (less than we do if the projectile hit a wall).
  const double shake = proj->data->impact_shake;
  if (shake > 0.0) {
    az_shake_camera(&state->camera, shake * 0.75, shake * 0.25);
  }
  // Pierce Missiles are special: when they hit a target, they ricochet towards
  // another target.
  if (proj->kind == AZ_PROJ_MISSILE_PIERCE) {
    if (proj->param < 4) {
      const double old_proj_angle = proj->angle;
      double best_dist = INFINITY;
      AZ_ARRAY_LOOP(baddie, state->baddies) {
        if (baddie->kind == AZ_BAD_NOTHING) continue;
        if (baddie->uid == proj->last_hit_uid) continue;
        if (az_baddie_has_flag(baddie, AZ_BADF_NO_HOMING_PROJ)) continue;
        const double dist = az_vdist(baddie->position, proj->position) +
          fabs(az_mod2pi(az_vtheta(az_vsub(baddie->position, proj->position)) -
                         old_proj_angle)) * 100.0;
        if (dist < best_dist) {
          best_dist = dist;
          proj->angle = az_vtheta(az_vsub(baddie->position, proj->position));
        }
      }
      proj->velocity = az_vpolar(az_vnorm(proj->velocity) - 50.0, proj->angle);
      ++proj->param;
    } else proj->kind = AZ_PROJ_NOTHING;
    return;
  }
  // Remove the projectile (unless it's piercing, in which case it should
  // continue on beyond this target).
  if (!(proj->data->properties & AZ_PROJF_PIERCING)) {
    proj->kind = AZ_PROJ_NOTHING;
  }
}

// Called when a projectile hits a baddie.
static void on_projectile_hit_baddie(
    az_space_state_t *state, az_projectile_t *proj, az_baddie_t *baddie,
    const az_component_data_t *component, az_vector_t normal) {
  assert(proj->kind != AZ_PROJ_NOTHING);
  assert(proj->fired_by == AZ_SHIP_UID || proj->fired_by == AZ_NULL_UID);
  proj->last_hit_uid = baddie->uid;
  const double prev_flare = baddie->armor_flare;
  if (!az_try_damage_baddie(state, baddie, component, proj->data->damage_kind,
                            proj->data->impact_damage * proj->power)) {
    az_play_sound(&state->soundboard, baddie->data->armor_sound);
  } else if (baddie->kind != AZ_BAD_NOTHING && prev_flare < 0.75) {
    az_play_sound(&state->soundboard, baddie->data->hurt_sound);
  }
  // Note that at this point, the baddie may now be dead and removed.  So we
  // can no longer use the `baddie` or `component` pointers.
  on_projectile_hit_target(state, proj, normal);
}

// Called when a projectile hits the ship.
static void on_projectile_hit_ship(
    az_space_state_t *state, az_projectile_t *proj, az_vector_t normal) {
  assert(proj->kind != AZ_PROJ_NOTHING);
  assert(proj->fired_by != AZ_SHIP_UID);
  az_ship_t *ship = &state->ship;
  assert(az_ship_is_alive(ship));
  proj->last_hit_uid = AZ_SHIP_UID;
  // Knock the ship around:
  az_vpluseq(&ship->velocity, az_vwithlen(
      az_vsub(ship->position, proj->position), 10.0 * proj->power *
      (proj->data->impact_damage + proj->data->splash_damage) +
      8.0 * proj->data->impact_shake));
  // Damage the ship and explode the projectile:
  az_damage_ship(state, proj->data->impact_damage * proj->power,
                 (bool)(proj->data->properties & AZ_PROJF_TEMP_INVINC));
  on_projectile_hit_target(state, proj, normal);
}

static void projectile_home_in(az_space_state_t *state,
                               az_projectile_t *proj,
                               double time) {
  assert(proj->data->homing_rate > 0.0);
  // Homing/phase projectiles' homing can be turned off by its special logic.
  if (proj->kind == AZ_PROJ_GUN_HOMING_PHASE && proj->param != 0) return;
  // First, figure out what position we're homing in on.
  bool found_target = false;
  az_vector_t goal = AZ_VZERO;
  if (proj->fired_by != AZ_SHIP_UID) {
    if (az_ship_is_decloaked(&state->ship)) {
      found_target = true;
      goal = state->ship.position;
    }
  } else {
    double best_dist = INFINITY;
    AZ_ARRAY_LOOP(baddie, state->baddies) {
      if (baddie->kind == AZ_BAD_NOTHING) continue;
      if (baddie->uid == proj->last_hit_uid) continue;
      if (az_baddie_has_flag(baddie, AZ_BADF_NO_HOMING_PROJ)) continue;
      const double dist = az_vdist(baddie->position, proj->position) +
        fabs(az_mod2pi(az_vtheta(az_vsub(baddie->position, proj->position)) -
                       proj->angle)) * 100.0;
      if (dist < best_dist) {
        best_dist = dist;
        found_target = true;
        goal = baddie->position;
      }
    }
  }
  if (!found_target) return;
  // Now, home in on the goal position.
  proj->angle = az_angle_towards(proj->angle, time * proj->data->homing_rate,
                                 az_vtheta(az_vsub(goal, proj->position)));
  proj->velocity = az_vpolar(az_vnorm(proj->velocity), proj->angle);
}

static void leave_particle_trail(
    az_space_state_t *state, az_projectile_t *proj, az_particle_kind_t kind,
    az_color_t color, double lifetime, double param1, double param2) {
  assert(proj->kind != AZ_PROJ_NOTHING);
  az_particle_t *particle;
  if (az_insert_particle(state, &particle)) {
    particle->kind = kind;
    particle->color = color;
    particle->position = proj->position;
    particle->velocity = AZ_VZERO;
    particle->angle = proj->angle;
    particle->lifetime = lifetime;
    particle->param1 = param1;
    particle->param2 = param2;
  }
}

static void leave_missile_trail(az_space_state_t *state,
                                az_projectile_t *proj,
                                double time, az_color_t color) {
  assert(proj->kind != AZ_PROJ_NOTHING);
  if (times_per_second(20, proj, time)) {
    az_particle_t *particle;
    if (az_insert_particle(state, &particle)) {
      particle->kind = AZ_PAR_BOOM;
      particle->color = color;
      particle->position = proj->position;
      particle->velocity = AZ_VZERO;
      particle->lifetime = 0.5;
      particle->param1 = 10.0;
    }
  }
}

// Called after aging the projectile, but before doing anything else (including
// removing it if it is past its lifetime).
static void projectile_special_logic(az_space_state_t *state,
                                     az_projectile_t *proj,
                                     double time) {
  assert(proj->kind != AZ_PROJ_NOTHING);
  // The projectile still hasn't hit anything.  Apply kind-specific logic to
  // the projectile (e.g. homing projectiles will home in).
  switch (proj->kind) {
    case AZ_PROJ_GUN_CHARGED_NORMAL:
    case AZ_PROJ_GUN_CHARGED_TRIPLE:
      leave_particle_trail(state, proj, AZ_PAR_EMBER,
                           (az_color_t){255, 255, 255, 128}, 0.1, 6.0, 0.0);
      break;
    case AZ_PROJ_GUN_CHARGED_FREEZE:
      leave_particle_trail(state, proj, AZ_PAR_EMBER,
                           (az_color_t){0, 255, 255, 128}, 0.2, 6.0, 0.0);
      // fallthrough
    case AZ_PROJ_GUN_FREEZE:
    case AZ_PROJ_GUN_FREEZE_HOMING:
    case AZ_PROJ_GUN_FREEZE_BURST:
    case AZ_PROJ_GUN_FREEZE_SHRAPNEL:
    case AZ_PROJ_GUN_FREEZE_PIERCE:
      for (int i = (proj->kind == AZ_PROJ_GUN_CHARGED_FREEZE ? 2 : 1);
           i > 0; --i) {
        az_add_speck(state, (az_color_t){0, 255, 255, 255},
                     (proj->kind == AZ_PROJ_GUN_CHARGED_FREEZE ? 1.0 :
                      proj->kind == AZ_PROJ_GUN_FREEZE_SHRAPNEL ? 0.2 : 0.3),
                     proj->position, az_vpolar(30.0, az_random(0, AZ_TWO_PI)));
      }
      break;
    case AZ_PROJ_GUN_HOMING:
      az_add_speck(state, (az_color_t){0, 128, 255, 255}, 0.2,
                   proj->position, AZ_VZERO);
      break;
    case AZ_PROJ_GUN_CHARGED_HOMING:
      leave_particle_trail(state, proj, AZ_PAR_EMBER,
                           (az_color_t){0, 96, 255, 128}, 0.2, 8.0, 0.0);
      break;
    case AZ_PROJ_GUN_CHARGED_PHASE:
      leave_particle_trail(state, proj, AZ_PAR_TRAIL,
                           (az_color_t){255, 192, 0, 128},
                           1.5, proj->data->speed * time, 5.0);
      break;
    case AZ_PROJ_GUN_CHARGED_PIERCE:
      leave_particle_trail(state, proj, AZ_PAR_TRAIL,
                           (az_color_t){255, 0, 255, 128},
                           1.0, proj->data->speed * time, 8.0);
      if (times_per_second(20, proj, time)) {
        leave_particle_trail(state, proj, AZ_PAR_EXPLOSION,
                             (az_color_t){255, 0, 255, 128}, 0.5, 7.0, 0.0);
      }
      break;
    case AZ_PROJ_GUN_HOMING_PHASE:
      // Check if we're inside a wall.  Homing is turned off when we are.
      proj->param = 0;
      AZ_ARRAY_LOOP(wall, state->walls) {
        if (wall->kind == AZ_WALL_NOTHING) continue;
        if (az_point_touches_wall(wall, proj->position)) {
          proj->param = 1;
          break;
        }
      }
      az_add_speck(state, (proj->param == 0 ?
                           (az_color_t){0, 255, 128, 255} :
                           (az_color_t){255, 255, 0, 255}),
                   0.2, proj->position, AZ_VZERO);
      // Move faster when inside a wall.
      proj->velocity =
        az_vpolar(proj->data->speed * (proj->param == 0 ? 1.0 : 1.5),
                  proj->angle);
      break;
    case AZ_PROJ_GUN_HOMING_PIERCE:
      az_add_speck(state, (az_color_t){255, 0, 255, 255}, 0.3,
                   proj->position, AZ_VZERO);
      break;
    case AZ_PROJ_GUN_BURST_PIERCE:
      {
        az_impact_t impact;
        az_ray_impact(
            state, proj->position, az_vwithlen(proj->velocity, 30.0),
            ~AZ_IMPF_BADDIE, AZ_SHIP_UID, &impact);
        if (impact.type != AZ_IMP_NOTHING ||
            proj->age >= proj->data->lifetime) {
          for (int i = -2; i <= 2; ++i) {
            const double theta =
              proj->angle + 0.1 * AZ_PI * (i + az_random(-.5, .5));
            assert(proj->data->shrapnel_kind != AZ_PROJ_NOTHING);
            az_add_projectile(state, proj->data->shrapnel_kind, proj->position,
                              theta, proj->power, proj->fired_by);
          }
          proj->kind = AZ_PROJ_NOTHING;
        }
      }
      break;
    case AZ_PROJ_GUN_CHARGED_BEAM:
      assert(proj->fired_by == AZ_SHIP_UID);
      {
        const double radius =
          proj->data->splash_radius * (proj->age / proj->data->lifetime);
        // Destroy enemy projectiles within the blast:
        AZ_ARRAY_LOOP(other_proj, state->projectiles) {
          if (other_proj->kind == AZ_PROJ_NOTHING) continue;
          if (other_proj->fired_by != AZ_SHIP_UID &&
              !(other_proj->data->properties & AZ_PROJF_NO_HIT) &&
              az_vwithin(other_proj->position, proj->position, radius)) {
            az_expire_projectile(state, other_proj);
          }
        }
        // Damage enemies within the blast (over the lifetime of the blast):
        AZ_ARRAY_LOOP(baddie, state->baddies) {
          if (baddie->kind == AZ_BAD_NOTHING) continue;
          if (az_baddie_has_flag(baddie, AZ_BADF_INCORPOREAL)) continue;
          const az_component_data_t *component;
          if (az_circle_touches_baddie(baddie, radius, proj->position,
                                       &component)) {
            az_try_damage_baddie(state, baddie, component,
                                 proj->data->damage_kind,
                                 proj->data->splash_damage * proj->power *
                                 (time / proj->data->lifetime));
          }
        }
      }
      break;
    case AZ_PROJ_ROCKET:
      az_add_speck(state, (az_color_t){255, 255, 0, 255}, 1.0, proj->position,
                   az_vrotate(az_vmul(proj->velocity, -az_random(0, 0.3)),
                              (az_random(-AZ_DEG2RAD(30), AZ_DEG2RAD(30)))));
      break;
    case AZ_PROJ_HYPER_ROCKET:
      for (int i = 0; i < 6; ++i) {
        az_add_speck(state, (az_color_t){255, 255, 0, 255},
                     0.5 + 0.1 * i, proj->position,
                     az_vrotate(az_vmul(proj->velocity, -az_random(0, 0.3)),
                                (az_random(-AZ_DEG2RAD(5), AZ_DEG2RAD(5)))));
      }
      break;
    case AZ_PROJ_MISSILE_FREEZE:
      leave_missile_trail(state, proj, time, (az_color_t){0, 192, 255, 255});
      break;
    case AZ_PROJ_MISSILE_BARRAGE:
      for (int i = 0; i < 4; ++i) {
        const double threshold = 0.33 * proj->data->lifetime * i;
        if (proj->age > threshold && proj->age - time <= threshold) {
          const double offset = 24 * i;
          for (int j = (i == 0); j <= 1; ++j) {
            az_add_projectile(
                state, AZ_PROJ_MISSILE_TRIPLE,
                az_vadd(proj->position, az_vpolar((j ? offset : -offset),
                                                  proj->angle + AZ_HALF_PI)),
                proj->angle, proj->power, proj->fired_by);
          }
        }
      }
      break;
    case AZ_PROJ_MISSILE_TRIPLE:
      leave_missile_trail(state, proj, time, (az_color_t){0, 255, 0, 255});
      break;
    case AZ_PROJ_MISSILE_HOMING:
      leave_missile_trail(state, proj, time, (az_color_t){128, 192, 255, 255});
      break;
    case AZ_PROJ_MISSILE_PHASE:
      leave_missile_trail(state, proj, time, (az_color_t){192, 192, 64, 255});
      proj->velocity = az_vrotate((az_vector_t){proj->data->speed,
            proj->param * proj->data->speed * cos(30.0 * proj->age)},
        proj->angle);
      break;
    case AZ_PROJ_MISSILE_BURST:
      leave_missile_trail(state, proj, time, (az_color_t){192, 96, 0, 255});
      {
        az_impact_t impact;
        az_ray_impact(
            state, proj->position, az_vwithlen(proj->velocity, 100.0),
            (AZ_IMPF_SHIP | AZ_IMPF_BADDIE), AZ_SHIP_UID, &impact);
        if (impact.type != AZ_IMP_NOTHING ||
            proj->age >= proj->data->lifetime) {
          for (int i = 0; i < 360; i += 40) {
            az_add_projectile(state, AZ_PROJ_ROCKET, proj->position,
                              az_mod2pi(proj->angle + AZ_DEG2RAD(i)),
                              proj->power, proj->fired_by);
          }
          proj->kind = AZ_PROJ_NOTHING;
          az_play_sound(&state->soundboard, AZ_SND_FIRE_ROCKET);
        }
      }
      break;
    case AZ_PROJ_MISSILE_PIERCE:
      leave_missile_trail(state, proj, time, (az_color_t){255, 0, 255, 255});
      break;
    case AZ_PROJ_MISSILE_BEAM:
      if (proj->age >= proj->data->lifetime) {
        // Calculate the impact point, starting from the ship.
        const az_vector_t beam_start =
          az_vadd(state->ship.position, az_vpolar(20.0, state->ship.angle));
        az_impact_t impact;
        az_ray_impact(
            state, beam_start, az_vpolar(1000.0, state->ship.angle),
            AZ_IMPF_SHIP, AZ_SHIP_UID, &impact);
        proj->position = impact.position;
        proj->angle = state->ship.angle;
        // Explode at the impact point.
        if (impact.type == AZ_IMP_BADDIE) {
          on_projectile_hit_baddie(
              state, proj, impact.target.baddie.baddie,
              impact.target.baddie.component, impact.normal);
        } else {
          on_projectile_hit_wall(state, proj, impact.normal);
        }
        // Add a particle for the beam.
        az_add_beam(state, (az_color_t){255, 64, 0, 192}, beam_start,
                    impact.position, 0.3, 5.0);
        az_play_sound(&state->soundboard, AZ_SND_FIRE_MISSILE_BEAM);
      } else {
        proj->position =
          az_vadd(state->ship.position, az_vpolar(20.0, state->ship.angle));
        proj->angle = az_mod2pi(proj->angle + 5.0 * time);
      }
      break;
    case AZ_PROJ_BOMB:
    case AZ_PROJ_MEGA_BOMB:
    case AZ_PROJ_MAGMA_EXPLOSION:
    case AZ_PROJ_MEDIUM_EXPLOSION:
    case AZ_PROJ_NUCLEAR_EXPLOSION:
      if (proj->age >= proj->data->lifetime) {
        on_projectile_hit_wall(state, proj, AZ_VZERO);
        return;
      }
      if (proj->kind == AZ_PROJ_BOMB &&
          !az_vwithin(proj->position, state->ship.position,
                      proj->data->splash_radius *
                      AZ_ATTUNED_EXPLOSIVES_RADIUS_FACTOR + 5.0)) {
        az_impact_t impact;
        az_circle_impact(state, 25.0, proj->position, AZ_VZERO,
                         ~AZ_IMPF_BADDIE, AZ_NULL_UID, &impact);
        if (impact.type == AZ_IMP_BADDIE) {
          on_projectile_hit_wall(state, proj, AZ_VZERO);
          return;
        }
      }
      if (proj->kind == AZ_PROJ_MEGA_BOMB && proj->age >= 0.25 &&
          times_per_second((proj->age < 2.0 ? 2 : 6), proj, time)) {
        az_play_sound(&state->soundboard, AZ_SND_BLINK_MEGA_BOMB);
      }
      proj->angle = az_mod2pi(proj->angle + 1.5 * time);
      break;
    case AZ_PROJ_ORION_BOMB:
      if (proj->age >= proj->data->lifetime) {
        az_projectile_t *blast = az_add_projectile(
            state, AZ_PROJ_ORION_BLAST, proj->position, proj->angle,
            proj->power, proj->fired_by);
        if (blast != NULL) {
          blast->velocity = az_vmul(az_vsub(
              proj->velocity, az_vpolar(proj->data->speed, proj->angle)), 0.5);
        }
        on_projectile_hit_wall(state, proj, AZ_VZERO);
      }
      break;
    case AZ_PROJ_ORION_BLAST:
      {
        const double factor = proj->age / proj->data->lifetime;
        const double radius = proj->data->splash_radius * factor * factor;
        // Propel the ship away from the blast:
        if (az_ship_is_alive(&state->ship) &&
            az_vwithin(state->ship.position, proj->position, radius)) {
          az_vpluseq(&state->ship.velocity, az_vwithlen(
              az_vsub(state->ship.position, proj->position),
              (500 + 5000 * factor * factor) * time));
        }
        // Damage enemies within the blast (over the lifetime of the blast):
        AZ_ARRAY_LOOP(baddie, state->baddies) {
          if (baddie->kind == AZ_BAD_NOTHING) continue;
          if (az_baddie_has_flag(baddie, AZ_BADF_INCORPOREAL)) continue;
          const az_component_data_t *component;
          if (az_circle_touches_baddie(baddie, 0.8 * radius, proj->position,
                                       &component)) {
            az_try_damage_baddie(state, baddie, component,
                                 proj->data->damage_kind,
                                 proj->data->splash_damage * proj->power *
                                 (time / proj->data->lifetime));
          }
        }
      }
      break;
    case AZ_PROJ_BOUNCING_FIREBALL:
      if (times_per_second(15, proj, time)) {
        leave_particle_trail(state, proj, AZ_PAR_EMBER,
                             (az_color_t){255, 128, 0, 128}, 0.3, 15.0, 0.0);
      }
      break;
    case AZ_PROJ_ERUPTION:
      if (times_per_second(20, proj, time)) {
        on_projectile_impact(state, proj, proj->velocity);
      }
      break;
    case AZ_PROJ_FIREBALL_FAST:
    case AZ_PROJ_FIREBALL_SLOW:
      leave_particle_trail(state, proj, AZ_PAR_EMBER,
                           (az_color_t){255, 128, 0, 128}, 0.1, 5.0, 0.0);
      break;
    case AZ_PROJ_FORCE_WAVE: {
      const double new_speed = az_vnorm(proj->velocity) + 700 * time;
      if (proj->age >= 0.5) {
        proj->velocity = az_vwithlen(
            az_vflatten(proj->velocity, proj->position), new_speed);
        proj->angle = az_vtheta(proj->velocity);
      } else {
        proj->velocity = az_vwithlen(proj->velocity, new_speed);
      }
      const double factor = fmin(1.0, 2.0 * proj->age);
      const az_vector_t vertices[] = {
        {0, -50 * factor}, {0, 50 * factor},
        {-100 * factor, 50 * factor}, {-100 * factor, -50 * factor}
      };
      const az_polygon_t polygon = AZ_INIT_POLYGON(vertices);
      AZ_ARRAY_LOOP(baddie, state->baddies) {
        if (baddie->kind != AZ_BAD_FORCE_EGG) continue;
        if (az_circle_touches_polygon_trans(
                polygon, proj->position, proj->angle, 1,
                baddie->position)) {
          az_vpluseq(&baddie->velocity, az_vmul(proj->velocity, 5 * time));
        }
      }
      if (az_ship_is_alive(&state->ship)) {
        if (az_circle_touches_polygon_trans(
                polygon, proj->position, proj->angle, 1,
                state->ship.position)) {
          az_vpluseq(&state->ship.velocity, az_vmul(proj->velocity, 5 * time));
        }
      }
    } break;
    case AZ_PROJ_GRENADE:
      if (times_per_second(15, proj, time)) {
        leave_particle_trail(state, proj, AZ_PAR_EMBER,
                             (az_color_t){128, 128, 128, 128}, 0.5, 8.0, 0);
      }
      az_vpluseq(&proj->velocity, az_vwithlen(proj->position, -150 * time));
      proj->angle = az_mod2pi(proj->angle + AZ_DEG2RAD(360) * time);
      break;
    case AZ_PROJ_GRAVITY_TORPEDO:
      leave_missile_trail(state, proj, time, (az_color_t){64, 64, 192, 255});
      proj->velocity = az_vrotate((az_vector_t){proj->data->speed,
            proj->data->speed * cos(7.0 * proj->age)}, proj->angle);
      break;
    case AZ_PROJ_GRAVITY_TORPEDO_WELL:
      assert(proj->fired_by != AZ_SHIP_UID);
      if (az_ship_is_alive(&state->ship) &&
          az_vwithin(proj->position, state->ship.position, 100.0)) {
        az_vpluseq(&state->ship.velocity, az_vwithlen(
            az_vsub(proj->position, state->ship.position),
            time * 800.0 * (1.0 - proj->age / proj->data->lifetime)));
      }
      break;
    case AZ_PROJ_ICE_TORPEDO:
      az_add_speck(state, (az_color_t){0, 255, 255, 255}, az_random(0.2, 1.0),
                   az_vadd(proj->position,
                           az_vpolar(az_random(-8.0, 8.0),
                                     proj->angle + AZ_HALF_PI)), AZ_VZERO);
      break;
    case AZ_PROJ_MAGNET_FUSION_BEAM: {
      // Calculate the impact point.
      const az_vector_t beam_start = proj->position;
      az_impact_t impact;
      az_ray_impact(state, beam_start, az_vpolar(10000.0, proj->angle),
                    AZ_IMPF_BADDIE, proj->fired_by, &impact);
      proj->position = impact.position;
      // Explode at the impact point.
      if (impact.type == AZ_IMP_SHIP) {
        on_projectile_hit_ship(state, proj, impact.normal);
      } else on_projectile_hit_wall(state, proj, impact.normal);
      // Add a particle for the beam.
      az_add_beam(state, (az_color_t){255, 64, 0, 192}, beam_start,
                  impact.position, 0.5, 5.0);
      az_play_sound(&state->soundboard, AZ_SND_FIRE_MISSILE_BEAM);
    } break;
    case AZ_PROJ_MYCOSPORE:
      proj->velocity = az_vrotate((az_vector_t){proj->data->speed,
            proj->data->speed * cos(7.0 * proj->age)}, proj->angle);
      break;
    case AZ_PROJ_OTH_HOMING:
      leave_particle_trail(state, proj, AZ_PAR_OTH_FRAGMENT, AZ_WHITE,
                           0.2, 4.0, AZ_DEG2RAD(720));
      break;
    case AZ_PROJ_OTH_MINIROCKET:
      leave_particle_trail(state, proj, AZ_PAR_OTH_FRAGMENT, AZ_WHITE,
                           0.3, 6.0, AZ_DEG2RAD(720));
      break;
    case AZ_PROJ_OTH_ROCKET:
      leave_particle_trail(state, proj, AZ_PAR_OTH_FRAGMENT, AZ_WHITE,
                           0.5, 9.0, AZ_DEG2RAD(720));
      break;
    case AZ_PROJ_OTH_SPRAY:
      if (az_clock_mod(2, 1, state->clock)) {
        leave_particle_trail(state, proj, AZ_PAR_OTH_FRAGMENT, AZ_WHITE,
                             0.1, 5.0, AZ_DEG2RAD(360));
      }
      break;
    case AZ_PROJ_PLANETARY_EXPLOSION: {
      // Kill everything (baddies and ship) below the shockwave:
      const double rho = az_vnorm(proj->position);
      AZ_ARRAY_LOOP(baddie, state->baddies) {
        if (baddie->kind == AZ_BAD_NOTHING) continue;
        if (az_baddie_has_flag(baddie, AZ_BADF_INCORPOREAL)) continue;
        if (az_vwithin(baddie->position, AZ_VZERO, rho)) {
          az_kill_baddie(state, baddie);
        }
      }
      if (az_ship_is_alive(&state->ship) &&
          az_vwithin(state->ship.position, AZ_VZERO, rho)) {
        az_kill_ship(state);
      }
      // Create minor explosions along the way:
      const double center_theta = az_vtheta(state->camera.center);
      const double theta_span =
        atan(AZ_SCREEN_WIDTH / az_vnorm(state->camera.center));
      const double step = 0.1 * theta_span;
      for (double theta = center_theta - theta_span;
           theta < center_theta + theta_span; theta += step) {
        if (az_random(0, 1) < 1.0 - pow(0.5, 3.0 * time)) {
          az_projectile_t subproj;
          az_init_projectile(
              &subproj, AZ_PROJ_PLANETARY_EXPLOSION,
              az_vpolar(rho, theta + step * az_random(-0.5, 0.5)),
              0, 1, AZ_NULL_UID);
          on_projectile_hit_wall(state, &subproj, AZ_VZERO);
        }
      }
    } break;
    case AZ_PROJ_PRISMATIC_WALL: {
      az_vector_t vertices[4];
      az_get_prismatic_wall_vertices(proj, vertices);
      const az_polygon_t polygon = AZ_INIT_POLYGON(vertices);
      if (az_ship_is_alive(&state->ship) &&
          az_polygon_contains(polygon, az_vrotate(
              az_vsub(state->ship.position, proj->position), -proj->angle))) {
        const double damage = proj->data->splash_damage * proj->power * time;
        az_damage_ship(state, damage, false);
      }
    } break;
    case AZ_PROJ_SCRAP_METAL:
      proj->angle = az_mod2pi(proj->angle + AZ_DEG2RAD(720) * time);
      if (times_per_second(20, proj, time)) {
        leave_particle_trail(state, proj, AZ_PAR_EMBER,
                             (az_color_t){255, 0, 128, 128}, 0.3, 8.0, 0.0);
      }
      break;
    case AZ_PROJ_SCRAP_SHRAPNEL:
      proj->angle = az_mod2pi(proj->angle + AZ_DEG2RAD(360) * time);
      if (times_per_second(30, proj, time)) {
        az_add_speck(state, (az_color_t){255, 0, 128, 255}, 0.2,
                     proj->position, AZ_VZERO);
      }
      break;
    case AZ_PROJ_SPARK:
      leave_particle_trail(state, proj, AZ_PAR_SPARK,
                           (az_color_t){0, 255, 0, 255}, 0.1, 6.0,
                           AZ_DEG2RAD(300));
      proj->velocity =
        az_vadd(proj->velocity, az_vwithlen(proj->position, -600 * time));
      break;
    case AZ_PROJ_TRINE_TORPEDO:
      assert(proj->fired_by != AZ_SHIP_UID);
      if (az_ship_is_decloaked(&state->ship) &&
          az_vwithin(proj->position, state->ship.position, 100.0)) {
        const az_vector_t position = proj->position;
        const double power = proj->power;
        const az_uid_t fired_by = proj->fired_by;
        proj->kind = AZ_PROJ_NOTHING; // We cannot use proj after this point.
        const double base_angle = az_random(0, AZ_TWO_PI);
        for (int i = 0; i < 3; ++i) {
          az_projectile_t *expander = az_add_projectile(
              state, AZ_PROJ_TRINE_TORPEDO_EXPANDER, position,
              base_angle + i * AZ_DEG2RAD(120), power, fired_by);
          if (expander != NULL) expander->age += 0.1 * i;
        }
      } else {
        leave_missile_trail(state, proj, time, (az_color_t){192, 64, 64, 255});
        proj->velocity = az_vrotate((az_vector_t){proj->data->speed,
              proj->data->speed * cos(7.0 * proj->age)}, proj->angle);
      }
      break;
    case AZ_PROJ_TRINE_TORPEDO_EXPANDER:
      assert(proj->fired_by != AZ_SHIP_UID);
      if (proj->age >= proj->data->lifetime) {
        const az_vector_t position = proj->position;
        const double power = proj->power;
        const az_uid_t fired_by = proj->fired_by;
        double goal_angle = proj->angle + AZ_PI;
        proj->kind = AZ_PROJ_NOTHING; // We cannot use proj after this point.
        if (az_ship_is_decloaked(&state->ship)) {
          goal_angle = az_vtheta(az_vsub(state->ship.position, position));
        }
        az_add_projectile(state, AZ_PROJ_TRINE_TORPEDO_FIREBALL, position,
                          goal_angle, power, fired_by);
        az_play_sound(&state->soundboard, AZ_SND_FIRE_ROCKET);
      } else {
        leave_missile_trail(state, proj, time, (az_color_t){192, 64, 64, 255});
        proj->velocity = az_vrotate((az_vector_t){proj->data->speed,
              proj->data->speed * cos(7.0 * proj->age)}, proj->angle);
      }
      break;
    case AZ_PROJ_TRINE_TORPEDO_FIREBALL:
      leave_particle_trail(state, proj, AZ_PAR_EMBER,
                           (az_color_t){255, 128, 0, 128}, 0.1, 10.0, 0.0);
      break;
    default: break;
  }
}

static void on_spiked_vine_seed_hit_wall(
    az_space_state_t *state, az_projectile_t *proj, az_wall_t *wall,
    az_vector_t normal) {
  assert(proj->kind == AZ_PROJ_SPIKED_VINE_SEED);
  const int wall_index = az_wall_data_index(wall->data);
  if (wall_index != 45 && wall_index != 59 && wall_index != 60 &&
      wall_index != 61) return;
  AZ_ARRAY_LOOP(baddie, state->baddies) {
    if (baddie->kind != AZ_BAD_SPIKED_VINE) continue;
    if (az_circle_touches_baddie(baddie, 40.0, proj->position, NULL)) return;
  }
  const double angle =
    az_vnonzero(normal) ? az_vtheta(normal) : az_mod2pi(proj->angle + AZ_PI);
  az_add_baddie(state, AZ_BAD_SPIKED_VINE,
                az_vadd(proj->position, az_vpolar(-5.0, angle)), angle);
}

static void tick_projectile(az_space_state_t *state, az_projectile_t *proj,
                            double time) {
  // Age the projectile, and remove it if it is expired.
  proj->age += time;
  projectile_special_logic(state, proj, time);
  if (proj->kind == AZ_PROJ_NOTHING) return;
  if (proj->age > proj->data->lifetime) {
    az_expire_projectile(state, proj);
    return;
  }

  // Figure out what, if anything, the projectile hits:
  az_impact_t impact = {.type = AZ_IMP_NOTHING};
  if (proj->data->properties & AZ_PROJF_NO_HIT) {
    impact.position = az_vadd(proj->position, az_vmul(proj->velocity, time));
  } else {
    az_impact_flags_t skip_types =
      (proj->fired_by == AZ_SHIP_UID ? AZ_IMPF_SHIP :
       proj->fired_by == AZ_NULL_UID ? 0 : AZ_IMPF_BADDIE);
    if (proj->data->properties & AZ_PROJF_PHASED) {
      skip_types |= AZ_IMPF_WALL | AZ_IMPF_DOOR_INSIDE | AZ_IMPF_DOOR_OUTSIDE;
    }
    if (proj->data->properties & AZ_PROJF_PIERCING) {
      skip_types |= AZ_IMPF_SHIP | AZ_IMPF_BADDIE;
    }
    az_ray_impact(state, proj->position, az_vmul(proj->velocity, time),
                  skip_types, proj->last_hit_uid, &impact);
    const az_vector_t delta = az_vsub(impact.position, proj->position);

    if (proj->data->properties & AZ_PROJF_PHASED) {
      // If this phased projectile was fired by the ship, also hit all doors
      // along the way.
      if (proj->fired_by == AZ_SHIP_UID) {
        AZ_ARRAY_LOOP(door, state->doors) {
          if (door->kind == AZ_DOOR_NOTHING) continue;
          if (az_ray_hits_door_outside(door, proj->position, delta,
                                       NULL, NULL)) {
            az_try_open_door(state, door, proj->data->damage_kind);
          }
        }
      }
      // For charged phased projectiles, also hit breakable walls on the way.
      // We only do this for _charged_ phased projectiles because we don't want
      // to apply this behavior to, say, phased rockets.
      if (proj->data->damage_kind & AZ_DMGF_CHARGED) {
        AZ_ARRAY_LOOP(wall, state->walls) {
          if (wall->kind == AZ_WALL_NOTHING ||
              wall->kind == AZ_WALL_INDESTRUCTIBLE) continue;
          az_vector_t point;
          if (az_ray_hits_wall(wall, proj->position, delta, &point, NULL)) {
            az_try_break_wall(state, wall, proj->data->damage_kind, point);
          }
        }
      }
    }

    // For piercing projectiles, hit all targets along the way.
    if (proj->data->properties & AZ_PROJF_PIERCING) {
      const az_vector_t start = proj->position;
      if (proj->fired_by != AZ_SHIP_UID && proj->last_hit_uid != AZ_SHIP_UID) {
        az_vector_t normal;
        if (az_ray_hits_ship(&state->ship, start, delta,
                             &proj->position, &normal)) {
          on_projectile_hit_ship(state, proj, normal);
          assert(proj->kind != AZ_PROJ_NOTHING);
        }
      }
      if (proj->fired_by == AZ_SHIP_UID || proj->fired_by == AZ_NULL_UID) {
        AZ_ARRAY_LOOP(baddie, state->baddies) {
          if (baddie->kind == AZ_BAD_NOTHING) continue;
          if (baddie->uid == proj->last_hit_uid) continue;
          if (az_baddie_has_flag(baddie, AZ_BADF_INCORPOREAL)) continue;
          az_vector_t normal;
          const az_component_data_t *component;
          if (az_ray_hits_baddie(baddie, start, delta,
                                 &proj->position, &normal, &component)) {
            on_projectile_hit_baddie(state, proj, baddie, component, normal);
            assert(proj->kind != AZ_PROJ_NOTHING);
          }
        }
      }
    }
  }

  // Move the projectile:
  proj->position = impact.position;

  // Resolve the impact (if any):
  switch (impact.type) {
    case AZ_IMP_NOTHING:
      if (proj->data->homing_rate != 0.0) {
        projectile_home_in(state, proj, time);
      }
      break;
    case AZ_IMP_BADDIE:
      on_projectile_hit_baddie(state, proj, impact.target.baddie.baddie,
                               impact.target.baddie.component, impact.normal);
      break;
    case AZ_IMP_SHIP:
      on_projectile_hit_ship(state, proj, impact.normal);
      break;
    case AZ_IMP_DOOR_OUTSIDE:
      if (proj->fired_by == AZ_SHIP_UID) {
        az_try_open_door(state, impact.target.door, proj->data->damage_kind);
      }
      on_projectile_hit_wall(state, proj, impact.normal);
      break;
    case AZ_IMP_DOOR_INSIDE:
      on_projectile_hit_wall(state, proj, impact.normal);
      break;
    case AZ_IMP_WALL:
      az_try_break_wall(state, impact.target.wall, proj->data->damage_kind,
                        proj->position);
      if (proj->kind == AZ_PROJ_SPIKED_VINE_SEED) {
        on_spiked_vine_seed_hit_wall(state, proj, impact.target.wall,
                                     impact.normal);
      }
      on_projectile_hit_wall(state, proj, impact.normal);
      break;
  }
}

void az_tick_projectiles(az_space_state_t *state, double time) {
  AZ_ARRAY_LOOP(proj, state->projectiles) {
    if (proj->kind == AZ_PROJ_NOTHING) continue;
    tick_projectile(state, proj, time);
  }
}

/*===========================================================================*/
