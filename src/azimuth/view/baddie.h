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
#ifndef AZIMUTH_VIEW_BADDIE_H_
#define AZIMUTH_VIEW_BADDIE_H_

#include "azimuth/state/baddie.h"
#include "azimuth/state/space.h"

/*===========================================================================*/

// Draw a single baddie.  The GL matrix should be at the camera position.
void az_draw_baddie(const az_baddie_t *baddie);

// Draw all baddies.  The GL matrix should be at the camera position.
void az_draw_baddies(const az_space_state_t *state);

/*===========================================================================*/

#endif // AZIMUTH_VIEW_BADDIE_H_
