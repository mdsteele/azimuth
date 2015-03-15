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
#ifndef AZIMUTH_STATE_BACKGROUND_H_
#define AZIMUTH_STATE_BACKGROUND_H_

/*===========================================================================*/

// The number of background patterns there are, including AZ_BG_SOLID_BLACK:
#define AZ_NUM_BG_PATTERNS 8

typedef enum {
  AZ_BG_SOLID_BLACK = 0,
  AZ_BG_BROWN_ROCK_WALL,
  AZ_BG_GREEN_HEX_TRELLIS,
  AZ_BG_YELLOW_PANELLING,
  AZ_BG_PURPLE_ROCK_WALL,
  AZ_BG_RED_GIRDERS,
  AZ_BG_GREEN_ROCK_WALL,
  AZ_BG_GREEN_ROCK_WALL_WITH_GIRDERS,
} az_background_pattern_t;

/*===========================================================================*/

#endif // AZIMUTH_STATE_BACKGROUND_H_
