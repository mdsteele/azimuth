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
#ifndef AZIMUTH_TICK_BADDIE_TURRET_H_
#define AZIMUTH_TICK_BADDIE_TURRET_H_

#include "azimuth/state/baddie.h"
#include "azimuth/state/space.h"

/*===========================================================================*/

void az_tick_bad_turret(
    az_space_state_t *state, az_baddie_t *baddie, double time);
void az_tick_bad_beam_turret(
    az_space_state_t *state, az_baddie_t *baddie, double time);
void az_tick_bad_broken_turret(
    az_space_state_t *state, az_baddie_t *baddie, double time);
void az_tick_bad_heavy_turret(
    az_space_state_t *state, az_baddie_t *baddie, double time);
void az_tick_bad_rocket_turret(
    az_space_state_t *state, az_baddie_t *baddie, double time);

void az_tick_bad_crawling_turret(
    az_space_state_t *state, az_baddie_t *baddie, double time);
void az_tick_bad_crawling_mortar(
    az_space_state_t *state, az_baddie_t *baddie, double time);

void az_tick_bad_security_drone(
    az_space_state_t *state, az_baddie_t *baddie, double time);

/*===========================================================================*/

#endif // AZIMUTH_TICK_BADDIE_TURRET_H_
