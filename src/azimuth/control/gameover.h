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
#ifndef AZIMUTH_CONTROL_GAMEOVER_H_
#define AZIMUTH_CONTROL_GAMEOVER_H_

/*===========================================================================*/

typedef enum {
  AZ_GOA_TRY_AGAIN,
  AZ_GOA_RETURN_TO_TITLE,
  AZ_GOA_QUIT
} az_gameover_action_t;

az_gameover_action_t az_gameover_event_loop(void);

/*===========================================================================*/

#endif // AZIMUTH_CONTROL_GAMEOVER_H_
