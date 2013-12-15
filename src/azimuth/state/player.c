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

#include "azimuth/state/player.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h> // for memset

#include "azimuth/constants.h"
#include "azimuth/util/misc.h"

/*===========================================================================*/

const char *az_gun_name(az_gun_t gun) {
  assert(gun != AZ_GUN_NONE);
  switch (gun) {
    case AZ_GUN_NONE: AZ_ASSERT_UNREACHABLE();
    case AZ_GUN_CHARGE: return "CHARGE";
    case AZ_GUN_FREEZE: return "FREEZE";
    case AZ_GUN_TRIPLE: return "TRIPLE";
    case AZ_GUN_HOMING: return "HOMING";
    case AZ_GUN_PHASE:  return "PHASE";
    case AZ_GUN_BURST:  return "BURST";
    case AZ_GUN_PIERCE: return "PIERCE";
    case AZ_GUN_BEAM:   return "BEAM";
  }
  AZ_ASSERT_UNREACHABLE();
}

/*===========================================================================*/

void az_init_player(az_player_t *player) {
  memset(player, 0, sizeof(*player));
  player->shields = player->max_shields = AZ_INITIAL_MAX_SHIELDS;
  player->energy = player->max_energy = AZ_INITIAL_MAX_ENERGY;
}

/*===========================================================================*/

static bool test_flag(const uint64_t *array, unsigned int idx) {
  return (array[idx >> 6] & (UINT64_C(1) << (idx & 0x3f))) != 0;
}

static void set_flag(uint64_t *array, unsigned int idx) {
  array[idx >> 6] |= (UINT64_C(1) << (idx & 0x3f));
}

static void clear_flag(uint64_t *array, unsigned int idx) {
  array[idx >> 6] &= ~(UINT64_C(1) << (idx & 0x3f));
}

/*===========================================================================*/

// Check whether the player has a particular upgrade yet.
bool az_has_upgrade(const az_player_t *player, az_upgrade_t upgrade) {
  const unsigned int idx = (unsigned int)upgrade;
  assert(idx < AZ_NUM_UPGRADES);
  assert(idx < 64 * AZ_ARRAY_SIZE(player->upgrades));
  return test_flag(player->upgrades, idx);
}

// Give an upgrade to the player.
void az_give_upgrade(az_player_t *player, az_upgrade_t upgrade) {
  // If the player already has this upgrade, do nothing.
  if (az_has_upgrade(player, upgrade)) return;

  // Record the upgrade as acquired.
  const unsigned int idx = (unsigned int)upgrade;
  assert(idx < AZ_NUM_UPGRADES);
  assert(idx < 64 * AZ_ARRAY_SIZE(player->upgrades));
  set_flag(player->upgrades, idx);

  // For weapon upgrades, select the new weapon.  For capacity upgrades,
  // increase max capacity.
  if (upgrade >= AZ_UPG_GUN_CHARGE && upgrade <= AZ_UPG_GUN_BEAM) {
    az_select_gun(player, AZ_GUN_CHARGE + (upgrade - AZ_UPG_GUN_CHARGE));
  } else if (upgrade == AZ_UPG_HYPER_ROCKETS) {
    player->rockets = player->max_rockets;
    az_select_ordnance(player, AZ_ORDN_ROCKETS);
  } else if (upgrade == AZ_UPG_MEGA_BOMBS || upgrade == AZ_UPG_ORION_BOOSTER) {
    player->bombs = player->max_bombs;
    az_select_ordnance(player, AZ_ORDN_BOMBS);
  } else if (upgrade >= AZ_UPG_ROCKET_AMMO_00 &&
             upgrade <= AZ_UPG_ROCKET_AMMO_29) {
    assert(player->max_rockets >= 0);
    assert(player->rockets <= player->max_rockets);
    const bool first_rockets = (player->max_rockets == 0);
    player->max_rockets += AZ_ROCKETS_PER_AMMO_UPGRADE;
    player->rockets += AZ_ROCKETS_PER_AMMO_UPGRADE;
    if (first_rockets) az_select_ordnance(player, AZ_ORDN_ROCKETS);
  } else if (upgrade >= AZ_UPG_BOMB_AMMO_00 &&
             upgrade <= AZ_UPG_BOMB_AMMO_19) {
    assert(player->max_bombs >= 0);
    assert(player->bombs <= player->max_bombs);
    const bool first_bombs = (player->max_bombs == 0);
    player->max_bombs += AZ_BOMBS_PER_AMMO_UPGRADE;
    player->bombs += AZ_BOMBS_PER_AMMO_UPGRADE;
    if (first_bombs) az_select_ordnance(player, AZ_ORDN_BOMBS);
  } else if (upgrade >= AZ_UPG_CAPACITOR_00 &&
             upgrade <= AZ_UPG_CAPACITOR_11) {
    assert(player->energy <= player->max_energy);
    player->max_energy += AZ_ENERGY_PER_CAPACITOR;
  } else if (upgrade >= AZ_UPG_SHIELD_BATTERY_00 &&
             upgrade <= AZ_UPG_SHIELD_BATTERY_11) {
    assert(player->shields <= player->max_shields);
    player->max_shields += AZ_SHIELDS_PER_BATTERY;
    player->shields = player->max_shields;
  }
}

