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

#include "azimuth/state/upgrade.h"

#include "azimuth/util/misc.h"

/*===========================================================================*/

const char *az_upgrade_name(az_upgrade_t upgrade) {
  switch (upgrade) {
    case AZ_UPG_GUN_CHARGE: return "CHARGE GUN";
    case AZ_UPG_GUN_FREEZE: return "FREEZE GUN";
    case AZ_UPG_GUN_TRIPLE: return "TRIPLE GUN";
    case AZ_UPG_GUN_HOMING: return "HOMING GUN";
    case AZ_UPG_GUN_PHASE: return "PHASE GUN";
    case AZ_UPG_GUN_BURST: return "BURST GUN";
    case AZ_UPG_GUN_PIERCE: return "PIERCE GUN";
    case AZ_UPG_GUN_BEAM: return "BEAM GUN";
    case AZ_UPG_ROCKET_AMMO_00:
    case AZ_UPG_ROCKET_AMMO_01:
    case AZ_UPG_ROCKET_AMMO_02:
    case AZ_UPG_ROCKET_AMMO_03:
    case AZ_UPG_ROCKET_AMMO_04:
    case AZ_UPG_ROCKET_AMMO_05:
    case AZ_UPG_ROCKET_AMMO_06:
    case AZ_UPG_ROCKET_AMMO_07:
    case AZ_UPG_ROCKET_AMMO_08:
    case AZ_UPG_ROCKET_AMMO_09:
    case AZ_UPG_ROCKET_AMMO_10:
    case AZ_UPG_ROCKET_AMMO_11:
    case AZ_UPG_ROCKET_AMMO_12:
    case AZ_UPG_ROCKET_AMMO_13:
    case AZ_UPG_ROCKET_AMMO_14:
    case AZ_UPG_ROCKET_AMMO_15:
    case AZ_UPG_ROCKET_AMMO_16:
    case AZ_UPG_ROCKET_AMMO_17:
    case AZ_UPG_ROCKET_AMMO_18:
    case AZ_UPG_ROCKET_AMMO_19:
      return "ROCKETS";
    case AZ_UPG_HYPER_ROCKETS: return "HYPER ROCKETS";
    case AZ_UPG_HIGH_EXPLOSIVES: return "HIGH EXPLOSIVES";
    case AZ_UPG_BOMB_AMMO_00:
    case AZ_UPG_BOMB_AMMO_01:
    case AZ_UPG_BOMB_AMMO_02:
    case AZ_UPG_BOMB_AMMO_03:
    case AZ_UPG_BOMB_AMMO_04:
    case AZ_UPG_BOMB_AMMO_05:
    case AZ_UPG_BOMB_AMMO_06:
    case AZ_UPG_BOMB_AMMO_07:
    case AZ_UPG_BOMB_AMMO_08:
    case AZ_UPG_BOMB_AMMO_09:
      return "BOMBS";
    case AZ_UPG_MEGA_BOMBS: return "MEGA BOMBS";
    case AZ_UPG_ATTUNED_EXPLOSIVES: return "ATTUNED EXPLOSIVES";
    case AZ_UPG_TRACTOR_BEAM: return "TRACTOR BEAM";
    case AZ_UPG_INFRASCANNER: return "INFRASCANNER";
    case AZ_UPG_MAGNET_SWEEP: return "MAGNET SWEEP";
    case AZ_UPG_RETRO_THRUSTERS: return "RETRO THRUSTERS";
    case AZ_UPG_CPLUS_DRIVE: return "C-PLUS DRIVE";
    case AZ_UPG_ORION_BOOSTER: return "ORION BOOSTER";
    case AZ_UPG_HARDENED_ARMOR: return "HARDENED ARMOR";
    case AZ_UPG_THERMAL_ARMOR: return "THERMAL ARMOR";
    case AZ_UPG_GRAVITIC_ARMOR: return "GRAVITIC ARMOR";
    case AZ_UPG_DYNAMIC_ARMOR: return "DYNAMIC ARMOR";
    case AZ_UPG_REACTIVE_ARMOR: return "REACTIVE ARMOR";
    case AZ_UPG_CAPACITOR_00:
    case AZ_UPG_CAPACITOR_01:
    case AZ_UPG_CAPACITOR_02:
    case AZ_UPG_CAPACITOR_03:
    case AZ_UPG_CAPACITOR_04:
    case AZ_UPG_CAPACITOR_05:
      return "CAPACITOR";
    case AZ_UPG_FUSION_REACTOR: return "FUSION REACTOR";
    case AZ_UPG_QUANTUM_REACTOR: return "QUANTUM REACTOR";
    case AZ_UPG_SHIELD_BATTERY_00:
    case AZ_UPG_SHIELD_BATTERY_01:
    case AZ_UPG_SHIELD_BATTERY_02:
    case AZ_UPG_SHIELD_BATTERY_03:
    case AZ_UPG_SHIELD_BATTERY_04:
    case AZ_UPG_SHIELD_BATTERY_05:
    case AZ_UPG_SHIELD_BATTERY_06:
    case AZ_UPG_SHIELD_BATTERY_07:
    case AZ_UPG_SHIELD_BATTERY_08:
    case AZ_UPG_SHIELD_BATTERY_09:
    case AZ_UPG_SHIELD_BATTERY_10:
    case AZ_UPG_SHIELD_BATTERY_11:
      return "SHIELD BATTERY";
  }
  AZ_ASSERT_UNREACHABLE();
}

/*===========================================================================*/
