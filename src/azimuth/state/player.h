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

#pragma once
#ifndef AZIMUTH_STATE_PLAYER_H_
#define AZIMUTH_STATE_PLAYER_H_

#include <stdbool.h>
#include <stdint.h>

#include "azimuth/constants.h"

/*===========================================================================*/

#define AZ_ROCKETS_PER_AMMO_UPGRADE 5
#define AZ_BOMBS_PER_AMMO_UPGRADE 3
#define AZ_ENERGY_PER_CAPACITOR 50
#define AZ_SHILEDS_PER_BATTERY 25

typedef enum {
  // Primary weapon:
  AZ_UPG_GUN_CHARGE = 0,
  AZ_UPG_GUN_FREEZE,
  AZ_UPG_GUN_TRIPLE,
  AZ_UPG_GUN_HOMING,
  AZ_UPG_GUN_PHASE,
  AZ_UPG_GUN_BURST,
  AZ_UPG_GUN_PIERCE,
  AZ_UPG_GUN_BEAM,
  // Rockets:
  AZ_UPG_ROCKET_AMMO_00,
  AZ_UPG_ROCKET_AMMO_01,
  AZ_UPG_ROCKET_AMMO_02,
  AZ_UPG_ROCKET_AMMO_03,
  AZ_UPG_ROCKET_AMMO_04,
  AZ_UPG_ROCKET_AMMO_05,
  AZ_UPG_ROCKET_AMMO_06,
  AZ_UPG_ROCKET_AMMO_07,
  AZ_UPG_ROCKET_AMMO_08,
  AZ_UPG_ROCKET_AMMO_09,
  AZ_UPG_ROCKET_AMMO_10,
  AZ_UPG_ROCKET_AMMO_11,
  AZ_UPG_ROCKET_AMMO_12,
  AZ_UPG_ROCKET_AMMO_13,
  AZ_UPG_ROCKET_AMMO_14,
  AZ_UPG_ROCKET_AMMO_15,
  AZ_UPG_ROCKET_AMMO_16,
  AZ_UPG_ROCKET_AMMO_17,
  AZ_UPG_ROCKET_AMMO_18,
  AZ_UPG_ROCKET_AMMO_19,
  AZ_UPG_HYPER_ROCKETS,
  AZ_UPG_HIGH_EXPLOSIVES,
  // Bombs:
  AZ_UPG_BOMB_AMMO_00,
  AZ_UPG_BOMB_AMMO_01,
  AZ_UPG_BOMB_AMMO_02,
  AZ_UPG_BOMB_AMMO_03,
  AZ_UPG_BOMB_AMMO_04,
  AZ_UPG_BOMB_AMMO_05,
  AZ_UPG_BOMB_AMMO_06,
  AZ_UPG_BOMB_AMMO_07,
  AZ_UPG_BOMB_AMMO_08,
  AZ_UPG_BOMB_AMMO_09,
  AZ_UPG_MEGA_BOMBS,
  AZ_UPG_ATTUNED_EXPLOSIVES,
  // Misc:
  AZ_UPG_TRACTOR_BEAM,
  AZ_UPG_INFRASCANNER,
  // Engines:
  AZ_UPG_RETRO_THRUSTERS,
  AZ_UPG_LATERAL_THRUSTERS,
  AZ_UPG_CPLUS_DRIVE,
  AZ_UPG_ORION_BOOSTER,
  // Armor:
  AZ_UPG_HARDENED_ARMOR,
  AZ_UPG_THERMAL_ARMOR,
  AZ_UPG_GRAVITIC_ARMOR,
  AZ_UPG_DYNAMIC_ARMOR,
  AZ_UPG_REACTIVE_ARMOR,
  // Energy:
  AZ_UPG_CAPACITOR_00,
  AZ_UPG_CAPACITOR_01,
  AZ_UPG_CAPACITOR_02,
  AZ_UPG_CAPACITOR_03,
  AZ_UPG_CAPACITOR_04,
  AZ_UPG_CAPACITOR_05,
  AZ_UPG_FUSION_REACTOR,
  AZ_UPG_QUANTUM_REACTOR,
  // Shields:
  AZ_UPG_SHIELD_BATTERY_00,
  AZ_UPG_SHIELD_BATTERY_01,
  AZ_UPG_SHIELD_BATTERY_02,
  AZ_UPG_SHIELD_BATTERY_03,
  AZ_UPG_SHIELD_BATTERY_04,
  AZ_UPG_SHIELD_BATTERY_05,
  AZ_UPG_SHIELD_BATTERY_06,
  AZ_UPG_SHIELD_BATTERY_07,
  AZ_UPG_SHIELD_BATTERY_08,
  AZ_UPG_SHIELD_BATTERY_09,
  AZ_UPG_SHIELD_BATTERY_10,
  AZ_UPG_SHIELD_BATTERY_11
} az_upgrade_t;

// The number of different upgrade kinds:
#define AZ_NUM_UPGRADES ((int)AZ_UPG_SHIELD_BATTERY_11 + 1)

