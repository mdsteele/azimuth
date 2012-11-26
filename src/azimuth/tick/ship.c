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

#include "azimuth/tick/ship.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h> // for NULL

#include "azimuth/constants.h"
#include "azimuth/state/particle.h"
#include "azimuth/state/projectile.h"
#include "azimuth/state/ship.h"
#include "azimuth/state/space.h"
#include "azimuth/util/color.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/random.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

// The time needed to charge the CHARGE gun, in seconds:
#define AZ_CHARGE_GUN_CHARGING_TIME 0.6
// The radius of the ship for purposes of collisions with e.g. walls:
#define AZ_SHIP_DEFLECTOR_RADIUS 15.0

typedef enum {
  AZ_COL_NOTHING = 0,
  AZ_COL_BADDIE,
  AZ_COL_DOOR_OUTSIDE,
  AZ_COL_DOOR_INSIDE,
  AZ_COL_WALL
} az_collide_type_t;

typedef struct {
  az_collide_type_t type;
  union {
    az_baddie_t *baddie;
    az_door_t *door;
    az_wall_t *wall;
  } target;
} az_collide_target_t;

static void recharge_ship_energy(az_player_t *player, double time) {
  const double recharge_rate = AZ_SHIP_BASE_RECHARGE_RATE +
    (az_has_upgrade(player, AZ_UPG_FUSION_REACTOR) ?
     AZ_FUSION_REACTOR_RECHARGE_RATE : 0.0) +
    (az_has_upgrade(player, AZ_UPG_QUANTUM_REACTOR) ?
     AZ_QUANTUM_REACTOR_RECHARGE_RATE : 0.0);
  player->energy = az_dmin(player->max_energy,
                           player->energy + recharge_rate * time);
}

static void on_ship_enter_door(az_space_state_t *state, az_door_t *door) {
  state->mode = AZ_MODE_DOORWAY;
  state->mode_data.doorway.step = AZ_DWS_FADE_OUT;
  state->mode_data.doorway.progress = 0.0;
  state->mode_data.doorway.door = door;
}

static void on_ship_hit_wall(az_space_state_t *state,
                             double elasticity, double impact_damage_coeff,
                             az_vector_t impact_point) {
  az_ship_t *ship = &state->ship;
  const az_vector_t impact_normal = az_vsub(ship->position, impact_point);
  // Push the ship slightly away from the impact point (so that we're
  // hopefully no longer in contact with the wall).
  if (impact_normal.x != 0.0 || impact_normal.y != 0.0) {
    ship->position = az_vadd(ship->position,
                             az_vmul(az_vunit(impact_normal), 0.5));
  }
  // Bounce the ship off the wall.
  const double old_speed_fraction =
    az_vnorm(ship->velocity) / AZ_SHIP_BASE_MAX_SPEED;
  ship->velocity = az_vsub(ship->velocity,
                           az_vmul(az_vproj(ship->velocity, impact_normal),
                                   1.0 + elasticity));
  // Damage the ship.
  az_damage_ship(state, impact_damage_coeff * old_speed_fraction);
  // Put a particle at the impact point.
  az_particle_t *particle;
  if (az_insert_particle(state, &particle)) {
    particle->kind = AZ_PAR_BOOM;
    particle->color = (az_color_t){255, 255, 255, 255};
    particle->position = impact_point;
    particle->velocity = AZ_VZERO;
    particle->lifetime = 0.3;
    particle->param1 = 10;
  }
}

static const az_node_t *choose_nearby_node(const az_space_state_t *state) {
  const az_node_t *best_node = NULL;
  double best_dist = 50.0;
  AZ_ARRAY_LOOP(node, state->nodes) {
    if (node->kind == AZ_NODE_NOTHING) continue;
    if (node->kind == AZ_NODE_TRACTOR) continue;
    const double dist = az_vdist(node->position, state->ship.position);
    if (dist <= best_dist) {
      best_dist = dist;
      best_node = node;
    }
  }
  return best_node;
}

