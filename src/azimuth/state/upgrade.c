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

#include <assert.h>
#include <stddef.h> // for NULL

#include "azimuth/constants.h"
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
    case AZ_UPG_HYPER_ROCKETS: return "HYPER ROCKETS";
    case AZ_UPG_MEGA_BOMBS: return "MEGA BOMBS";
    case AZ_UPG_HIGH_EXPLOSIVES: return "HIGH EXPLOSIVES";
    case AZ_UPG_ATTUNED_EXPLOSIVES: return "ATTUNED EXPLOSIVES";
    case AZ_UPG_RETRO_THRUSTERS: return "RETRO THRUSTERS";
    case AZ_UPG_CPLUS_DRIVE: return "C-PLUS DRIVE";
    case AZ_UPG_ORION_BOOSTER: return "ORION BOOSTER";
    case AZ_UPG_HARDENED_ARMOR: return "HARDENED ARMOR";
    case AZ_UPG_DYNAMIC_ARMOR: return "DYNAMIC ARMOR";
    case AZ_UPG_THERMAL_ARMOR: return "THERMAL ARMOR";
    case AZ_UPG_REACTIVE_ARMOR: return "REACTIVE ARMOR";
    case AZ_UPG_FUSION_REACTOR: return "FUSION REACTOR";
    case AZ_UPG_QUANTUM_REACTOR: return "QUANTUM REACTOR";
    case AZ_UPG_TRACTOR_BEAM: return "TRACTOR BEAM";
    case AZ_UPG_INFRASCANNER: return "INFRASCANNER";
    case AZ_UPG_MAGNET_SWEEP: return "MAGNET SWEEP";
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
    case AZ_UPG_ROCKET_AMMO_20:
    case AZ_UPG_ROCKET_AMMO_21:
    case AZ_UPG_ROCKET_AMMO_22:
    case AZ_UPG_ROCKET_AMMO_23:
    case AZ_UPG_ROCKET_AMMO_24:
    case AZ_UPG_ROCKET_AMMO_25:
    case AZ_UPG_ROCKET_AMMO_26:
    case AZ_UPG_ROCKET_AMMO_27:
    case AZ_UPG_ROCKET_AMMO_28:
    case AZ_UPG_ROCKET_AMMO_29:
      return "ROCKETS";
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
    case AZ_UPG_BOMB_AMMO_10:
    case AZ_UPG_BOMB_AMMO_11:
    case AZ_UPG_BOMB_AMMO_12:
    case AZ_UPG_BOMB_AMMO_13:
    case AZ_UPG_BOMB_AMMO_14:
    case AZ_UPG_BOMB_AMMO_15:
    case AZ_UPG_BOMB_AMMO_16:
    case AZ_UPG_BOMB_AMMO_17:
    case AZ_UPG_BOMB_AMMO_18:
    case AZ_UPG_BOMB_AMMO_19:
      return "BOMBS";
    case AZ_UPG_CAPACITOR_00:
    case AZ_UPG_CAPACITOR_01:
    case AZ_UPG_CAPACITOR_02:
    case AZ_UPG_CAPACITOR_03:
    case AZ_UPG_CAPACITOR_04:
    case AZ_UPG_CAPACITOR_05:
      return "CAPACITOR";
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

