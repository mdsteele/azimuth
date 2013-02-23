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
#ifndef AZIMUTH_STATE_UPGRADE_H_
#define AZIMUTH_STATE_UPGRADE_H_

/*===========================================================================*/

#define AZ_ROCKETS_PER_AMMO_UPGRADE 5
#define AZ_BOMBS_PER_AMMO_UPGRADE 3
#define AZ_ENERGY_PER_CAPACITOR 50
#define AZ_SHIELDS_PER_BATTERY 25

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

// Return the name of the upgrade as a NUL-terminted string.
const char *az_upgrade_name(az_upgrade_t upgrade);

/*===========================================================================*/

#endif // AZIMUTH_STATE_UPGRADE_H_
