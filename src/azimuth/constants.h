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
#ifndef AZIMUTH_CONSTANTS_H_
#define AZIMUTH_CONSTANTS_H_

/*===========================================================================*/
// Screen:

// The dimensions of the screen, in pixels:
#define AZ_SCREEN_WIDTH  640
#define AZ_SCREEN_HEIGHT 480

// The distance from the center of the screen to the corner:
#define AZ_SCREEN_RADIUS 400
#if (AZ_SCREEN_WIDTH/2)*(AZ_SCREEN_WIDTH/2) + \
    (AZ_SCREEN_HEIGHT/2)*(AZ_SCREEN_HEIGHT/2) != \
    AZ_SCREEN_RADIUS*AZ_SCREEN_RADIUS
#error "Incorrect AZ_SCREEN_RADIUS value."
#endif

/*===========================================================================*/
// Pickups:

// How close the ship must be to a pickup to collect it, in pixels:
#define AZ_PICKUP_COLLECTION_RANGE 20.0
// How long pickups last before expiring, in seconds:
#define AZ_PICKUP_MAX_AGE 10.0

/*===========================================================================*/
// Planetoid:

// The maximum number of rooms allowed in the game scenario:
#define AZ_MAX_NUM_ROOMS 192
// The radius of the planetoid in which the game takes place, in pixels:
#define AZ_PLANETOID_RADIUS 100000.0
// The acceleration due to gravity at the surface of the planetoid, in
// pixels/second^2:
#define AZ_PLANETOID_SURFACE_GRAVITY 1.6

/*===========================================================================*/
// Player:

// The max energy you start with, with no upgrades:
#define AZ_INITIAL_MAX_ENERGY 100
// The max shields you start with, with no upgrades:
#define AZ_INITIAL_MAX_SHIELDS 100

/*===========================================================================*/
// Saving:

// How many slots there are in which to save a game:
#define AZ_NUM_SAVED_GAME_SLOTS 6

/*===========================================================================*/
// Ship:

// How fast the ship should be able to travel, using only normal thrusters,
// with no upgrades, in pixels/second:
#define AZ_SHIP_BASE_MAX_SPEED 400.0
// How fast the ship recharges energy without upgrades, in energy/second:
#define AZ_SHIP_BASE_RECHARGE_RATE 75.0
// How fast the ship accelerates with normal thrusters, in pixels/second^2:
#define AZ_SHIP_BASE_THRUST_ACCEL 500.0
// How fast the ship can turn, in radians/second:
#define AZ_SHIP_TURN_RATE 5.0
// Max range of tractor beam, in pixels:
#define AZ_TRACTOR_BEAM_MAX_RANGE 300.0

/*===========================================================================*/
// Upgrades:

// How close the ship must be to an upgrade to pick it up:
#define AZ_UPGRADE_COLLECTION_RADIUS 30.0

// The factor by which having the Dynamic Armor upgrade boosts your effective
// maximum speed with normal thrusters:
#define AZ_DYNAMIC_ARMOR_SPEED_MULT 1.5
// The additional energy produced by the Fusion Reactor upgrade, in
// energy/second:
#define AZ_FUSION_REACTOR_RECHARGE_RATE 75.0
// The additional energy produced by the Quantum Reactor upgrade, in
// energy/second:
#define AZ_QUANTUM_REACTOR_RECHARGE_RATE 125.0

/*===========================================================================*/
// Weapons:

// How much damage the BEAM gun normally deals per second:
#define AZ_BEAM_GUN_BASE_DAMAGE_PER_SECOND 15.0
// How much energy the BEAM gun normally uses per second:
#define AZ_BEAM_GUN_BASE_ENERGY_PER_SECOND 75.0

/*===========================================================================*/

#endif // AZIMUTH_CONSTANTS_H_
