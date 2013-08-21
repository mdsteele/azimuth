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
#include <stddef.h> // for NULL

#include "azimuth/constants.h"
#include "azimuth/state/door.h"
#include "azimuth/state/particle.h"
#include "azimuth/state/projectile.h"
#include "azimuth/state/ship.h"
#include "azimuth/state/space.h"
#include "azimuth/tick/door.h" // for az_try_open_door
#include "azimuth/tick/gravfield.h"
#include "azimuth/tick/object.h"
#include "azimuth/tick/script.h"
#include "azimuth/util/color.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/random.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/
// Constants:

// How long after using energy we are unable to recharge, in seconds:
#define AZ_RECHARGE_COOLDOWN_TIME 0.5
// The time needed to charge the CHARGE gun, in seconds:
#define AZ_CHARGE_GUN_CHARGING_TIME 0.6
// The time needed to charge up hyper/mega ordnance, in seconds:
#define AZ_ORDN_CHARGING_TIME 0.6
// How much damage triple-gun projectiles do compared to normal ones:
#define AZ_TRIPLE_DAMAGE_MULT 0.8
// How long it takes the ship's shield flare to die down, in seconds:
#define AZ_SHIP_SHIELD_FLARE_TIME 0.5
// Wall elasticity for closed doors:
#define AZ_DOOR_ELASTICITY 0.5
// The max time, in seconds, between key taps to count as a double-tap:
#define AZ_DOUBLE_TAP_TIME 0.3
// The time needed to charge up the C-plus drive, in seconds:
#define AZ_CPLUS_CHARGE_TIME 2.0
// How long the C-plus drive stays ready once charged, in seconds:
#define AZ_CPLUS_DECAY_TIME 3.5
// Energy consumed by the C-plus drive per second while active:
#define AZ_CPLUS_POWER_COST 70.0
// Damage per second taken in superheated rooms without Thermal Armor:
#define AZ_HEAT_DAMAGE_PER_SECOND 5.0

/*===========================================================================*/
// Basics:

static bool is_normal_mode(const az_space_state_t *state) {
  return (state->mode == AZ_MODE_NORMAL ||
          state->mode == AZ_MODE_BOSS_DEATH ||
          (state->mode == AZ_MODE_DOORWAY &&
           state->doorway_mode.step == AZ_DWS_FADE_IN));
}

static bool try_use_energy(az_ship_t *ship, double energy, bool exhaust) {
  if (ship->player.energy < energy) {
    if (exhaust) ship->player.energy = 0.0;
    return false;
  }
  ship->player.energy -= energy;
  assert(ship->player.energy >= 0.0);
  ship->recharge_cooldown =
    fmax(ship->recharge_cooldown, AZ_RECHARGE_COOLDOWN_TIME);
  return true;
}

static void recharge_ship_energy(az_player_t *player, double time) {
  const double recharge_rate = AZ_SHIP_BASE_RECHARGE_RATE +
    (az_has_upgrade(player, AZ_UPG_FUSION_REACTOR) ?
     AZ_FUSION_REACTOR_RECHARGE_RATE : 0.0) +
    (az_has_upgrade(player, AZ_UPG_QUANTUM_REACTOR) ?
     AZ_QUANTUM_REACTOR_RECHARGE_RATE : 0.0);
  player->energy = fmin(player->max_energy,
                        player->energy + recharge_rate * time);
}

static void apply_gravity_to_ship(az_space_state_t *state, double time,
                                  bool *is_in_water) {
  assert(is_in_water != NULL);
  az_apply_gravfields_to_ship(state, time, is_in_water);
  // Planetary gravity works as follows.  By the spherical shell theorem, the
  // force of gravity on the ship (while inside the planetoid) varies linearly
  // with the distance from the planetoid's center, so the change in velocity
  // is time * surface_gravity * vnorm(position) / planetoid_radius.  However,
  // we should then multiply this scalar by the unit vector pointing from the
  // ship towards the core, which is -position/vnorm(position).  The
  // vnorm(position) factors cancel and we end up with what we have here.
  const double bouyancy_multiplier = (*is_in_water ? -0.35 : 1.0);
  az_vpluseq(&state->ship.velocity,
      az_vmul(state->ship.position, -time * bouyancy_multiplier *
              (AZ_PLANETOID_SURFACE_GRAVITY / AZ_PLANETOID_RADIUS)));
}

static void apply_drag_to_ship(az_ship_t *ship, bool is_in_water,
                               double time) {
  const double max_speed = AZ_SHIP_BASE_MAX_SPEED *
    (az_has_upgrade(&ship->player, AZ_UPG_DYNAMIC_ARMOR) ?
     (is_in_water ? 1.0 : 1.1) : (is_in_water ? 0.4 : 1.0));
  const double drag_coeff =
    AZ_SHIP_BASE_THRUST_ACCEL / (max_speed * max_speed);
  const az_vector_t drag_force =
    az_vmul(ship->velocity, -drag_coeff * az_vnorm(ship->velocity));
  az_vpluseq(&ship->velocity, az_vmul(drag_force, time));
}

/*===========================================================================*/
// Impacts:

