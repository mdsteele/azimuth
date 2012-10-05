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

/*===========================================================================*/

#define MAX_UPGRADES 128u

// Check whether the player has a particular upgrade yet.
bool az_has_upgrade(const az_player_t *player, az_upgrade_t upgrade) {
  const unsigned int index = (unsigned int)upgrade;
  assert(index < MAX_UPGRADES);
  if (index < 64u) {
    return 0 != (player->upgrades1 & (1u << index));
  } else {
    return 0 != (player->upgrades2 & (1u << (index - 64u)));
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
  assert(index < MAX_UPGRADES);
  if (index < 64u) {
    player->upgrades1 |= (1u << index);
  } else {
    player->upgrades2 |= (1u << (index - 64));
  }
}

/*===========================================================================*/