/*===========================================================================*/

bool az_test_room_visited(const az_player_t *player, az_room_key_t room) {
  const unsigned int idx = (unsigned int)room;
  assert(idx < AZ_MAX_NUM_ROOMS);
  assert(idx < 64 * AZ_ARRAY_SIZE(player->rooms_visited));
  return test_flag(player->rooms_visited, idx);
}

void az_set_room_visited(az_player_t *player, az_room_key_t room) {
  const unsigned int idx = (unsigned int)room;
  assert(idx < AZ_MAX_NUM_ROOMS);
  assert(idx < 64 * AZ_ARRAY_SIZE(player->rooms_visited));
  set_flag(player->rooms_visited, idx);
}

/*===========================================================================*/

bool az_test_zone_mapped(const az_player_t *player, az_zone_key_t zone) {
  const unsigned int idx = (unsigned int)zone;
  assert(idx < AZ_MAX_NUM_ZONES);
  assert(idx < 64 * AZ_ARRAY_SIZE(player->zones_mapped));
  return test_flag(player->zones_mapped, idx);
}

void az_set_zone_mapped(az_player_t *player, az_zone_key_t zone) {
  const unsigned int idx = (unsigned int)zone;
  assert(idx < AZ_MAX_NUM_ZONES);
  assert(idx < 64 * AZ_ARRAY_SIZE(player->zones_mapped));
  set_flag(player->zones_mapped, idx);
}

/*===========================================================================*/

bool az_test_flag(const az_player_t *player, az_flag_t flag) {
  const unsigned int idx = (unsigned int)flag;
  assert(idx < AZ_MAX_NUM_FLAGS);
  assert(idx < 64 * AZ_ARRAY_SIZE(player->flags));
  return test_flag(player->flags, idx);
}

void az_set_flag(az_player_t *player, az_flag_t flag) {
  const unsigned int idx = (unsigned int)flag;
  assert(idx < AZ_MAX_NUM_FLAGS);
  assert(idx < 64 * AZ_ARRAY_SIZE(player->flags));
  set_flag(player->flags, idx);
}

void az_clear_flag(az_player_t *player, az_flag_t flag) {
  const unsigned int idx = (unsigned int)flag;
  assert(idx < AZ_MAX_NUM_FLAGS);
  assert(idx < 64 * AZ_ARRAY_SIZE(player->flags));
  clear_flag(player->flags, idx);
}

/*===========================================================================*/

void az_select_gun(az_player_t *player, az_gun_t gun) {
  assert(player->gun1 != AZ_GUN_NONE ||
         (player->gun2 == AZ_GUN_NONE && !player->next_gun));
  if (gun == AZ_GUN_NONE) return;
  // Do nothing if the player doesn't yet have the requested gun.
  if (!az_has_upgrade(player, AZ_UPG_GUN_CHARGE + (gun - AZ_GUN_CHARGE))) {
    return;
  }
  // Check if the chosen gun is already selected.
  if (player->gun1 == gun) {
    // If we select CHARGE twice in a row, put us back to CHARGE/NONE.
    if (gun == AZ_GUN_CHARGE && player->next_gun) {
      player->gun2 = AZ_GUN_NONE;
    }
    player->next_gun = true;
    return;
  }
  if (player->gun2 == gun) {
    player->next_gun = false;
    return;
  }
  // Change guns.
  if (player->next_gun) player->gun2 = gun;
  else player->gun1 = gun;
  player->next_gun = !player->next_gun;
  // Reorder if necessary.
  assert(player->gun1 != player->gun2);
  if (player->gun2 < player->gun1 && player->gun2 != AZ_GUN_NONE) {
    const az_gun_t temp = player->gun1;
    player->gun1 = player->gun2;
    player->gun2 = temp;
    player->next_gun = !player->next_gun;
  }
}

void az_select_ordnance(az_player_t *player, az_ordnance_t ordn) {
  switch (ordn) {
    case AZ_ORDN_NONE: break;
    case AZ_ORDN_ROCKETS:
      if (player->max_rockets <= 0) return;
      break;
    case AZ_ORDN_BOMBS:
      if (player->max_bombs <= 0) return;
      break;
  }
  player->ordnance = ordn;
}

/*===========================================================================*/