static void apply_gravity_to_ship(az_ship_t *ship, double time) {
  // Gravity works as follows.  By the spherical shell theorem, the force of
  // gravity on the ship (while inside the planetoid) varies linearly with the
  // distance from the planetoid's center, so the change in velocity is time *
  // surface_gravity * vnorm(position) / planetoid_radius.  However, we should
  // then multiply this scalar by the unit vector pointing from the ship
  // towards the core, which is -position/vnorm(position).  The vnorm(position)
  // factors cancel and we end up with what we have here.
  ship->velocity = az_vadd(ship->velocity,
      az_vmul(ship->position, -time *
              (AZ_PLANETOID_SURFACE_GRAVITY / AZ_PLANETOID_RADIUS)));
}

static void apply_drag_to_ship(az_ship_t *ship, double time) {
  const double base_max_speed =
    AZ_SHIP_BASE_MAX_SPEED *
    (az_has_upgrade(&ship->player, AZ_UPG_DYNAMIC_ARMOR) ?
     AZ_DYNAMIC_ARMOR_SPEED_MULT : 1.0);
  const double drag_coeff =
    AZ_SHIP_BASE_THRUST_ACCEL / (base_max_speed * base_max_speed);
  const az_vector_t drag_force =
    az_vmul(ship->velocity, -drag_coeff * az_vnorm(ship->velocity));
  ship->velocity = az_vadd(ship->velocity, az_vmul(drag_force, time));
}

/*===========================================================================*/

static void fire_gun_multi(az_space_state_t *state, double energy_mult,
                           az_proj_kind_t kind, int num_shots, double dtheta,
                           double theta_offset) {
  const double energy_cost = 20.0 * energy_mult;
  az_ship_t *ship = &state->ship;
  if (ship->player.energy < energy_cost) return;
  ship->player.energy -= energy_cost;
  const az_vector_t gun_position =
    az_vadd(ship->position, az_vpolar(18, ship->angle));
  double theta = 0.0;
  for (int i = 0; i < num_shots; ++i) {
    az_projectile_t *proj;
    if (az_insert_projectile(state, &proj)) {
      az_init_projectile(proj, kind, false, gun_position,
                         ship->angle + theta_offset + theta);
    } else break;
    theta = -theta;
    if (i % 2 == 0) theta += dtheta;
  }
}

static void fire_gun_single(az_space_state_t *state, double energy_mult,
                            az_proj_kind_t kind) {
  fire_gun_multi(state, energy_mult, kind, 1, 0.0, 0.0);
}

static void beam_emit_particles(az_space_state_t *state, az_vector_t position,
                                az_vector_t normal, az_color_t color) {
  az_particle_t *particle;
  if (az_insert_particle(state, &particle)) {
    particle->kind = AZ_PAR_SPECK;
    particle->color = color;
    particle->position = position;
    particle->velocity = az_vpolar(20.0 + 50.0 * az_random(),
                                   az_vtheta(normal) +
                                   (az_random() - 0.5) * AZ_PI);
    particle->angle = 0.0;
    particle->lifetime = 1.0;
  }
}