static void on_ship_impact(az_space_state_t *state, const az_impact_t *impact,
                           const az_node_t *tractor_node) {
  az_ship_t *ship = &state->ship;

  // If we're entering or exiting a body of water, make a splash.
  const az_vector_t delta = az_vsub(impact->position, ship->position);
  AZ_ARRAY_LOOP(gravfield, state->gravfields) {
    if (gravfield->kind != AZ_GRAV_WATER) continue;
    az_vector_t position;
    double angle;
    if (az_ray_hits_water_surface(
            gravfield, ship->position, delta, &position, &angle)) {
      az_particle_t *particle;
      if (az_insert_particle(state, &particle)) {
        particle->kind = AZ_PAR_SPLOOSH;
        particle->color = (az_color_t){167, 205, 255, 192};
        particle->position = position;
        particle->angle = angle;
        particle->velocity = AZ_VZERO;
        particle->param1 =
          4.0 * sqrt(fabs(az_vdot(ship->velocity, az_vpolar(1, angle))));
        particle->lifetime = 0.1 * sqrt(particle->param1);
      }
      az_play_sound(&state->soundboard, AZ_SND_SPLASH);
    }
  }

  // Move the ship:
  ship->position = impact->position;
  if (tractor_node != NULL) {
    assert(az_has_upgrade(&state->ship.player, AZ_UPG_TRACTOR_BEAM));
    ship->velocity = az_vrotate(ship->velocity, impact->angle);
    ship->angle = az_mod2pi(ship->angle + impact->angle);
  }

  // Resolve the collision (if any):
  double damage = 0.0, elasticity = 0.0;
  bool induce_temp_invincibility = false;
  az_vector_t rel_velocity = ship->velocity;
  switch (impact->type) {
    case AZ_IMP_NOTHING: return;
    case AZ_IMP_BADDIE:
      {
        az_baddie_t *baddie = impact->target.baddie.baddie;
        assert(baddie->kind != AZ_BAD_NOTHING);
        const az_component_data_t *component = impact->target.baddie.component;
        // If the ship is in C-plus mode, we will kill the baddie outright and
        // keep going without taking damage (unless the baddie is immune to
        // C-plus damage).
        if (ship->cplus.state == AZ_CPLUS_ACTIVE &&
            az_try_damage_baddie(state, baddie, component, AZ_DMGF_CPLUS,
                                 baddie->data->max_health)) {
          assert(baddie->kind == AZ_BAD_NOTHING);
          return;
        }
        // We didn't kill the baddie yet, so we're going to bounce off of it
        // and possibly take damage.
        elasticity = 0.5;
        rel_velocity = az_vsub(ship->velocity, baddie->velocity);
        if (baddie->frozen == 0.0) {
          ship->tractor_beam.active = false;
          damage = component->impact_damage;
          if (damage > 0.0) induce_temp_invincibility = true;
        }
        // If we have Reactive Armor, and if the baddie is dealing damage to
        // us, deal damage back to the baddie.
        if (damage > 0.0 &&
            az_has_upgrade(&ship->player, AZ_UPG_REACTIVE_ARMOR) &&
            az_try_damage_baddie(state, baddie, component, AZ_DMGF_REACTIVE,
                                 0.5 * damage)) {
          ship->reactive_flare = 1.0;
          az_particle_t *particle;
          if (az_insert_particle(state, &particle)) {
            particle->kind = AZ_PAR_BOOM;
            particle->color = (az_color_t){255, 0, 0, 255};
            particle->position = az_vsub(ship->position, impact->normal);
            particle->velocity = AZ_VZERO;
            particle->lifetime = 0.4;
            particle->param1 = 15.0;
          }
        }
        // If the baddie hasn't yet been killed, but it's a KAMIKAZE baddie,
        // kill it now.
        if (baddie->kind != AZ_BAD_NOTHING &&
            (baddie->data->properties & AZ_BADF_KAMIKAZE)) {
          az_kill_baddie(state, baddie);
        }
      }
      break;
    case AZ_IMP_DOOR_INSIDE:
      assert(impact->target.door->kind != AZ_DOOR_NOTHING);
      ship->tractor_beam.active = false;
      state->mode = AZ_MODE_DOORWAY;
      state->doorway_mode = (az_doorway_mode_data_t){
        .step = AZ_DWS_FADE_OUT, .progress = 0.0,
        .destination = impact->target.door->destination,
        .entrance = {
          .kind = impact->target.door->kind,
          .uid = impact->target.door->uid,
          .position = impact->target.door->position,
          .angle = impact->target.door->angle
        }
      };
      return;
    case AZ_IMP_DOOR_OUTSIDE:
      elasticity = AZ_DOOR_ELASTICITY;
      break;
    case AZ_IMP_SHIP: AZ_ASSERT_UNREACHABLE();
    case AZ_IMP_WALL:
      assert(impact->target.wall->kind != AZ_WALL_NOTHING);
      // If C-plus is active, maybe we can just bust right through the wall.
      // Otherwise, the ship hits the wall and bounces off.
      if (ship->cplus.state == AZ_CPLUS_ACTIVE &&
          az_try_break_wall(state, impact->target.wall, AZ_DMGF_CPLUS,
                            ship->position)) {
        return;
      } else {
        elasticity = impact->target.wall->data->elasticity;
        damage = (ship->cplus.state == AZ_CPLUS_ACTIVE ? 0.0 :
                  impact->target.wall->data->impact_damage_coeff *
                  az_vnorm(ship->velocity) / AZ_SHIP_BASE_MAX_SPEED);
        if (az_has_upgrade(&ship->player, AZ_UPG_HARDENED_ARMOR)) {
          damage *= AZ_HARDENED_ARMOR_WALL_DAMAGE_FACTOR;
        }
      }
      break;
  }

  // Update C-plus drive status.
  if (state->ship.cplus.state == AZ_CPLUS_ACTIVE) {
    induce_temp_invincibility = true;
    state->ship.cplus.state = AZ_CPLUS_INACTIVE;
    rel_velocity = az_vsub(rel_velocity, ship->velocity);
    ship->velocity = az_vmul(ship->velocity, 0.3);
    az_vpluseq(&rel_velocity, ship->velocity);
    az_reset_sound(&state->soundboard, AZ_SND_CPLUS_ACTIVE);
    az_play_sound(&state->soundboard, AZ_SND_CPLUS_IMPACT);
  }

  // Push the ship slightly away from the impact point (so that we're
  // hopefully no longer in contact with whatever we hit).
  if (tractor_node == NULL) {
    az_vpluseq(&ship->position, az_vwithlen(impact->normal, 0.5));
  } else {
    const double dtheta = -copysign(0.0001, impact->angle);
    ship->position =
      az_vadd(tractor_node->position,
              az_vrotate(az_vsub(ship->position, tractor_node->position),
                         dtheta));
    ship->velocity = az_vrotate(ship->velocity, dtheta);
    ship->angle = az_mod2pi(ship->angle + dtheta);
  }

  // Bounce the ship off the object.
  if (tractor_node == NULL) {
    ship->velocity =
      az_vsub(ship->velocity, az_vmul(az_vproj(rel_velocity, impact->normal),
                                      1.0 + elasticity));
  } else {
    ship->velocity = az_vmul(ship->velocity, -elasticity);
  }

  // Put a particle at the impact point and play a sound.
  az_particle_t *particle;
  if (az_insert_particle(state, &particle)) {
    particle->kind = AZ_PAR_BOOM;
    particle->color = (az_color_t){255, 255, 255, 255};
    particle->position = az_vsub(ship->position, impact->normal);
    particle->velocity = AZ_VZERO;
    particle->lifetime = 0.3;
    particle->param1 = 10;
  }
  az_play_sound(&state->soundboard, AZ_SND_HIT_WALL);

  // Damage the ship.
  az_damage_ship(state, damage, induce_temp_invincibility);
}

/*===========================================================================*/
// Weapons:

// Calculate the base power multiplier for gun projectiles.  Each gun upgrade
// that the player has collected boosts power by 5.5% for all guns (but not for
// ordnance).
static double gun_power_mult(const az_player_t *player) {
  double power = 1.0;
  for (az_upgrade_t upg = AZ_UPG_GUN_CHARGE; upg <= AZ_UPG_GUN_BEAM; ++upg) {
    if (az_has_upgrade(player, upg)) power *= 1.055;
  }
  return power;
}

// Calculate the base power multiplier for ordnance.
static double ordn_power_mult(const az_player_t *player) {
  return (az_has_upgrade(player, AZ_UPG_HIGH_EXPLOSIVES) ?
          AZ_HIGH_EXPLOSIVES_POWER_MULTIPLIER : 1.0);
}

static void fire_projectiles(
    az_space_state_t *state, az_proj_kind_t kind, double power, bool forward,
    int num_shots, double dtheta, double theta_offset, bool alt_params,
    az_sound_key_t sound) {
  const az_ship_t *ship = &state->ship;
  const az_vector_t gun_position =
    az_vadd(ship->position, az_vpolar((forward ? 18 : -12), ship->angle));
  double theta = 0.0;
  for (int i = 0; i < num_shots; ++i) {
    az_projectile_t *proj = az_add_projectile(
        state, kind, false, gun_position,
        ship->angle + theta_offset + theta, power);
    if (proj == NULL) break;
    if (alt_params) proj->param = (i % 2 ? 1 : -1);
    theta = -theta;
    if (i % 2 == 0) theta += dtheta;
  }
  az_play_sound(&state->soundboard, sound);
}

