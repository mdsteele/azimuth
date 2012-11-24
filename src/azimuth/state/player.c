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

/*===========================================================================*/

const char *az_gun_name(az_gun_t gun) {
  switch (gun) {
    case AZ_GUN_NONE:
      assert(false);
      break;
    case AZ_GUN_CHARGE: return "CHARGE";
    case AZ_GUN_FREEZE: return "FREEZE";
    case AZ_GUN_TRIPLE: return "TRIPLE";
    case AZ_GUN_HOMING: return "HOMING";
    case AZ_GUN_BEAM:   return "BEAM";
    case AZ_GUN_PHASE:  return "PHASE";
    case AZ_GUN_BURST:  return "BURST";
    case AZ_GUN_PIERCE: return "PIERCE";
  }
  return "XXXXXX";
}

/*===========================================================================*/

void az_init_player(az_player_t *player) {
  memset(player, 0, sizeof(*player));
  player->shields = player->max_shields = AZ_INITIAL_MAX_SHIELDS;
  player->energy = player->max_energy = AZ_INITIAL_MAX_ENERGY;
}

/*===========================================================================*/

// Check whether the player has a particular upgrade yet.
bool az_has_upgrade(const az_player_t *player, az_upgrade_t upgrade) {
  const unsigned int index = (unsigned int)upgrade;
  assert(index < AZ_NUM_UPGRADES);
  if (index < 64u) {
    return 0 != (player->upgrades1 & (UINT64_C(1) << index));
  } else {
    return 0 != (player->upgrades2 & (UINT64_C(1) << (index - 64u)));
  }
}

// Give an upgrade to the player.
void az_give_upgrade(az_player_t *player, az_upgrade_t upgrade) {
  // If the player already has this upgrade, do nothing.
  if (az_has_upgrade(player, upgrade)) return;

  // For capacity upgrades, increase max capacity.
  if (upgrade >= AZ_UPG_ROCKET_AMMO_00 && upgrade <= AZ_UPG_ROCKET_AMMO_19) {
    assert(player->rockets <= player->max_rockets);
    player->max_rockets += AZ_ROCKETS_PER_AMMO_UPGRADE;
    player->rockets += AZ_ROCKETS_PER_AMMO_UPGRADE;
  } else if (upgrade >= AZ_UPG_BOMB_AMMO_00 &&
             upgrade <= AZ_UPG_BOMB_AMMO_09) {
    assert(player->bombs <= player->max_bombs);
    player->max_bombs += AZ_BOMBS_PER_AMMO_UPGRADE;
    player->bombs += AZ_BOMBS_PER_AMMO_UPGRADE;
  } else if (upgrade >= AZ_UPG_CAPACITOR_00 &&
             upgrade <= AZ_UPG_CAPACITOR_05) {
    assert(player->energy <= player->max_energy);
    player->max_energy += AZ_ENERGY_PER_CAPACITOR;
  } else if (upgrade >= AZ_UPG_SHIELD_BATTERY_00 &&
             upgrade <= AZ_UPG_SHIELD_BATTERY_11) {
    assert(player->shields <= player->max_shields);
    player->max_shields += AZ_SHILEDS_PER_BATTERY;
    player->shields = player->max_shields;
  }

  // Record the upgrade as acquired.
  const unsigned int index = (unsigned int)upgrade;
  assert(index < AZ_NUM_UPGRADES);
  if (index < 64u) {
    player->upgrades1 |= (UINT64_C(1) << index);
  } else {
    player->upgrades2 |= (UINT64_C(1) << (index - 64));
  }
}

/*===========================================================================*/

bool az_test_room_visited(const az_player_t *player, az_room_key_t room) {
  const unsigned int index = (unsigned int)room;
  assert(index < AZ_MAX_NUM_ROOMS);
  if (index < 64u) {
    return (bool)(player->rooms1 & (UINT64_C(1) << index));
  } else if (index < 128u) {
    return (bool)(player->rooms2 & (UINT64_C(1) << (index - 64u)));
  } else {
    return (bool)(player->rooms3 & (UINT64_C(1) << (index - 128u)));
  }
}

void az_set_room_visited(az_player_t *player, az_room_key_t room) {
  const unsigned int index = (unsigned int)room;
  assert(index < AZ_MAX_NUM_ROOMS);
  if (index < 64u) {
    player->rooms1 |=  (UINT64_C(1) << index);
  } else if (index < 128u) {
    player->rooms2 |= (UINT64_C(1) << (index - 64u));
  } else {
    player->rooms3 |= (UINT64_C(1) << (index - 128u));
  }
}

/*===========================================================================*/