static void fire_beam(az_space_state_t *state, az_gun_t minor, double time) {
  az_ship_t *ship = &state->ship;
  if (!ship->controls.fire_held) return;

  // Determine how much energy the beam needs and how much damage it'll do:
  double energy_cost = AZ_BEAM_GUN_BASE_ENERGY_PER_SECOND * time;
  double damage_mult = 1.0;
  switch (minor) {
    case AZ_GUN_NONE: break;
    case AZ_GUN_CHARGE: AZ_ASSERT_UNREACHABLE();
    case AZ_GUN_FREEZE: energy_cost *= 1.5; break;
    case AZ_GUN_TRIPLE: energy_cost *= 2.0; damage_mult *= 0.7; break;
    case AZ_GUN_HOMING: energy_cost *= 2.0; damage_mult *= 0.5; break;
    case AZ_GUN_PHASE: energy_cost *= 2.0; break;
    case AZ_GUN_BURST: energy_cost *= 2.5; break;
    case AZ_GUN_PIERCE: energy_cost *= 3.0; damage_mult *= 2.0; break;
    case AZ_GUN_BEAM: AZ_ASSERT_UNREACHABLE();
  }
  if (ship->player.energy < energy_cost) {
    ship->player.energy = 0.0;
    return;
  }
  ship->player.energy -= energy_cost;

  az_vector_t beam_start =
    az_vadd(ship->position, az_vpolar(18, ship->angle));
  double beam_init_angle = ship->angle;
  // Normally, there will be a single beam.  But for a TRIPLE/BEAM gun, there
  // are three beams, and for a BURST/BEAM gun, the beam reflects off of
  // whatever it hits, creating a second beam.
  const int num_beams = (minor == AZ_GUN_TRIPLE ? 3 :
                         minor == AZ_GUN_BURST ? 3 : 1);
  for (int beam_index = 0; beam_index < num_beams; ++beam_index) {
    // Determine how much damage the beam should do.  If this is a BURST beam,
    // it loses power with each reflection.
    if (minor == AZ_GUN_BURST && beam_index > 0) {
      damage_mult *= 0.5;
    }
    const double damage =
      AZ_BEAM_GUN_BASE_DAMAGE_PER_SECOND * damage_mult * time;

    // Determine the angle of the beam.  If this is a TRIPLE beam, the second
    // and third beams are at offset angles; if this is a HOMING beam, the
    // angle will point towards the nearest baddie.
    double beam_angle = beam_init_angle;
    if (minor == AZ_GUN_TRIPLE) {
      if (beam_index == 1) beam_angle += AZ_DEG2RAD(10);
      else if (beam_index == 2) beam_angle -= AZ_DEG2RAD(10);
    } else if (minor == AZ_GUN_HOMING) {
      double best_dist = INFINITY;
      AZ_ARRAY_LOOP(baddie, state->baddies) {
        if (baddie->kind == AZ_BAD_NOTHING) continue;
        const az_vector_t delta = az_vsub(baddie->position, beam_start);
        const double dist = az_vnorm(delta);
        if (dist >= best_dist) continue;
        const double angle = az_vtheta(delta);
        if (fabs(az_mod2pi(angle - beam_init_angle)) <= AZ_DEG2RAD(30)) {
          best_dist = dist;
          beam_angle = angle;
        }
      }
    }

    // Determine what the beam hits (if anything), and calculate its ending
    // point and the normal vector at impact.
    az_vector_t delta = az_vpolar(2.5 * AZ_SCREEN_RADIUS, beam_angle);
    az_vector_t normal = AZ_VZERO;
    bool did_hit = false;
    az_color_t hit_color = AZ_WHITE;
    az_door_t *hit_door = NULL;
    if (minor != AZ_GUN_PHASE) {
      AZ_ARRAY_LOOP(door, state->doors) {
        if (door->kind == AZ_DOOR_NOTHING) continue;
        az_vector_t hit_at;
        if (az_ray_hits_door(door, beam_start, delta, &hit_at, &normal)) {
          did_hit = true;
          delta = az_vsub(hit_at, beam_start);
          hit_door = door;
        }
      }
      AZ_ARRAY_LOOP(wall, state->walls) {
        if (wall->kind == AZ_WALL_NOTHING) continue;
        az_vector_t hit_at;
        if (az_ray_hits_wall(wall, beam_start, delta, &hit_at, &normal)) {
          did_hit = true;
          delta = az_vsub(hit_at, beam_start);
          hit_color = wall->data->color;
        }
      }
    }
    az_baddie_t *hit_baddie = NULL;
    AZ_ARRAY_LOOP(baddie, state->baddies) {
      if (baddie->kind == AZ_BAD_NOTHING) continue;
      az_vector_t hit_at, baddie_normal;
      if (az_ray_hits_baddie(baddie, beam_start, delta, &hit_at,
                             &baddie_normal)) {
        did_hit = true;
        if (minor == AZ_GUN_PIERCE) {
          beam_emit_particles(state, hit_at, baddie_normal, AZ_WHITE);
          az_damage_baddie(state, baddie, damage);
        } else {
          delta = az_vsub(hit_at, beam_start);
          normal = baddie_normal;
          hit_color = AZ_WHITE;
          hit_baddie = baddie;
          hit_door = NULL;
        }
      }
    }
    const az_vector_t beam_end = az_vadd(beam_start, delta);

    // Add a particle for the beam itself:
    az_particle_t *particle;
    if (az_insert_particle(state, &particle)) {
      particle->kind = AZ_PAR_BEAM;
      const uint8_t alt = 32 * az_clock_zigzag(6, 1, state->clock);
      particle->color = (az_color_t){
        (minor == AZ_GUN_FREEZE ? alt : 255),
        (minor == AZ_GUN_FREEZE ? 128 : minor == AZ_GUN_PIERCE ? 0 : alt),
        (minor == AZ_GUN_FREEZE ? 255 : minor == AZ_GUN_PHASE ? 0 : alt),
        192};
      particle->position = beam_start;
      particle->velocity = AZ_VZERO;
      particle->angle = beam_angle;
      particle->lifetime = 0.0;
      particle->param1 = az_vnorm(delta);
      particle->param2 =
        sqrt(damage_mult) * (3.0 + 0.5 * az_clock_zigzag(8, 1, state->clock));
    }

    // Resolve hits:
    if (did_hit) {
      // Add particles off of whatever the beam hits:
      beam_emit_particles(state, beam_end, normal, hit_color);
      // If we hit a (normal) door, open the door.
      if (hit_door != NULL) {
        assert(minor != AZ_GUN_PHASE); // phased beams can't hit doors
        assert(hit_baddie == NULL);
        if (hit_door->kind == AZ_DOOR_NORMAL) {
          hit_door->is_open = true;
        }
      }
      // If we hit a baddie, damage it.
      if (hit_baddie != NULL) {
        assert(minor != AZ_GUN_PIERCE); // pierced baddies are dealt with above
        assert(hit_door == NULL);
        az_damage_baddie(state, hit_baddie, damage);
        // TODO: if FREEZE beam, freeze the baddie
      }
      // If this is a BURST beam, the next beam reflects off of the impact
      // point.
      if (minor == AZ_GUN_BURST) {
        const double normal_theta = az_vtheta(normal);
        beam_start = az_vadd(beam_end, az_vpolar(0.1, normal_theta));
        beam_init_angle = az_mod2pi(2.0 * normal_theta - beam_angle + AZ_PI);
      }
    }
    // If a BURST beam doesn't hit anything, it doesn't reflect (so there is no
    // additional beam).
    else if (minor == AZ_GUN_BURST) break;
  }
}