// Fire multiple projectiles from a gun weapon (applying the gun power bonus).
static void fire_gun_multi(
    az_space_state_t *state, double energy_mult, az_proj_kind_t kind,
    double base_power, int num_shots, double dtheta, double theta_offset,
    az_sound_key_t sound) {
  az_ship_t *ship = &state->ship;
  // Consume energy for this shot (or return early if we can't afford it).
  if (energy_mult > 0.0) {
    const double energy_cost = 5.0 * energy_mult;
    if (!try_use_energy(ship, energy_cost, false)) return;
  }
  fire_projectiles(state, kind, base_power * gun_power_mult(&ship->player),
                   true, num_shots, dtheta, theta_offset, false, sound);
}

// Fire a single projectile from a gun weapon (applying the gun power bonus).
static void fire_gun_single(
    az_space_state_t *state, double energy_mult, az_proj_kind_t kind,
    az_sound_key_t sound) {
  fire_gun_multi(state, energy_mult, kind, 1.0, 1, 0.0, 0.0, sound);
}

// Fire multiple ordnance projectiles (applying the ordnance power bonus).
static void fire_ordn_multi(
    az_space_state_t *state, az_proj_kind_t kind, bool forward,
    int num_shots, double dtheta, bool alt_params, az_sound_key_t sound,
    double recoil) {
  const double power = ordn_power_mult(&state->ship.player);
  fire_projectiles(state, kind, power, forward, num_shots, dtheta,
                   (forward ? 0.0 : AZ_PI), alt_params, sound);
  if (recoil != 0.0) {
    state->ship.velocity =
      az_vadd(state->ship.velocity, az_vpolar(-recoil, state->ship.angle));
  }
}

// Fire a single ordnance projectile (applying the ordnance power bonus).
static void fire_ordn_single(
    az_space_state_t *state, az_proj_kind_t kind, bool forward,
    az_sound_key_t sound, double recoil) {
  fire_ordn_multi(state, kind, forward, 1, 0.0, false, sound, recoil);
}

static void beam_emit_particles(az_space_state_t *state, az_vector_t position,
                                az_vector_t normal, az_color_t color) {
  az_add_speck(state, color, 1.0, position,
               az_vpolar(az_random(20.0, 70.0), az_vtheta(normal) +
                         az_random(-AZ_HALF_PI, AZ_HALF_PI)));
}

static void fire_beam(az_space_state_t *state, az_gun_t minor, double time) {
  az_ship_t *ship = &state->ship;
  if (!ship->controls.fire_held) return;

  // Determine how much energy the beam needs and how much damage it'll do:
  double energy_cost = AZ_BEAM_GUN_BASE_ENERGY_PER_SECOND * time;
  double damage_mult = gun_power_mult(&ship->player);
  switch (minor) {
    case AZ_GUN_NONE: break;
    case AZ_GUN_CHARGE: AZ_ASSERT_UNREACHABLE();
    case AZ_GUN_FREEZE: energy_cost *= 1.5; break;
    case AZ_GUN_TRIPLE: energy_cost *= 2.0; damage_mult *= 0.7; break;
    case AZ_GUN_HOMING: energy_cost *= 1.5; damage_mult *= 0.5; break;
    case AZ_GUN_PHASE: energy_cost *= 2.0; break;
    case AZ_GUN_BURST: energy_cost *= 2.5; break;
    case AZ_GUN_PIERCE: energy_cost *= 3.0; damage_mult *= 2.0; break;
    case AZ_GUN_BEAM: AZ_ASSERT_UNREACHABLE();
  }
  if (!try_use_energy(ship, energy_cost, true)) return;

  az_damage_flags_t damage_kind = AZ_DMGF_BEAM;
  if (minor == AZ_GUN_FREEZE) damage_kind |= AZ_DMGF_FREEZE;
  else if (minor == AZ_GUN_PIERCE) damage_kind |= AZ_DMGF_PIERCE;

  az_vector_t beam_start =
    az_vadd(ship->position, az_vpolar(18, ship->angle));
  double beam_init_angle = ship->angle;
  // Normally, there will be a single beam.  But for a TRIPLE/BEAM gun, there
  // are three beams, and for a BURST/BEAM gun, the beam reflects off of
  // whatever it hits, creating additional beams.
  const int num_beams = (minor == AZ_GUN_TRIPLE ? 3 :
                         minor == AZ_GUN_BURST ? 3 : 1);
  for (int beam_index = 0; beam_index < num_beams; ++beam_index) {
    // Determine how much damage the beam should do.  Note that for BURST
    // beams, the damage_mult will change with each loop iteration.
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
        if (baddie->data->properties & AZ_BADF_NO_HOMING_BEAM) continue;
        const az_vector_t delta = az_vsub(baddie->position, beam_start);
        const double dist = az_vnorm(delta);
        if (dist >= best_dist) continue;
        const double angle = az_vtheta(delta);
        if (fabs(az_mod2pi(angle - beam_init_angle)) <= AZ_DEG2RAD(60)) {
          best_dist = dist;
          beam_angle = angle;
        }
      }
    }

    // Determine what the beam hits (if anything).
    az_impact_t impact;
    az_impact_flags_t skip_types = AZ_IMPF_SHIP;
    if (minor == AZ_GUN_PHASE) {
      skip_types |= AZ_IMPF_WALL | AZ_IMPF_DOOR_INSIDE | AZ_IMPF_DOOR_OUTSIDE;
    } else if (minor == AZ_GUN_PIERCE) skip_types |= AZ_IMPF_BADDIE;
    az_ray_impact(state, beam_start,
                  az_vpolar(2.5 * AZ_SCREEN_RADIUS, beam_angle),
                  skip_types, AZ_SHIP_UID, &impact);

    // If this is a PHASE beam, hit all doors along the beam.
    if (minor == AZ_GUN_PHASE) {
      assert(impact.type != AZ_IMP_DOOR_OUTSIDE);
      const az_vector_t delta = az_vsub(impact.position, beam_start);
      AZ_ARRAY_LOOP(door, state->doors) {
        if (door->kind == AZ_DOOR_NOTHING) continue;
        if (az_ray_hits_door_outside(door, beam_start, delta, NULL, NULL)) {
          az_try_open_door(state, door, damage_kind);
        }
      }
    }
    // Or, if this is a PIERCE beam, hit all baddies along the beam.
    else if (minor == AZ_GUN_PIERCE) {
      assert(impact.type != AZ_IMP_BADDIE);
      const az_vector_t delta = az_vsub(impact.position, beam_start);
      AZ_ARRAY_LOOP(baddie, state->baddies) {
        if (baddie->kind == AZ_BAD_NOTHING) continue;
        if (baddie->data->properties & AZ_BADF_INCORPOREAL) continue;
        az_vector_t position, normal;
        const az_component_data_t *component;
        if (az_ray_hits_baddie(baddie, beam_start, delta,
                               &position, &normal, &component)) {
          beam_emit_particles(state, position, normal, AZ_WHITE);
          az_try_damage_baddie(state, baddie, component, damage_kind, damage);
        }
      }
    }

    // Add a particle for the beam itself:
    const uint8_t alt = 32 * az_clock_zigzag(6, 1, state->clock);
    const az_color_t beam_color = {
      (minor == AZ_GUN_FREEZE ? alt : 255),
      (minor == AZ_GUN_FREEZE ? 128 : minor == AZ_GUN_PIERCE ? 0 : alt),
      (minor == AZ_GUN_FREEZE ? 255 : minor == AZ_GUN_PHASE ? 0 : alt), 192};
    az_add_beam(state, beam_color, beam_start, impact.position, 0.0,
        sqrt(damage_mult) * (3.0 + 0.5 * az_clock_zigzag(8, 1, state->clock)));

    // Resolve hits:
    bool did_hit = true;
    bool did_damage = false;
    az_color_t hit_color = AZ_WHITE;
    switch (impact.type) {
      case AZ_IMP_NOTHING: did_hit = false; break;
      case AZ_IMP_BADDIE:
        assert(minor != AZ_GUN_PIERCE); // pierced baddies are dealt with above
        did_damage = az_try_damage_baddie(
            state, impact.target.baddie.baddie,
            impact.target.baddie.component, damage_kind, damage);
        break;
      case AZ_IMP_DOOR_INSIDE: did_hit = false; break;
      case AZ_IMP_DOOR_OUTSIDE:
        az_try_open_door(state, impact.target.door, damage_kind);
        break;
      case AZ_IMP_SHIP: AZ_ASSERT_UNREACHABLE();
      case AZ_IMP_WALL:
        hit_color = impact.target.wall->data->color1;
        break;
    }
    if (did_hit) {
      // Add particles off of whatever the beam hits:
      beam_emit_particles(state, impact.position, impact.normal, hit_color);
      // If this is a BURST beam, the next beam reflects off of the impact
      // point.  The next beam will be reduced in power.
      if (minor == AZ_GUN_BURST) {
        const double normal_theta = az_vtheta(impact.normal);
        beam_start = az_vadd(impact.position, az_vpolar(0.1, normal_theta));
        beam_init_angle = az_mod2pi(2.0 * normal_theta - beam_angle + AZ_PI);
        damage_mult *= 0.5;
      }
    }
    // If a BURST beam damages a baddie, or if it doesn't hit anything, it
    // doesn't reflect (so there is no additional beam).
    if (minor == AZ_GUN_BURST && (did_damage || !did_hit)) break;
  }

  // Play a looping sound (based on beam kind):
  az_sound_key_t sound;
  switch (minor) {
    case AZ_GUN_FREEZE: sound = AZ_SND_BEAM_FREEZE; break;
    case AZ_GUN_PHASE:  sound = AZ_SND_BEAM_PHASE;  break;
    case AZ_GUN_PIERCE: sound = AZ_SND_BEAM_PIERCE; break;
    default:            sound = AZ_SND_BEAM_NORMAL; break;
  }
  az_loop_sound(&state->soundboard, sound);
}

