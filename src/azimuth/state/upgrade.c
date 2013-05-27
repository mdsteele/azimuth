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

void az_get_upgrade_description(az_upgrade_t upgrade, const char **line1_out,
                                const char **line2_out) {
  assert(line1_out != NULL);
  assert(line2_out != NULL);
  switch (upgrade) {
    case AZ_UPG_GUN_CHARGE:
      *line1_out = "Hold [V] to charge, release to fire.";
      *line2_out = "Charged shots can destroy certain walls.";
      break;
    case AZ_UPG_GUN_FREEZE:
      *line1_out = "Shots can freeze enemies.";
      *line2_out = "Press [2] to select, press [V] to fire.";
      break;
    case AZ_UPG_GUN_TRIPLE:
      *line1_out = "Fires three shots at once.";
      *line2_out = "Press [3] to select, press [V] to fire.";
      break;
    case AZ_UPG_GUN_HOMING:
      *line1_out = "Shots will seek out enemies.";
      *line2_out = "Press [4] to select, press [V] to fire.";
      break;
    case AZ_UPG_GUN_PHASE:
      *line1_out = "Shots will pass through walls.";
      *line2_out = "Press [5] to select, press [V] to fire.";
      break;
    case AZ_UPG_GUN_BURST:
      *line1_out = "Shots will explode into shrapnel on impact.";
      *line2_out = "Press [6] to select, press [V] to fire.";
      break;
    case AZ_UPG_GUN_PIERCE:
      *line1_out = "Shots will pierce through multiple enemies.";
      *line2_out = "Press [7] to select, press [V] to fire.";
      break;
    case AZ_UPG_GUN_BEAM:
      *line1_out = "Fires a continuous beam.";
      *line2_out = "Press [8] to select, press [V] to fire.";
      break;
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
      { AZ_STATIC_ASSERT(AZ_ROCKETS_PER_AMMO_UPGRADE == 5); }
      *line1_out = "Maximum rockets increased by 5.";
      *line2_out = "Press [9] to select, hold [C] and press [V] to fire.";
      break;
    case AZ_UPG_HYPER_ROCKETS:
      *line1_out = "Press [9] to select rockets, hold [C] to charge,";
      *line2_out = "and press [V] to fire.  Uses 5 rockets.";
      break;
    case AZ_UPG_HIGH_EXPLOSIVES:
      { AZ_STATIC_ASSERT(AZ_HIGH_EXPLOSIVES_POWER_MULTIPLIER == 1.5); }
      *line1_out = "Your rockets and bombs now deal 50% more damage.";
      *line2_out = NULL;
      break;
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
      { AZ_STATIC_ASSERT(AZ_BOMBS_PER_AMMO_UPGRADE == 3); }
      *line1_out = "Maximum bombs increased by 3.";
      *line2_out = "Press [0] to select, hold [C] and press [V] to drop.";
      break;
    case AZ_UPG_MEGA_BOMBS:
      *line1_out = "Press [0] to select bombs, hold [C] to charge,";
      *line2_out = "and press [V] to drop.  Uses 5 bombs.";
      break;
    case AZ_UPG_ATTUNED_EXPLOSIVES:
      { AZ_STATIC_ASSERT(AZ_ATTUNED_EXPLOSIVES_DAMAGE_FACTOR == 0.5); }
      *line1_out = "Your own rockets and bombs now deal only half";
      *line2_out = "as much splash damage to your own ship.";
      break;
    case AZ_UPG_TRACTOR_BEAM:
      *line1_out = "Hold [X] to lock onto flashing tractor nodes.";
      *line2_out = NULL;
      break;
    case AZ_UPG_INFRASCANNER:
      *line1_out = "Improves your ship's sensors in low-visibility areas.";
      *line2_out = NULL;
      break;
    case AZ_UPG_MAGNET_SWEEP:
      *line1_out = "Nearby shield and ammo pickups will be";
      *line2_out = "pulled toward your ship.";
      break;
    case AZ_UPG_RETRO_THRUSTERS:
      *line1_out = "Press [DOWN] to accelerate your ship backwards.";
      *line2_out = NULL;
      break;
    case AZ_UPG_CPLUS_DRIVE:
      *line1_out = "Fly straight without stopping to charge, then";
      *line2_out = "double-tap [UP].  Destroys certain walls on impact.";
      break;
    case AZ_UPG_ORION_BOOSTER:
      *line1_out = "Press [0] to select bombs, hold [C] to charge,";
      *line2_out = "then double-tap [DOWN] to boost.  Uses 5 bombs.";
      break;
    case AZ_UPG_HARDENED_ARMOR:
      { AZ_STATIC_ASSERT(AZ_ARMOR_DAMAGE_FACTOR == 0.8); }
      *line1_out = "All damage taken is reduced by one fifth.";
      { AZ_STATIC_ASSERT(AZ_HARDENED_ARMOR_WALL_DAMAGE_FACTOR == 0.5); }
      *line2_out =
        "Damage taken from hitting walls is further reduced by half.";
      break;
    case AZ_UPG_THERMAL_ARMOR:
      *line1_out = "Damage is no longer taken from extreme heat.";
      *line2_out = "All other damage taken is reduced by one fifth.";
      break;
    case AZ_UPG_GRAVITIC_ARMOR:
      *line1_out = "All damage taken is reduced by one fifth.";
      *line2_out =
        "The effects of gravity distortions on your ship are reduced.";
      break;
    case AZ_UPG_DYNAMIC_ARMOR:
      *line1_out = "All damage taken is reduced by one fifth.";
      *line2_out = "Drag forces from air and water are reduced.";
      break;
    case AZ_UPG_REACTIVE_ARMOR:
      *line1_out = "All damage taken is reduced by one fifth.";
      *line2_out = "Colliding with enemies will now damage them.";
      break;
    case AZ_UPG_CAPACITOR_00:
    case AZ_UPG_CAPACITOR_01:
    case AZ_UPG_CAPACITOR_02:
    case AZ_UPG_CAPACITOR_03:
    case AZ_UPG_CAPACITOR_04:
    case AZ_UPG_CAPACITOR_05:
      *line1_out = "Maximum energy increased by 50.";
      *line2_out = NULL;
      break;
    case AZ_UPG_FUSION_REACTOR:
      *line1_out = "Energy recharge rate is increased.";
      *line2_out = NULL;
      break;
    case AZ_UPG_QUANTUM_REACTOR:
      *line1_out = "Energy recharge rate is greatly increased.";
      *line2_out = NULL;
      break;
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
      *line1_out = "Maximum shields increased by 25.";
      *line2_out = NULL;
      break;
  }
}

/*===========================================================================*/