static void fire_weapons(az_space_state_t *state, double time) {
  az_ship_t *ship = &state->ship;
  az_player_t *player = &ship->player;
  az_controls_t *controls = &ship->controls;
  assert(player->gun2 != AZ_GUN_CHARGE);
  if (player->gun1 == AZ_GUN_CHARGE) {
    if (controls->fire_held) {
      ship->gun_charge = fmin(1.0, ship->gun_charge +
                              AZ_CHARGE_GUN_CHARGING_TIME * time);
    }
  } else {
    ship->gun_charge = 0.0;
  }
  if (!(controls->fire_pressed || controls->fire_held ||
        ship->gun_charge > 0.0)) return;
  const az_vector_t gun_position =
    az_vadd(ship->position, az_vpolar(18, ship->angle));
  az_projectile_t *proj = NULL;

  // If the ordnance key is held, we should be firing ordnance.
  if (controls->fire_pressed && controls->ordn_held) {
    controls->fire_pressed = false;
    switch (player->ordnance) {
      case AZ_ORDN_NONE: return;
      case AZ_ORDN_ROCKETS:
        if (player->rockets <= 0) return;
        if (az_insert_projectile(state, &proj)) {
          az_init_projectile(proj, AZ_PROJ_ROCKET, false, gun_position,
                             ship->angle);
          --player->rockets;
        }
        return;
      case AZ_ORDN_BOMBS:
        if (player->bombs <= 0) return;
        if (az_insert_projectile(state, &proj)) {
          az_init_projectile(proj, AZ_PROJ_BOMB, false, gun_position,
                             ship->angle);
          --player->bombs;
        }
        return;
    }
    AZ_ASSERT_UNREACHABLE();
  }

  // If we're not firing ordnance, we should be firing our gun.  If the gun is
  // fully charged and the player releases the fire key, fire a charged shot.
  if (ship->gun_charge > 0.0 && !controls->fire_held) {
    assert(player->gun1 == AZ_GUN_CHARGE);
    if (ship->gun_charge >= 1.0) {
      switch (player->gun2) {
        case AZ_GUN_NONE:
          fire_gun_single(state, 0.0, AZ_PROJ_GUN_CHARGED_NORMAL);
          break;
        case AZ_GUN_CHARGE: AZ_ASSERT_UNREACHABLE();
        case AZ_GUN_FREEZE: break; // TODO
        case AZ_GUN_TRIPLE:
          fire_gun_multi(state, 0.0, AZ_PROJ_GUN_CHARGED_TRIPLE,
                         3, AZ_DEG2RAD(10), 0);
          break;
        case AZ_GUN_HOMING:
          fire_gun_multi(state, 0.0, AZ_PROJ_GUN_CHARGED_HOMING,
                         4, AZ_DEG2RAD(90), AZ_DEG2RAD(45));
          break;
        case AZ_GUN_PHASE: break; // TODO
        case AZ_GUN_BURST:
          fire_gun_multi(state, 0.0, AZ_PROJ_GUN_BURST, 9, AZ_DEG2RAD(5), 0);
          break;
        case AZ_GUN_PIERCE: break; // TODO
        case AZ_GUN_BEAM: break; // TODO
      }
    }
    ship->gun_charge = 0.0;
    return;
  }

  // If we're not firing a charged shot, just fire a normal shot.
  az_gun_t minor_gun = player->gun1, major_gun = player->gun2;
  if (minor_gun == AZ_GUN_CHARGE) minor_gun = AZ_GUN_NONE;
  if (major_gun == AZ_GUN_NONE) {
    major_gun = minor_gun;
    minor_gun = AZ_GUN_NONE;
  }
  assert(minor_gun < major_gun ||
         (minor_gun == AZ_GUN_NONE && major_gun == AZ_GUN_NONE));
  if (major_gun == AZ_GUN_BEAM) {
    if (!controls->fire_held) return;
  } else if (!controls->fire_pressed) return;
  controls->fire_pressed = false;
  switch (major_gun) {
    case AZ_GUN_NONE:
      assert(minor_gun == AZ_GUN_NONE);
      fire_gun_single(state, 1.0, AZ_PROJ_GUN_NORMAL);
      return;
    case AZ_GUN_CHARGE: AZ_ASSERT_UNREACHABLE();
    case AZ_GUN_FREEZE:
      assert(minor_gun == AZ_GUN_NONE);
      fire_gun_single(state, 1.5, AZ_PROJ_GUN_FREEZE);
      return;
    case AZ_GUN_TRIPLE:
      switch (minor_gun) {
        case AZ_GUN_NONE:
          fire_gun_multi(state, 2.0, AZ_PROJ_GUN_TRIPLE, 3, AZ_DEG2RAD(10), 0);
          return;
        case AZ_GUN_FREEZE:
          fire_gun_multi(state, 3.0, AZ_PROJ_GUN_FREEZE_TRIPLE,
                         3, AZ_DEG2RAD(10), 0);
          return;
        default: AZ_ASSERT_UNREACHABLE();
      }
    case AZ_GUN_HOMING:
      switch (minor_gun) {
        case AZ_GUN_NONE:
          fire_gun_single(state, 2.0, AZ_PROJ_GUN_HOMING);
          return;
        case AZ_GUN_FREEZE:
          fire_gun_single(state, 3.0, AZ_PROJ_GUN_FREEZE_HOMING);
          return;
        case AZ_GUN_TRIPLE:
          fire_gun_multi(state, 4.0, AZ_PROJ_GUN_TRIPLE_HOMING,
                         3, AZ_DEG2RAD(30), 0);
          return;
        default: AZ_ASSERT_UNREACHABLE();
      }
    case AZ_GUN_PHASE: return; // TODO
    case AZ_GUN_BURST:
      switch (minor_gun) {
        case AZ_GUN_NONE:
          fire_gun_single(state, 2.5, AZ_PROJ_GUN_BURST);
          return;
        case AZ_GUN_FREEZE: return; // TODO
        case AZ_GUN_TRIPLE:
          fire_gun_multi(state, 5.0, AZ_PROJ_GUN_TRIPLE_BURST,
                         3, AZ_DEG2RAD(1), 0);
          return;
        case AZ_GUN_HOMING:
          fire_gun_single(state, 5.0, AZ_PROJ_GUN_HOMING_BURST);
          return;
        case AZ_GUN_PHASE: return; // TODO
        default: AZ_ASSERT_UNREACHABLE();
      }
    case AZ_GUN_PIERCE:
      switch (minor_gun) {
        case AZ_GUN_NONE:
          fire_gun_single(state, 3.0, AZ_PROJ_GUN_PIERCE);
          return;
        case AZ_GUN_FREEZE:
          fire_gun_single(state, 4.5, AZ_PROJ_GUN_FREEZE_PIERCE);
          return;
        case AZ_GUN_TRIPLE:
          fire_gun_multi(state, 6.0, AZ_PROJ_GUN_TRIPLE_PIERCE,
                         3, AZ_DEG2RAD(10), 0);
          return;
        case AZ_GUN_HOMING:
          fire_gun_single(state, 6.0, AZ_PROJ_GUN_HOMING_PIERCE);
          return;
        case AZ_GUN_PHASE: return; // TODO
        case AZ_GUN_BURST:
          fire_gun_single(state, 7.0, AZ_PROJ_GUN_BURST_PIERCE);
          return;
        default: AZ_ASSERT_UNREACHABLE();
      }
    case AZ_GUN_BEAM:
      fire_beam(state, minor_gun, time);
      return;
  }
  AZ_ASSERT_UNREACHABLE();
}