static void charge_weapons(az_space_state_t *state, double time) {
  az_ship_t *ship = &state->ship;
  az_player_t *player = &ship->player;
  az_controls_t *controls = &ship->controls;
  // Charge gun:
  assert(player->gun2 != AZ_GUN_CHARGE);
  if (player->gun1 == AZ_GUN_CHARGE) {
    if (controls->fire_held) {
      if (ship->gun_charge < 1.0) {
        ship->gun_charge = fmin(1.0, ship->gun_charge +
                                AZ_CHARGE_GUN_CHARGING_TIME * time);
        az_persist_sound(&state->soundboard, AZ_SND_CHARGING_GUN);
      } else {
        az_loop_sound(&state->soundboard, AZ_SND_CHARGED_GUN);
      }
    }
  } else {
    ship->gun_charge = 0.0;
    az_reset_sound(&state->soundboard, AZ_SND_CHARGING_GUN);
  }
  // Charge ordnance:
  if ((player->ordnance == AZ_ORDN_ROCKETS &&
       az_has_upgrade(player, AZ_UPG_HYPER_ROCKETS)) ||
      (player->ordnance == AZ_ORDN_BOMBS &&
       (az_has_upgrade(player, AZ_UPG_MEGA_BOMBS) ||
        az_has_upgrade(player, AZ_UPG_ORION_BOOSTER)))) {
    if (controls->ordn_held) {
      if (ship->ordn_charge < 1.0) {
        ship->ordn_charge = fmin(1.0, ship->ordn_charge +
                                 AZ_ORDN_CHARGING_TIME * time);
        az_persist_sound(&state->soundboard, AZ_SND_CHARGING_ORDNANCE);
      } else {
        az_loop_sound(&state->soundboard, AZ_SND_CHARGED_ORDNANCE);
      }
    }
  } else {
    ship->ordn_charge = 0.0;
    az_reset_sound(&state->soundboard, AZ_SND_CHARGING_ORDNANCE);
  }
}

