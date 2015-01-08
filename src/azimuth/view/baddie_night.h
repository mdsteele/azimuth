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
#ifndef AZIMUTH_VIEW_BADDIE_NIGHT_H_
#define AZIMUTH_VIEW_BADDIE_NIGHT_H_

#include "azimuth/state/baddie.h"
#include "azimuth/util/clock.h"

/*===========================================================================*/

void az_draw_bad_nightbug(
    const az_baddie_t *baddie, float frozen, az_clock_t clock);

void az_draw_bad_nightshade(
    const az_baddie_t *baddie, float frozen, az_clock_t clock);

void az_draw_bad_nocturne(const az_baddie_t *baddie, az_clock_t clock);

/*===========================================================================*/

#endif // AZIMUTH_VIEW_BADDIE_NIGHT_H_
