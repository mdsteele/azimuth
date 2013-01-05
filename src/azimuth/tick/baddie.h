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
#ifndef AZIMUTH_TICK_BADDIE_H_
#define AZIMUTH_TICK_BADDIE_H_

#include "azimuth/state/space.h"

/*===========================================================================*/

// Reduce the baddie's health by the given amount, if the baddie is susceptible
// to the given kind of damage.  Return true iff any damage was dealt.  If this
// is enough to destroy the baddie, remove it and add particles/pickups in its
// place.  If the the given damage kind includes AZ_DMGF_FREEZE, this may
// freeze the baddie.
bool az_try_damage_baddie(az_space_state_t *state, az_baddie_t *baddie,
                          const az_component_data_t *component,
                          az_damage_flags_t damage_kind, double damage_amount);

void az_tick_baddies(az_space_state_t *state, double time);

/*===========================================================================*/

#endif // AZIMUTH_TICK_BADDIE_H_
