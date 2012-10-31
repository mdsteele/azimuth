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
#ifndef AZIMUTH_STATE_SAVE_H_
#define AZIMUTH_STATE_SAVE_H_

#include <stdbool.h>

#include "azimuth/state/planet.h"
#include "azimuth/state/player.h"

/*===========================================================================*/

typedef struct {
  struct {
    bool present; // false if there is nothing saved here
    az_player_t player;
  } games[6];
} az_saved_games_t;

void az_reset_saved_games(az_saved_games_t *games);

bool az_load_games_from_file(const az_planet_t *planet,
                             const char *filepath,
                             az_saved_games_t *games_out);

bool az_save_games_to_file(const az_saved_games_t *games,
                           const char *filepath);

/*===========================================================================*/

#endif // AZIMUTH_STATE_SAVE_H_
