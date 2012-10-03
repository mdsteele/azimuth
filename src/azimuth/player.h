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
#ifndef AZIMUTH_PLAYER_H_
#define AZIMUTH_PLAYER_H_

#include <stdbool.h>
#include <stdint.h>

/*===========================================================================*/

#define AZ_ROCKETS_PER_AMMO_UPGRADE 5
#define AZ_BOMBS_PER_AMMO_UPGRADE 3
#define AZ_ROCKETS_PER_PICKUP 2
#define AZ_BOMBS_PER_PICKUP 1

typedef enum {
  AZ_UPG_GUN_CHARGE = 0,
  AZ_UPG_GUN_FREEZE,
  AZ_UPG_GUN_TRIPLE,
  AZ_UPG_GUN_HOMING,
  AZ_UPG_GUN_BEAM,
  AZ_UPG_GUN_WAVE,
  AZ_UPG_GUN_BURST,
  AZ_UPG_GUN_PIERCE,
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
  AZ_UPG_ROCKET_AMMO_19
  // TODO: add more here
} az_upgrade_t;

typedef enum {
  AZ_GUN_CHARGE,
  AZ_GUN_FREEZE,
  AZ_GUN_TRIPLE,
  AZ_GUN_HOMING,
  AZ_GUN_BEAM,
  AZ_GUN_WAVE,
  AZ_GUN_BURST,
  AZ_GUN_PIERCE,
  AZ_GUN_NONE
} az_gun_t;

typedef enum {
  AZ_ORDN_ROCKETS,
  AZ_ORDN_BOMBS,
  AZ_ORDN_NONE
} az_ordnance_t;

typedef struct {
  uint64_t upgrades1, upgrades2;
  uint64_t rooms1, rooms2, rooms3;
  uint64_t flags;
  int shields, max_shields;
  int energy, max_energy;
  int rockets, max_rockets;
  int bombs, max_bombs;
  az_gun_t gun1, gun2;
  az_ordnance_t ordnance;
} az_player_t;

bool az_has_upgrade(const az_player_t *player, az_upgrade_t upgrade);
void az_give_upgrade(az_player_t *player, az_upgrade_t upgrade);

bool az_test_room(const az_player_t *player, int room);
void az_set_room(const az_player_t *player, int room);

bool az_test_flag(const az_player_t *player, int flag);
void az_set_flag(const az_player_t *player, int flag);

/*===========================================================================*/

#endif // AZIMUTH_PLAYER_H_