/*===========================================================================*/

// Which primary weapon module does the player have equipped?
typedef enum {
  AZ_GUN_NONE = 0,
  AZ_GUN_CHARGE,
  AZ_GUN_FREEZE,
  AZ_GUN_TRIPLE,
  AZ_GUN_HOMING,
  AZ_GUN_PHASE,
  AZ_GUN_BURST,
  AZ_GUN_PIERCE,
  AZ_GUN_BEAM
} az_gun_t;

// Get the name for a gun kind.  The argument must not be AZ_GUN_NONE.
const char *az_gun_name(az_gun_t gun);

// Which secondary (ordnance) weapon does the player have equipped?
typedef enum {
  AZ_ORDN_NONE = 0,
  AZ_ORDN_ROCKETS,
  AZ_ORDN_BOMBS
} az_ordnance_t;

// A bitset of damage kinds, made from OR-ing together the below constants.
// This type is used for checking whether or not a given baddie is immune to
// damage from a given weapon.  Note that AZ_DMGF_FREEZE is special -- it
// determines whether a baddie can be frozen, assuming it isn't immune to the
// weapon based on the other flags.
typedef uint_fast16_t az_damage_flags_t;
#define AZ_DMGF_NORMAL       ((az_damage_flags_t)(1u << 0))
#define AZ_DMGF_CHARGED      ((az_damage_flags_t)(1u << 1))
#define AZ_DMGF_FREEZE       ((az_damage_flags_t)(1u << 2))
#define AZ_DMGF_PIERCE       ((az_damage_flags_t)(1u << 3))
#define AZ_DMGF_ROCKET       ((az_damage_flags_t)(1u << 4))
#define AZ_DMGF_BOMB         ((az_damage_flags_t)(1u << 5))
#define AZ_DMGF_HYPER_ROCKET ((az_damage_flags_t)(1u << 6))
#define AZ_DMGF_MEGA_BOMB    ((az_damage_flags_t)(1u << 7))
#define AZ_DMGF_CPLUS        ((az_damage_flags_t)(1u << 8))
#define AZ_DMGF_REACTIVE     ((az_damage_flags_t)(1u << 9))
// All kinds of damage that are able to flare/destroy destructible walls:
#define AZ_DMGF_WALL_FLARE (AZ_DMGF_CHARGED | AZ_DMGF_ROCKET | \
    AZ_DMGF_HYPER_ROCKET | AZ_DMGF_BOMB | AZ_DMGF_MEGA_BOMB | AZ_DMGF_CPLUS)

/*===========================================================================*/

typedef int az_room_key_t;
typedef int az_flag_t;

// This structure stores any and all information needed to store a saved game.
typedef struct {
  // Bitmasks storing game progress:
  uint64_t upgrades1, upgrades2;
  uint64_t rooms1, rooms2, rooms3;
#if AZ_MAX_NUM_ROOMS > 64 * 3
#error "Not enough roomsN bitvectors in az_player_t."
#elif AZ_MAX_NUM_ROOMS <= 64 * 2
#error "Too many roomsN bitvectors in az_player_t."
#endif
  uint64_t flags;
  // Total game time for this playthrough so far, in seconds:
  double total_time;
  // The room we're currently in.  For save files, this indicates the room in
  // which we last saved.
  az_room_key_t current_room;
  // Current ship supplies:
  double shields, max_shields;
  double energy, max_energy;
  int rockets, max_rockets;
  int bombs, max_bombs;
  // Current weapon selections:
  az_gun_t gun1, gun2;
  bool next_gun; // false for gun1, true for gun2
  az_ordnance_t ordnance;
} az_player_t;

// Set a the player's fields as they should be for a new game (except for the
// current_room field, which is simply zeroed, and should be set to the correct
// initial room for the planet).
void az_init_player(az_player_t *player);

// Check whether the player has a particular upgrade yet.
bool az_has_upgrade(const az_player_t *player, az_upgrade_t upgrade);
// Give an upgrade to the player.
void az_give_upgrade(az_player_t *player, az_upgrade_t upgrade);

// Check whether the player has visited a particular room yet.
bool az_test_room_visited(const az_player_t *player, az_room_key_t room);
// Set the player as having visited the given room.
void az_set_room_visited(az_player_t *player, az_room_key_t room);

// Check whether a given game flag is set for the player.
bool az_test_flag(const az_player_t *player, az_flag_t flag);
// Set the given game flag for the player.
void az_set_flag(az_player_t *player, az_flag_t flag);
// Clear the given game flag for the player.
void az_clear_flag(az_player_t *player, az_flag_t flag);

// Change which gun the player has selected.  This will have no effect if the
// player hasn't yet acquired the given gun.
void az_select_gun(az_player_t *player, az_gun_t gun);
// Change which ordnance (rockets or bombs) the player has selected.  This will
// have no effect if the player hasn't yet acquired the given ordnance.
void az_select_ordnance(az_player_t *player, az_ordnance_t ordn);

/*===========================================================================*/

#endif // AZIMUTH_STATE_PLAYER_H_