const char *az_upgrade_description(az_upgrade_t upgrade) {
  switch (upgrade) {
    case AZ_UPG_GUN_CHARGE:
      return ("Hold [$f] to charge, release to fire.\n"
              "Charged shots can destroy certain walls.");
    case AZ_UPG_GUN_FREEZE:
      return ("Shots can freeze enemies.\n"
              "Press [2] to select, press [$f] to fire.");
    case AZ_UPG_GUN_TRIPLE:
      return ("Fires three shots at once.\n"
              "Press [3] to select, press [$f] to fire.");
    case AZ_UPG_GUN_HOMING:
      return ("Shots will seek out enemies.\n"
              "Press [4] to select, press [$f] to fire.");
    case AZ_UPG_GUN_PHASE:
      return ("Shots will pass through walls.\n"
              "Press [5] to select, press [$f] to fire.");
    case AZ_UPG_GUN_BURST:
      return ("Shots will explode into shrapnel on impact.\n"
              "Press [6] to select, press [$f] to fire.");
    case AZ_UPG_GUN_PIERCE:
      return ("Shots will pierce through multiple enemies.\n"
              "Press [7] to select, press [$f] to fire.");
    case AZ_UPG_GUN_BEAM:
      return ("Fires a continuous beam.\n"
              "Press [8] to select, press [$f] to fire.");
    case AZ_UPG_HYPER_ROCKETS:
      { AZ_STATIC_ASSERT(AZ_ROCKETS_PER_HYPER_ROCKET == 3); }
      return ("Press [9] to select rockets, hold [$o] to charge,\n"
              "and press [$f] to fire.  Uses 3 rockets.");
    case AZ_UPG_MEGA_BOMBS:
      { AZ_STATIC_ASSERT(AZ_BOMBS_PER_MEGA_BOMB == 3); }
      return ("Press [0] to select bombs, hold [$o] to charge,\n"
              "and press [$f] to drop.  Uses 3 bombs.");
    case AZ_UPG_HIGH_EXPLOSIVES:
      { AZ_STATIC_ASSERT(AZ_HIGH_EXPLOSIVES_POWER_MULTIPLIER == 1.5); }
      return "Your rockets and bombs now deal 50% more damage.";
    case AZ_UPG_ATTUNED_EXPLOSIVES:
      { AZ_STATIC_ASSERT(AZ_ATTUNED_EXPLOSIVES_DAMAGE_FACTOR == 0.5); }
      return ("Your own rockets and bombs now deal only half\n"
              "as much splash damage to your own ship.");
    case AZ_UPG_RETRO_THRUSTERS:
      return "Press [$d] to accelerate your ship backwards.";
    case AZ_UPG_CPLUS_DRIVE:
      return ("Fly straight without stopping to charge, then\n"
              "double-tap-and-hold [$u].  Destroys certain walls on impact.");
    case AZ_UPG_ORION_BOOSTER:
      { AZ_STATIC_ASSERT(AZ_BOMBS_PER_ORION_BOOST == 2); }
      return ("Press [0] to select bombs, hold [$o] to charge,\n"
              "then double-tap [$d] to boost.  Uses 2 bombs.");
    case AZ_UPG_HARDENED_ARMOR:
      { AZ_STATIC_ASSERT(AZ_ARMOR_DAMAGE_FACTOR == 0.8); }
      { AZ_STATIC_ASSERT(AZ_HARDENED_ARMOR_WALL_DAMAGE_FACTOR == 0.5); }
      return ("All damage taken is reduced by one fifth.\n"
              "Damage taken from hitting walls is further reduced by half.");
    case AZ_UPG_THERMAL_ARMOR:
      return ("Damage is no longer taken from extreme heat.\n"
              "All other damage taken is reduced by one fifth.");
    case AZ_UPG_DYNAMIC_ARMOR:
      return ("All damage taken is reduced by one fifth.\n"
              "Drag forces from air and water are reduced.");
    case AZ_UPG_REACTIVE_ARMOR:
      return ("All damage taken is reduced by one fifth.\n"
              "Colliding with enemies will now damage them.");
    case AZ_UPG_FUSION_REACTOR:
      return "Energy recharge rate is increased.";
    case AZ_UPG_QUANTUM_REACTOR:
      return "Energy recharge rate is greatly increased.";
    case AZ_UPG_TRACTOR_BEAM:
      return "Hold [$t] to lock onto flashing tractor nodes.";
    case AZ_UPG_INFRASCANNER:
      return "Improves your ship's sensors in low-visibility areas.";
    case AZ_UPG_MAGNET_SWEEP:
      return ("Nearby shield and ammo pickups will be\n"
              "pulled toward your ship.");
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
    case AZ_UPG_ROCKET_AMMO_20:
    case AZ_UPG_ROCKET_AMMO_21:
    case AZ_UPG_ROCKET_AMMO_22:
    case AZ_UPG_ROCKET_AMMO_23:
    case AZ_UPG_ROCKET_AMMO_24:
    case AZ_UPG_ROCKET_AMMO_25:
    case AZ_UPG_ROCKET_AMMO_26:
    case AZ_UPG_ROCKET_AMMO_27:
    case AZ_UPG_ROCKET_AMMO_28:
    case AZ_UPG_ROCKET_AMMO_29:
      { AZ_STATIC_ASSERT(AZ_ROCKETS_PER_AMMO_UPGRADE == 5); }
      return ("Maximum rockets increased by 5.\n"
              "Press [9] to select, hold [$o] and press [$f] to fire.");
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
    case AZ_UPG_BOMB_AMMO_10:
    case AZ_UPG_BOMB_AMMO_11:
    case AZ_UPG_BOMB_AMMO_12:
    case AZ_UPG_BOMB_AMMO_13:
    case AZ_UPG_BOMB_AMMO_14:
    case AZ_UPG_BOMB_AMMO_15:
    case AZ_UPG_BOMB_AMMO_16:
    case AZ_UPG_BOMB_AMMO_17:
    case AZ_UPG_BOMB_AMMO_18:
    case AZ_UPG_BOMB_AMMO_19:
      { AZ_STATIC_ASSERT(AZ_BOMBS_PER_AMMO_UPGRADE == 3); }
      return ("Maximum bombs increased by 3.\n"
              "Press [0] to select, hold [$o] and press [$f] to drop.");
    case AZ_UPG_CAPACITOR_00:
    case AZ_UPG_CAPACITOR_01:
    case AZ_UPG_CAPACITOR_02:
    case AZ_UPG_CAPACITOR_03:
    case AZ_UPG_CAPACITOR_04:
    case AZ_UPG_CAPACITOR_05:
      return "Maximum energy increased by 50.";
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
      return "Maximum shields increased by 25.";
  }
  AZ_ASSERT_UNREACHABLE();
}

/*===========================================================================*/
