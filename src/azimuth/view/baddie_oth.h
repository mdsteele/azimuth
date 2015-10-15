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
#ifndef AZIMUTH_VIEW_BADDIE_OTH_H_
#define AZIMUTH_VIEW_BADDIE_OTH_H_

#include "azimuth/state/baddie.h"
#include "azimuth/util/clock.h"

/*===========================================================================*/

void az_draw_bad_oth_brawler(
    const az_baddie_t *baddie, float frozen, az_clock_t clock);

void az_draw_bad_oth_crab(
    const az_baddie_t *baddie, float frozen, az_clock_t clock);

void az_draw_bad_oth_crawler(
    const az_baddie_t *baddie, float frozen, az_clock_t clock);

void az_draw_bad_oth_gunship(
    const az_baddie_t *baddie, float frozen, az_clock_t clock);

void az_draw_bad_oth_minicrab(
    const az_baddie_t *baddie, float frozen, az_clock_t clock);

void az_draw_bad_oth_orb(
    const az_baddie_t *baddie, float frozen, az_clock_t clock);

void az_draw_bad_oth_razor_1(
    const az_baddie_t *baddie, float frozen, az_clock_t clock);

void az_draw_bad_oth_razor_2(
    const az_baddie_t *baddie, float frozen, az_clock_t clock);

void az_draw_bad_oth_snapdragon(
    const az_baddie_t *baddie, float frozen, az_clock_t clock);

/*===========================================================================*/

void az_draw_bad_reflection(const az_baddie_t *baddie, az_clock_t clock);

/*===========================================================================*/

#endif // AZIMUTH_VIEW_BADDIE_OTH_H_