/*===========================================================================*/

void az_tick_ship(az_space_state_t *state, double time) {
  if (state->mode == AZ_MODE_GAME_OVER) return;
  az_ship_t *ship = &state->ship;
  az_player_t *player = &ship->player;
  az_controls_t *controls = &ship->controls;
  const bool has_lateral = az_has_upgrade(player, AZ_UPG_LATERAL_THRUSTERS);
  const double impulse = AZ_SHIP_BASE_THRUST_ACCEL * time;

  if (!(controls->fire_held &&
        (player->gun1 == AZ_GUN_BEAM || player->gun2 == AZ_GUN_BEAM))) {
    recharge_ship_energy(player, time);
  }

  // Deactivate tractor beam if necessary, otherwise get the node it is locked
  // onto.
  az_node_t *tractor_node = NULL;
  if (ship->tractor_beam.active) {
    if (!controls->util_held ||
        !az_lookup_node(state, ship->tractor_beam.node_uid, &tractor_node)) {
      ship->tractor_beam.active = false;
    }
  }

  // Apply velocity.  If the tractor beam is active, that implies angular
  // motion (around the locked-onto node); otherwise, linear motion.
  if (ship->tractor_beam.active) {
    // TODO: check for wall/baddie impacts
    assert(tractor_node != NULL);
    assert(tractor_node->kind == AZ_NODE_TRACTOR);
    const az_vector_t delta = az_vsub(ship->position, tractor_node->position);
    assert(az_dapprox(0.0, az_vdot(ship->velocity, delta)));
    assert(az_dapprox(ship->tractor_beam.distance, az_vnorm(delta)));
    const double invdist = 1.0 / ship->tractor_beam.distance;
    const double dtheta =
      time * invdist *
      az_vdot(ship->velocity, az_vrot90ccw(az_vmul(delta, invdist)));
    ship->position = az_vadd(az_vrotate(delta, dtheta),
                             tractor_node->position);
    ship->velocity = az_vrotate(ship->velocity, dtheta);
    ship->angle = az_mod2pi(ship->angle + dtheta);
  } else {
    assert(tractor_node == NULL);

    // Figure out what, if anything, the ship hits:
    const az_vector_t start = ship->position;
    az_vector_t delta = az_vmul(ship->velocity, time);
    az_collide_target_t collide = {.type = AZ_COL_NOTHING};
    az_vector_t impact_pos = AZ_VZERO;
    // Baddies:
    AZ_ARRAY_LOOP(baddie, state->baddies) {
      if (baddie->kind == AZ_BAD_NOTHING) continue;
      az_vector_t end_pos;
      if (az_circle_hits_baddie(baddie, AZ_SHIP_DEFLECTOR_RADIUS, start, delta,
                                &end_pos, &impact_pos)) {
        collide.type = AZ_COL_BADDIE;
        collide.target.baddie = baddie;
        delta = az_vsub(end_pos, start);
      }
    }
    // Doors:
    AZ_ARRAY_LOOP(door, state->doors) {
      if (door->kind == AZ_DOOR_NOTHING) continue;
      az_vector_t end_pos;
      if (az_ray_enters_door(door, start, delta, &end_pos)) {
        collide.type = AZ_COL_DOOR_INSIDE;
        collide.target.door = door;
        delta = az_vsub(end_pos, start);
      }
      if (az_circle_hits_door(door, AZ_SHIP_DEFLECTOR_RADIUS, start, delta,
                              &end_pos, &impact_pos)) {
        collide.type = AZ_COL_DOOR_OUTSIDE;
        collide.target.door = door;
        delta = az_vsub(end_pos, start);
      }
    }
    // Walls:
    AZ_ARRAY_LOOP(wall, state->walls) {
      if (wall->kind == AZ_WALL_NOTHING) continue;
      az_vector_t end_pos;
      if (az_circle_hits_wall(wall, AZ_SHIP_DEFLECTOR_RADIUS, start, delta,
                              &end_pos, &impact_pos)) {
        collide.type = AZ_COL_WALL;
        collide.target.wall = wall;
        delta = az_vsub(end_pos, start);
      }
    }

    // Move the ship:
    ship->position = az_vadd(start, delta);

    // Resolve the collision (if any):
    switch (collide.type) {
      case AZ_COL_NOTHING: break;
      case AZ_COL_BADDIE:
        // TODO: make these parameters depend on baddie
        on_ship_hit_wall(state, 0.1, 10.0, impact_pos);
        break;
      case AZ_COL_DOOR_INSIDE:
        on_ship_enter_door(state, collide.target.door);
        break;
      case AZ_COL_DOOR_OUTSIDE:
        on_ship_hit_wall(state, 0.5, 0.0, impact_pos);
        break;
      case AZ_COL_WALL:
        on_ship_hit_wall(state, collide.target.wall->data->elasticity,
                         collide.target.wall->data->impact_damage_coeff,
                         impact_pos);
        break;
    }
  }

  // If we press the util key while near a save point, initiate saving.
  if (controls->util_pressed) {
    controls->util_pressed = false;
    const az_node_t *node = choose_nearby_node(state);
    // TODO: Handle other kinds of node actions (e.g. refill/comm nodes)
    if (node != NULL && node->kind == AZ_NODE_SAVE_POINT) {
      state->mode = AZ_MODE_SAVING;
    }
  }

  // Check if we hit an upgrade.
  if (state->mode == AZ_MODE_NORMAL) {
    AZ_ARRAY_LOOP(node, state->nodes) {
      if (node->kind != AZ_NODE_UPGRADE) continue;
      if (az_vwithin(ship->position, node->position,
                     AZ_UPGRADE_COLLECTION_RADIUS)) {
        state->mode = AZ_MODE_UPGRADE;
        state->mode_data.upgrade.step = AZ_UGS_OPEN;
        state->mode_data.upgrade.progress = 0.0;
        state->mode_data.upgrade.upgrade = node->upgrade;
        node->kind = AZ_NODE_NOTHING;
        az_give_upgrade(player, node->upgrade);
        break;
      }
    }
  }

  // By now it's possible we've changed modes (e.g. we may have gone through a
  // door, or been destroyed by a wall impact).  We should only continue
  // onwards if we're still in normal mode.
  if (state->mode != AZ_MODE_NORMAL) return;

  apply_gravity_to_ship(ship, time);

  // Turning left:
  if (controls->left && !controls->right) {
    if (!controls->burn) {
      ship->angle = az_mod2pi(ship->angle + AZ_SHIP_TURN_RATE * time);
    } else if (has_lateral) {
      ship->velocity = az_vadd(ship->velocity,
                               az_vpolar(impulse / 2,
                                         ship->angle - AZ_HALF_PI));
    }
  }

  // Turning right:
  if (controls->right && !controls->left) {
    if (!controls->burn) {
      ship->angle = az_mod2pi(ship->angle - AZ_SHIP_TURN_RATE * time);
    } else if (has_lateral) {
      ship->velocity = az_vadd(ship->velocity,
                               az_vpolar(impulse / 2,
                                         ship->angle + AZ_HALF_PI));
    }
  }

  // Forward thrust:
  if (controls->up && !controls->down) {
    if (!controls->burn) {
      ship->velocity = az_vadd(ship->velocity,
                               az_vpolar(impulse, ship->angle));
    } else if (has_lateral) {
      ship->velocity = az_vadd(ship->velocity,
                               az_vpolar(-impulse/2, ship->angle));
    }
  }

  // Retro thrusters:
  if (controls->down && !controls->up &&
      az_has_upgrade(player, AZ_UPG_RETRO_THRUSTERS)) {
    const double speed = az_vnorm(ship->velocity);
    if (speed <= impulse) {
      ship->velocity = AZ_VZERO;
    } else {
      ship->velocity = az_vmul(ship->velocity, (speed - impulse) / speed);
    }
  }

  apply_drag_to_ship(ship, time);

  // Activate tractor beam if necessary:
  if (controls->util_held && controls->fire_pressed &&
      !ship->tractor_beam.active) {
    assert(tractor_node == NULL);
    double best_distance = AZ_TRACTOR_BEAM_MAX_RANGE;
    AZ_ARRAY_LOOP(node, state->nodes) {
      if (node->kind == AZ_NODE_TRACTOR) {
        const double dist = az_vnorm(az_vsub(node->position, ship->position));
        if (dist <= best_distance) {
          tractor_node = node;
          best_distance = dist;
        }
      }
    }
    if (tractor_node != NULL) {
      ship->tractor_beam.active = true;
      ship->tractor_beam.node_uid = tractor_node->uid;
      ship->tractor_beam.distance = best_distance;
      controls->fire_pressed = false;
    }
  }
  // Apply tractor beam's velocity changes:
  if (ship->tractor_beam.active) {
    assert(tractor_node != NULL);
    assert(tractor_node->kind == AZ_NODE_TRACTOR);
    ship->velocity = az_vflatten(ship->velocity,
        az_vsub(ship->position, tractor_node->position));
  }

  fire_weapons(state, time);
}

/*===========================================================================*/