static void fire_weapons(az_space_state_t *state, double time) {
  az_ship_t *ship = &state->ship;
  az_player_t *player = &ship->player;
  az_controls_t *controls = &ship->controls;

  // If the ordnance key is held, we should be firing ordnance.
  if (controls->fire_pressed && controls->ordn_held) {
    controls->fire_pressed = false;
    switch (player->ordnance) {
      case AZ_ORDN_NONE: return;
      case AZ_ORDN_ROCKETS:
        if (ship->ordn_charge >= 1.0 &&
            az_has_upgrade(player, AZ_UPG_HYPER_ROCKETS) &&
            player->rockets >= AZ_ROCKETS_PER_HYPER_ROCKET) {
          player->rockets -= AZ_ROCKETS_PER_HYPER_ROCKET;
          fire_ordn_single(state, AZ_PROJ_HYPER_ROCKET, true,
                           AZ_SND_FIRE_HYPER_ROCKET, 100);
        } else {
          if (player->rockets <= 0) return;
          --player->rockets;
          fire_ordn_single(state, AZ_PROJ_ROCKET, true,
                           AZ_SND_FIRE_ROCKET, 20);
        }
        ship->ordn_charge = 0.0;
        az_reset_sound(&state->soundboard, AZ_SND_CHARGING_ORDNANCE);
        return;
      case AZ_ORDN_BOMBS:
        if (ship->ordn_charge >= 1.0 &&
            az_has_upgrade(player, AZ_UPG_MEGA_BOMBS) &&
            player->bombs >= AZ_BOMBS_PER_MEGA_BOMB) {
          player->bombs -= AZ_BOMBS_PER_MEGA_BOMB;
          fire_ordn_single(state, AZ_PROJ_MEGA_BOMB, false,
                           AZ_SND_BLINK_MEGA_BOMB, 0);
        } else {
          if (player->bombs <= 0) return;
          --player->bombs;
          fire_ordn_single(state, AZ_PROJ_BOMB, false, AZ_SND_DROP_BOMB, 0);
        }
        ship->ordn_charge = 0.0;
        az_reset_sound(&state->soundboard, AZ_SND_CHARGING_ORDNANCE);
        return;
    }
    AZ_ASSERT_UNREACHABLE();
  }

  if (!controls->ordn_held || controls->fire_pressed) {
    ship->ordn_charge = 0.0;
    az_reset_sound(&state->soundboard, AZ_SND_CHARGING_ORDNANCE);
  }

  // If we're not firing ordnance, we should be firing our gun.  If the gun is
  // fully charged and the player releases the fire key, fire a charged shot.
  if (ship->gun_charge > 0.0 && !controls->fire_held) {
    assert(player->gun1 == AZ_GUN_CHARGE);
    if (ship->gun_charge >= 1.0) {
      // If the hyper rockets are also fully charged (and if the ship has
      // enough rocket ammo), fire off a gun/rocket combo.
      if (ship->ordn_charge >= 1.0 && player->gun2 != AZ_GUN_NONE &&
          player->ordnance == AZ_ORDN_ROCKETS &&
          player->rockets >= AZ_ROCKETS_PER_MISSILE_COMBO) {
        assert(az_has_upgrade(player, AZ_UPG_HYPER_ROCKETS));
        player->rockets -= AZ_ROCKETS_PER_MISSILE_COMBO;
        switch (player->gun2) {
          case AZ_GUN_NONE: AZ_ASSERT_UNREACHABLE();
          case AZ_GUN_CHARGE: AZ_ASSERT_UNREACHABLE();
          case AZ_GUN_FREEZE:
            fire_ordn_single(state, AZ_PROJ_MISSILE_FREEZE, true,
                             AZ_SND_FIRE_HYPER_ROCKET, 70);
            break;
          case AZ_GUN_TRIPLE:
            fire_ordn_single(state, AZ_PROJ_MISSILE_BARRAGE, true,
                             AZ_SND_FIRE_HYPER_ROCKET, 0);
            break;
          case AZ_GUN_HOMING:
            fire_ordn_multi(state, AZ_PROJ_MISSILE_HOMING, true, 3,
                            AZ_DEG2RAD(30), false,
                            AZ_SND_FIRE_HYPER_ROCKET, 40);
            break;
          case AZ_GUN_PHASE:
            fire_ordn_multi(state, AZ_PROJ_MISSILE_PHASE, true, 2,
                            AZ_DEG2RAD(0), true,
                            AZ_SND_FIRE_HYPER_ROCKET, 100);
            break;
          case AZ_GUN_BURST:
            fire_ordn_single(state, AZ_PROJ_MISSILE_BURST, true,
                             AZ_SND_FIRE_HYPER_ROCKET, 70);
            break;
          case AZ_GUN_PIERCE:
            fire_ordn_single(state, AZ_PROJ_MISSILE_PIERCE, true,
                             AZ_SND_FIRE_HYPER_ROCKET, 100);
            break;
          case AZ_GUN_BEAM:
            fire_ordn_single(state, AZ_PROJ_MISSILE_BEAM, true,
                             AZ_SND_CHARGED_MISSILE_BEAM, 0);
            break;
        }
        ship->ordn_charge = 0.0;
      }
      // Otherwise, fire a charged shot (no rockets).
      else {
        switch (player->gun2) {
          case AZ_GUN_NONE:
            fire_gun_single(state, 0.0, AZ_PROJ_GUN_CHARGED_NORMAL,
                            AZ_SND_FIRE_GUN_NORMAL);
            break;
          case AZ_GUN_CHARGE: AZ_ASSERT_UNREACHABLE();
          case AZ_GUN_FREEZE:
            fire_gun_single(state, 0.0, AZ_PROJ_GUN_CHARGED_FREEZE,
                            AZ_SND_FIRE_GUN_FREEZE);
            break;
          case AZ_GUN_TRIPLE:
            fire_gun_multi(state, 0.0, AZ_PROJ_GUN_CHARGED_TRIPLE, 1.0,
                           3, AZ_DEG2RAD(10), 0, AZ_SND_FIRE_GUN_NORMAL);
            break;
          case AZ_GUN_HOMING:
            fire_gun_multi(state, 0.0, AZ_PROJ_GUN_CHARGED_HOMING, 1.0,
                           4, AZ_DEG2RAD(90), AZ_DEG2RAD(45),
                           AZ_SND_FIRE_GUN_NORMAL);
            break;
          case AZ_GUN_PHASE:
            fire_gun_single(state, 0.0, AZ_PROJ_GUN_CHARGED_PHASE,
                            AZ_SND_FIRE_GUN_NORMAL);
            break;
          case AZ_GUN_BURST:
            fire_gun_multi(state, 0.0, AZ_PROJ_GUN_BURST, 1.0, 9,
                           AZ_DEG2RAD(5), 0, AZ_SND_FIRE_GUN_NORMAL);
          break;
          case AZ_GUN_PIERCE:
            fire_gun_single(state, 0.0, AZ_PROJ_GUN_CHARGED_PIERCE,
                            AZ_SND_FIRE_GUN_PIERCE);
            break;
          case AZ_GUN_BEAM:
            fire_gun_single(state, 0.0, AZ_PROJ_GUN_CHARGED_BEAM,
                            AZ_SND_FIRE_GUN_CHARGED_BEAM);
            break;
        }
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
      fire_gun_single(state, 1.0, AZ_PROJ_GUN_NORMAL, AZ_SND_FIRE_GUN_NORMAL);
      return;
    case AZ_GUN_CHARGE: AZ_ASSERT_UNREACHABLE();
    case AZ_GUN_FREEZE:
      assert(minor_gun == AZ_GUN_NONE);
      fire_gun_single(state, 1.5, AZ_PROJ_GUN_FREEZE, AZ_SND_FIRE_GUN_FREEZE);
      return;
    case AZ_GUN_TRIPLE:
      switch (minor_gun) {
        case AZ_GUN_NONE:
          fire_gun_multi(state, 2.0, AZ_PROJ_GUN_NORMAL, AZ_TRIPLE_DAMAGE_MULT,
                         3, AZ_DEG2RAD(10), 0, AZ_SND_FIRE_GUN_NORMAL);
          return;
        case AZ_GUN_FREEZE:
          fire_gun_multi(state, 3.0, AZ_PROJ_GUN_FREEZE, AZ_TRIPLE_DAMAGE_MULT,
                         3, AZ_DEG2RAD(10), 0, AZ_SND_FIRE_GUN_FREEZE);
          return;
        default: AZ_ASSERT_UNREACHABLE();
      }
    case AZ_GUN_HOMING:
      switch (minor_gun) {
        case AZ_GUN_NONE:
          fire_gun_single(state, 1.5, AZ_PROJ_GUN_HOMING,
                          AZ_SND_FIRE_GUN_NORMAL);
          return;
        case AZ_GUN_FREEZE:
          fire_gun_single(state, 2.25, AZ_PROJ_GUN_FREEZE_HOMING,
                          AZ_SND_FIRE_GUN_FREEZE);
          return;
        case AZ_GUN_TRIPLE:
          fire_gun_multi(state, 3.0, AZ_PROJ_GUN_HOMING, AZ_TRIPLE_DAMAGE_MULT,
                         3, AZ_DEG2RAD(30), 0, AZ_SND_FIRE_GUN_NORMAL);
          return;
        default: AZ_ASSERT_UNREACHABLE();
      }
    case AZ_GUN_PHASE:
      switch (minor_gun) {
        case AZ_GUN_NONE:
          fire_gun_multi(state, 2.0, AZ_PROJ_GUN_PHASE, 1.0,
                         15, AZ_DEG2RAD(1), 0, AZ_SND_FIRE_GUN_NORMAL);
          return;
        case AZ_GUN_FREEZE:
          fire_gun_multi(state, 3.0, AZ_PROJ_GUN_FREEZE_PHASE, 1.0,
                         15, AZ_DEG2RAD(1), 0, AZ_SND_FIRE_GUN_FREEZE);
          return;
        case AZ_GUN_TRIPLE:
          fire_gun_multi(state, 4.0, AZ_PROJ_GUN_PHASE, AZ_TRIPLE_DAMAGE_MULT,
                         45, AZ_DEG2RAD(1), 0, AZ_SND_FIRE_GUN_NORMAL);
          return;
        case AZ_GUN_HOMING:
          fire_gun_multi(state, 3.0, AZ_PROJ_GUN_HOMING_PHASE, 1.0,
                         15, AZ_DEG2RAD(1), 0, AZ_SND_FIRE_GUN_NORMAL);
          return;
        default: AZ_ASSERT_UNREACHABLE();
      }
    case AZ_GUN_BURST:
      switch (minor_gun) {
        case AZ_GUN_NONE:
          fire_gun_single(state, 2.5, AZ_PROJ_GUN_BURST,
                          AZ_SND_FIRE_GUN_NORMAL);
          return;
        case AZ_GUN_FREEZE:
          fire_gun_single(state, 3.75, AZ_PROJ_GUN_FREEZE_BURST,
                          AZ_SND_FIRE_GUN_NORMAL);
          return;
        case AZ_GUN_TRIPLE:
          fire_gun_multi(state, 5.0, AZ_PROJ_GUN_BURST, AZ_TRIPLE_DAMAGE_MULT,
                         3, AZ_DEG2RAD(1), 0, AZ_SND_FIRE_GUN_NORMAL);
          return;
        case AZ_GUN_HOMING:
          fire_gun_single(state, 3.75, AZ_PROJ_GUN_HOMING_BURST,
                          AZ_SND_FIRE_GUN_NORMAL);
          return;
        case AZ_GUN_PHASE:
          fire_gun_multi(state, 5.0, AZ_PROJ_GUN_PHASE_BURST, 1.0,
                         11, AZ_DEG2RAD(1), 0, AZ_SND_FIRE_GUN_NORMAL);
          return;
        default: AZ_ASSERT_UNREACHABLE();
      }
    case AZ_GUN_PIERCE:
      switch (minor_gun) {
        case AZ_GUN_NONE:
          fire_gun_single(state, 3.0, AZ_PROJ_GUN_PIERCE,
                          AZ_SND_FIRE_GUN_PIERCE);
          return;
        case AZ_GUN_FREEZE:
          fire_gun_single(state, 4.5, AZ_PROJ_GUN_FREEZE_PIERCE,
                          AZ_SND_FIRE_GUN_PIERCE);
          return;
        case AZ_GUN_TRIPLE:
          fire_gun_multi(state, 6.0, AZ_PROJ_GUN_PIERCE, AZ_TRIPLE_DAMAGE_MULT,
                         3, AZ_DEG2RAD(10), 0, AZ_SND_FIRE_GUN_PIERCE);
          return;
        case AZ_GUN_HOMING:
          fire_gun_single(state, 4.5, AZ_PROJ_GUN_HOMING_PIERCE,
                        AZ_SND_FIRE_GUN_PIERCE);
          return;
        case AZ_GUN_PHASE:
          fire_gun_multi(state, 6.0, AZ_PROJ_GUN_PHASE_PIERCE, 1.0,
                         21, AZ_DEG2RAD(1), 0, AZ_SND_FIRE_GUN_PIERCE);
          return;
        case AZ_GUN_BURST:
          fire_gun_single(state, 7.0, AZ_PROJ_GUN_BURST_PIERCE,
                          AZ_SND_FIRE_GUN_PIERCE);
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
// Engines:

static void apply_cplus_drive(az_space_state_t *state, bool is_in_water,
                              double time) {
  az_ship_t *ship = &state->ship;
  az_player_t *player = &ship->player;
  az_controls_t *controls = &ship->controls;

  // If we don't have the C-plus drive upgrade yet, do nothing.
  if (!az_has_upgrade(player, AZ_UPG_CPLUS_DRIVE)) {
    assert(ship->cplus.state == AZ_CPLUS_INACTIVE);
    assert(ship->cplus.charge == 0.0);
    assert(ship->cplus.tap_time == 0.0);
    return;
  }

  // We can't use the C-plus drive in water without dynamic armor.
  if (is_in_water && !az_has_upgrade(player, AZ_UPG_DYNAMIC_ARMOR)) {
    ship->cplus.state = AZ_CPLUS_INACTIVE;
    ship->cplus.charge = 0.0;
    ship->cplus.tap_time = 0.0;
    az_reset_sound(&state->soundboard, AZ_SND_CPLUS_ACTIVE);
    return;
  }

  switch (ship->cplus.state) {
    case AZ_CPLUS_INACTIVE:
      assert(ship->cplus.charge >= 0.0);
      assert(ship->cplus.charge <= 1.0);
      assert(ship->cplus.tap_time == 0.0);
      // If we stop thrusting, or if we're not going forward fast enough,
      // reset the charge on the C-plus drive.
      static const double min_cplus_charge_speed = 300.0; // pixels/second
      if (controls->down_held || !controls->up_held ||
          az_vdot(ship->velocity, az_vpolar(1, ship->angle)) <
          min_cplus_charge_speed) {
        if (ship->cplus.charge == 1.0) {
          ship->cplus.state = AZ_CPLUS_READY;
        } else ship->cplus.charge = 0.0;
        break;
      } else {
        // Otherwise, increase the charge on the C-plus drive.
        const double old_charge = ship->cplus.charge;
        ship->cplus.charge = fmin(1.0, ship->cplus.charge +
                                  time / AZ_CPLUS_CHARGE_TIME);
        // Play a sound when we are fully charged.
        if (ship->cplus.charge >= 1.0 && old_charge < 1.0) {
          az_play_sound(&state->soundboard, AZ_SND_CPLUS_CHARGED);
        }
        // If we're at least halfway charged, leave a double trail of smoke
        // behind the ship (green if we're fully charged, gray otherwise).
        if (ship->cplus.charge >= 0.5) {
          az_particle_t *particle;
          for (int offset = -30; offset <= 30; offset += 60) {
            if (az_insert_particle(state, &particle)) {
              particle->kind = AZ_PAR_EMBER;
              if (ship->cplus.charge == 1.0) {
                particle->color = (az_color_t){64, 160, 64, 255};
              } else {
                particle->color = (az_color_t){128, 128, 128,
                    255 * (ship->cplus.charge - 0.25)};
              }
              particle->position =
                az_vadd(ship->position,
                        az_vpolar(-17.0, ship->angle + AZ_DEG2RAD(offset)));
              particle->velocity = AZ_VZERO;
              particle->lifetime = 0.5;
              particle->param1 = 8.0;
            }
          }
        }
      }
      break;
    case AZ_CPLUS_READY:
      assert(ship->cplus.charge > 0.0);
      assert(ship->cplus.charge <= 1.0);
      assert(ship->cplus.tap_time >= 0.0);
      assert(ship->cplus.tap_time <= AZ_DOUBLE_TAP_TIME);
      az_loop_sound(&state->soundboard, AZ_SND_CPLUS_READY);
      // Decay the charge on the C-plus drive.  If it falls to zero before we
      // activate the drive, put us back into the INACTIVE state.
      ship->cplus.charge = fmax(0.0, ship->cplus.charge -
                                time / AZ_CPLUS_DECAY_TIME);
      if (ship->cplus.charge == 0.0) {
        ship->cplus.state = AZ_CPLUS_INACTIVE;
        ship->cplus.tap_time = 0.0;
      } else {
        // If the player presses the Up key twice within AZ_DOUBLE_TAP_TIME,
        // put the C-plus drive into the ACTIVE state.
        ship->cplus.tap_time = fmax(0.0, ship->cplus.tap_time - time);
        if (controls->up_pressed) {
          if (ship->cplus.tap_time > 0.0) {
            ship->cplus.state = AZ_CPLUS_ACTIVE;
            ship->cplus.charge = 0.0;
            ship->cplus.tap_time = 0.0;
            ship->temp_invincibility = 0.0;
            ship->thrusters = AZ_THRUST_NONE;
          } else ship->cplus.tap_time = AZ_DOUBLE_TAP_TIME;
        }
      }
      break;
    case AZ_CPLUS_ACTIVE:
      assert(ship->cplus.charge == 0.0);
      assert(ship->cplus.tap_time == 0.0);
      assert(ship->temp_invincibility == 0.0);
      assert(ship->thrusters == AZ_THRUST_NONE);
      // Drain energy while the C-plus drive is active.  If we stop thrusting
      // or if we run out of energy, deactivate the C-plus drive.
      const double energy_cost = AZ_CPLUS_POWER_COST * time;
      if (controls->down_held || !controls->up_held ||
          !try_use_energy(ship, energy_cost, true)) {
        ship->cplus.state = AZ_CPLUS_INACTIVE;
        az_reset_sound(&state->soundboard, AZ_SND_CPLUS_ACTIVE);
      } else {
        // As long as the C-plus drive is active, the ship moves at a constant
        // (fast) speed, and leaves a trail of green smoke behind it.
        ship->velocity = az_vpolar(1000.0, ship->angle);
        az_particle_t *particle;
        if (az_insert_particle(state, &particle)) {
          particle->kind = AZ_PAR_EMBER;
          particle->color = (az_color_t){64, 255, 64, 255};
          particle->position =
            az_vadd(ship->position, az_vpolar(-15.0, ship->angle));
          particle->velocity = AZ_VZERO;
          particle->lifetime = 0.3;
          particle->param1 = 20;
        }
        az_persist_sound(&state->soundboard, AZ_SND_CPLUS_ACTIVE);
      }
      break;
  }
}

static void apply_orion_booster(az_space_state_t *state, double time) {
  az_ship_t *ship = &state->ship;
  az_player_t *player = &ship->player;
  az_controls_t *controls = &ship->controls;

  // If we don't have the orion booster upgrade yet, do nothing.
  if (!az_has_upgrade(player, AZ_UPG_ORION_BOOSTER)) {
    assert(ship->orion.tap_time == 0.0);
    return;
  }

  // If the player presses the Down key twice within AZ_DOUBLE_TAP_TIME,
  // try to activate the orion booster.
  ship->orion.tap_time = fmax(0.0, ship->orion.tap_time - time);
  if (controls->down_pressed && player->ordnance == AZ_ORDN_BOMBS) {
    if (ship->orion.tap_time > 0.0 && ship->ordn_charge >= 1.0 &&
        player->bombs >= AZ_BOMBS_PER_ORION_BOOST) {
      az_projectile_t *proj = az_add_projectile(
          state, AZ_PROJ_ORION_BOMB, false,
          az_vadd(ship->position, az_vpolar(-15, ship->angle)),
          ship->angle + AZ_PI, ordn_power_mult(player));
      if (proj != NULL) {
        az_vpluseq(&proj->velocity, ship->velocity);
        player->bombs -= AZ_BOMBS_PER_ORION_BOOST;
        ship->ordn_charge = 0.0;
        ship->orion.tap_time = 0.0;
        az_play_sound(&state->soundboard, AZ_SND_ORION_BOOSTER);
      }
    } else ship->orion.tap_time = AZ_DOUBLE_TAP_TIME;
  }
}

static void apply_ship_thrusters(az_ship_t *ship, bool is_in_water,
                                 double time) {
  const az_player_t *player = &ship->player;
  const az_controls_t *controls = &ship->controls;
  const bool dynamic = az_has_upgrade(player, AZ_UPG_DYNAMIC_ARMOR);
  // Calculate the change in velocity imparted by the ship's thrusters.  If
  // we're in water, we scale it down to account for the added mass effect
  // (see http://en.wikipedia.org/wiki/Added_mass).  Dynamic Armor reduces this
  // effect somewhat.
  const double impulse = AZ_SHIP_BASE_THRUST_ACCEL * time *
    (!is_in_water ? 1.0 : dynamic ? 0.6 : 0.4);
  // Also, turn more slowly when we're in water (unless we have Dynamic Armor).
  const double turn_rate =
    (ship->cplus.state == AZ_CPLUS_ACTIVE ? AZ_DEG2RAD(60) :
     AZ_SHIP_TURN_RATE * (is_in_water && !dynamic ? 0.6 : 1.0));
  // Turning left:
  if (controls->left_held && !controls->right_held) {
    ship->angle = az_mod2pi(ship->angle + turn_rate * time);
  }
  // Turning right:
  if (controls->right_held && !controls->left_held) {
    ship->angle = az_mod2pi(ship->angle - turn_rate * time);
  }
  if (ship->cplus.state == AZ_CPLUS_ACTIVE) return;
  // Forward thrust:
  if (controls->up_held && !controls->down_held) {
    az_vpluseq(&ship->velocity, az_vpolar(impulse, ship->angle));
    ship->thrusters = AZ_THRUST_FORWARD;
  }
  // Retro thrusters:
  else if (controls->down_held && !controls->up_held &&
      az_has_upgrade(player, AZ_UPG_RETRO_THRUSTERS)) {
    az_vpluseq(&ship->velocity, az_vpolar(-0.8 * impulse, ship->angle));
    ship->thrusters = AZ_THRUST_REVERSE;
  } else ship->thrusters = AZ_THRUST_NONE;
}

/*===========================================================================*/
// Complete tick function:

void az_tick_ship(az_space_state_t *state, double time) {
  assert(is_normal_mode(state));
  az_ship_t *ship = &state->ship;
  assert(az_ship_is_present(ship));
  az_player_t *player = &ship->player;
  az_controls_t *controls = &ship->controls;

  // Cool down various ship systems.
  assert(ship->recharge_cooldown >= 0.0);
  ship->recharge_cooldown = fmax(0.0, ship->recharge_cooldown - time);
  assert(ship->shield_flare >= 0.0);
  assert(ship->shield_flare <= 1.0);
  ship->shield_flare =
    fmax(0.0, ship->shield_flare - time / AZ_SHIP_SHIELD_FLARE_TIME);
  assert(ship->reactive_flare >= 0.0);
  assert(ship->reactive_flare <= 1.0);
  ship->reactive_flare =
    fmax(0.0, ship->reactive_flare - time / AZ_SHIP_SHIELD_FLARE_TIME);
  assert(ship->temp_invincibility >= 0.0);
  ship->temp_invincibility = fmax(0.0, ship->temp_invincibility - time);

  // If we're in a superheated room, and we don't have Thermal Armor, take
  // damage continuously.
  if ((state->planet->rooms[player->current_room].properties &
       AZ_ROOMF_HEATED) && !az_has_upgrade(player, AZ_UPG_THERMAL_ARMOR)) {
    az_loop_sound(&state->soundboard, AZ_SND_HEAT_DAMAGE);
    az_damage_ship(state, AZ_HEAT_DAMAGE_PER_SECOND * time, false);
    if (state->mode == AZ_MODE_GAME_OVER) return;
  }

  // Recharge energy, but only if we're not currently (1) using the C-plus
  // drive, (2) waiting for energy usage to cool down, (3) trying to fire a
  // beam gun, or (4) charging the gun.
  if (ship->cplus.state != AZ_CPLUS_ACTIVE && ship->recharge_cooldown <= 0.0 &&
      !(controls->fire_held &&
        (player->gun1 == AZ_GUN_BEAM || player->gun2 == AZ_GUN_BEAM ||
         (player->gun1 == AZ_GUN_CHARGE && ship->gun_charge < 1.0)))) {
    recharge_ship_energy(player, time);
  }

  // Allow weapons to charge up, and maintain persisted sounds, as necessary.
  charge_weapons(state, time);
  if (ship->cplus.state == AZ_CPLUS_ACTIVE) {
    az_hold_sound(&state->soundboard, AZ_SND_CPLUS_ACTIVE);
  }

  // Deactivate tractor beam if necessary, otherwise get the node it is locked
  // onto.
  az_node_t *tractor_node = NULL;
  if (ship->tractor_beam.active) {
    assert(az_has_upgrade(&state->ship.player, AZ_UPG_TRACTOR_BEAM));
    if (!controls->util_held ||
        !az_lookup_node(state, ship->tractor_beam.node_uid, &tractor_node)) {
      ship->tractor_beam.active = false;
    }
  }

  // Apply tractor beam's velocity changes:
  if (ship->tractor_beam.active) {
    assert(tractor_node != NULL);
    assert(tractor_node->kind == AZ_NODE_TRACTOR);
    ship->velocity = az_vflatten(ship->velocity,
        az_vsub(ship->position, tractor_node->position));
    az_loop_sound(&state->soundboard, AZ_SND_TRACTOR_BEAM);
  } else assert(tractor_node == NULL);

  // Apply velocity.  If the tractor beam is active, that implies angular
  // motion (around the locked-onto node); otherwise, linear motion.
  assert(is_normal_mode(state));
  assert(az_ship_is_present(ship));
  az_impact_flags_t impact_flags = AZ_IMPF_SHIP;
  if (ship->temp_invincibility > 0.0) impact_flags |= AZ_IMPF_BADDIE;
  if (ship->tractor_beam.active) {
    assert(tractor_node != NULL);
    assert(tractor_node->kind == AZ_NODE_TRACTOR);

    // Calculate the angle, dtheta, that the ship will swing if not obstructed.
    const az_vector_t delta = az_vsub(ship->position, tractor_node->position);
    assert(az_dapprox(0.0, az_vdot(ship->velocity, delta)));
    assert(az_dapprox(ship->tractor_beam.distance, az_vnorm(delta)));
    const double invdist = 1.0 / ship->tractor_beam.distance;
    const double dtheta = time * invdist *
      az_vdot(ship->velocity, az_vrot90ccw(az_vmul(delta, invdist)));

    // Figure out what, if anything, the ship hits:
    az_impact_t impact;
    az_arc_circle_impact(
        state, AZ_SHIP_DEFLECTOR_RADIUS, ship->position,
        tractor_node->position, dtheta, impact_flags, AZ_SHIP_UID, &impact);
    on_ship_impact(state, &impact, tractor_node);
    if (!ship->tractor_beam.active) tractor_node = NULL;
  } else {
    assert(tractor_node == NULL);
    // Figure out what, if anything, the ship hits:
    az_impact_t impact;
    az_circle_impact(
        state, AZ_SHIP_DEFLECTOR_RADIUS, ship->position,
        az_vmul(ship->velocity, time), impact_flags, AZ_SHIP_UID, &impact);
    on_ship_impact(state, &impact, NULL);
  }
  if (!is_normal_mode(state)) return;

  // If we press the util key while near a console, use it.
  if (controls->util_pressed) {
    const az_node_t *console = NULL;
    double best_dist = AZ_CONSOLE_RANGE;
    AZ_ARRAY_LOOP(node, state->nodes) {
      if (node->kind == AZ_NODE_CONSOLE) {
        const double dist = az_vdist(node->position, state->ship.position);
        if (dist <= best_dist) {
          best_dist = dist;
          console = node;
        }
      }
    }
    if (console != NULL) {
      controls->util_pressed = false;
      ship->gun_charge = ship->ordn_charge = 0.0;
      ship->temp_invincibility = 0.0;
      ship->thrusters = AZ_THRUST_NONE;
      ship->cplus.state = AZ_CPLUS_INACTIVE;
      ship->cplus.charge = 0.0;
      ship->cplus.tap_time = 0.0;
      ship->orion.tap_time = 0.0;
      ship->tractor_beam.active = false;
      state->mode = AZ_MODE_CONSOLE;
      state->console_mode = (az_console_mode_data_t){
        .step = AZ_CSS_ALIGN, .progress = 0.0, .node_uid = console->uid,
        .position_delta = az_vsub(ship->position, console->position),
        .angle_delta = az_mod2pi(ship->angle - console->angle)
      };
      return;
    }
  }

  // Check if we hit an upgrade.
  assert(is_normal_mode(state));
  assert(az_ship_is_present(ship));
  AZ_ARRAY_LOOP(node, state->nodes) {
    if (node->kind != AZ_NODE_UPGRADE) continue;
    if (az_vwithin(ship->position, node->position,
                   AZ_UPGRADE_COLLECTION_RADIUS)) {
      const az_upgrade_t upgrade = node->subkind.upgrade;
      const az_script_t *script = node->on_use;
      node->kind = AZ_NODE_NOTHING;
      state->mode = AZ_MODE_UPGRADE;
      state->upgrade_mode = (az_upgrade_mode_data_t){
        .step = AZ_UGS_OPEN, .progress = 0.0, .upgrade = upgrade
      };
      az_give_upgrade(player, upgrade);
      az_run_script(state, script);
      return;
    }
  }

  // Apply various forces:
  bool is_in_water;
  apply_gravity_to_ship(state, time, &is_in_water);
  apply_cplus_drive(state, is_in_water, time);
  apply_orion_booster(state, time);
  apply_ship_thrusters(ship, is_in_water, time);
  apply_drag_to_ship(ship, is_in_water, time);

  // Activate tractor beam if necessary:
  if (controls->util_pressed && !ship->tractor_beam.active &&
      az_has_upgrade(&state->ship.player, AZ_UPG_TRACTOR_BEAM)) {
    assert(tractor_node == NULL);
    double best_distance = AZ_TRACTOR_BEAM_MAX_RANGE;
    AZ_ARRAY_LOOP(node, state->nodes) {
      if (node->kind == AZ_NODE_TRACTOR) {
        const double dist = az_vdist(node->position, ship->position);
        if (dist <= best_distance) {
          tractor_node = node;
          best_distance = dist;
        }
      }
    }
    if (tractor_node != NULL) {
      controls->util_pressed = false;
      ship->tractor_beam.active = true;
      ship->tractor_beam.node_uid = tractor_node->uid;
      ship->tractor_beam.distance = best_distance;
      az_run_script(state, tractor_node->on_use);
    }
  }

  fire_weapons(state, time);
}

/*===========================================================================*/
