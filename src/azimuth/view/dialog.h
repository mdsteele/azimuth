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
#ifndef AZIMUTH_VIEW_DIALOG_H_
#define AZIMUTH_VIEW_DIALOG_H_

#include <stdbool.h>

#include "azimuth/state/dialog.h"
#include "azimuth/util/clock.h"

/*===========================================================================*/

// Call this at program startup to initialize drawing of portraits.  This must
// be called _after_ az_init_gui, and must be called _before_ any calls to
// az_draw_portrait.
void az_init_portrait_drawing(void);

// Draw a single right-facing portrait.  The GL matrix should be at the
// bottom-left corner of the portrait.
void az_draw_portrait(az_portrait_t portrait, bool talking, az_clock_t clock);

/*===========================================================================*/

#endif // AZIMUTH_VIEW_DIALOG_H_
