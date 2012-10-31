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

#include "azimuth/state/save.h"

#include <assert.h>
#include <inttypes.h> // for PRIx64 and SCNx64
#include <stdbool.h>
#include <stdio.h>

#include "azimuth/state/planet.h"
#include "azimuth/state/player.h"
#include "azimuth/util/misc.h"

/*===========================================================================*/

void az_reset_saved_games(az_saved_games_t *games) {
  AZ_ARRAY_LOOP(game, games->games) game->present = false;
}

/*===========================================================================*/

static bool parse_saved_game(const az_planet_t *planet, FILE *file,
                             az_player_t *player) {
  az_init_player(player);
  uint64_t upgrades1, upgrades2;
  int rockets, bombs, gun1, gun2, ordnance;
  if (fscanf(file, " u1=%"SCNx64" u2=%"SCNx64
             " r1=%"SCNx64" r2=%"SCNx64" r3=%"SCNx64
             " f1=%"SCNx64" tt=%lf cr=%d"
             " rk=%d bm=%d g1=%d g2=%d or=%d\n",
             &upgrades1, &upgrades2,
             &player->rooms1, &player->rooms2, &player->rooms3,
             &player->flags, &player->total_time, &player->current_room,
             &rockets, &bombs, &gun1, &gun2, &ordnance) < 13) return false;
  if (player->total_time < 0.0) return false;
  if (player->current_room < 0 ||
      player->current_room >= planet->num_rooms) {
    return false;
  }
  for (unsigned int i = 0u; i < 64u; ++i) {
    if (upgrades1 & (UINT64_C(1) << i)) {
      az_give_upgrade(player, (az_upgrade_t)i);
    }
  }
  for (unsigned int i = 64u; i < 128u; ++i) {
    if (upgrades2 & (UINT64_C(1) << (i - 64u))) {
      az_give_upgrade(player, (az_upgrade_t)i);
    }
  }
  if (rockets < 0 || rockets > player->max_rockets) return false;
  else player->rockets = rockets;
  if (bombs < 0 || bombs > player->max_bombs) return false;
  else player->bombs = bombs;
  player->shields = player->max_shields;
  player->energy = player->max_energy;
  // TODO: validate and set gun1/gun2/ordnance
  return true;
}

static bool parse_saved_games(const az_planet_t *planet, FILE *file,
                              az_saved_games_t *games_out) {
  AZ_ARRAY_LOOP(game, games_out->games) {
    if (fgetc(file) != '@') return false;
    switch (fgetc(file)) {
      case 'S':
        game->present = true;
        parse_saved_game(planet, file, &game->player);
        break;
      case 'N':
        game->present = false;
        fscanf(file, "\n");
        break;
      default: return false;
    }
  }
  return true;
}

bool az_load_games_from_file(const az_planet_t *planet,
                             const char *filepath,
                             az_saved_games_t *games_out) {
  assert(games_out != NULL);
  FILE *file = fopen(filepath, "r");
  if (file == NULL) return false;
  const bool ok = parse_saved_games(planet, file, games_out);
  fclose(file);
  return ok;
}

/*===========================================================================*/

static bool write_games(const az_saved_games_t *games, FILE *file) {
  AZ_ARRAY_LOOP(game, games->games) {
    if (game->present) {
      const az_player_t *player = &game->player;
      if (fprintf(file, "@S u1=%"PRIx64" u2=%"PRIx64
                  " r1=%"PRIx64" r2=%"PRIx64" r3=%"PRIx64
                  " f1=%"PRIx64" tt=%.02f cr=%d"
                  " rk=%d bm=%d g1=%d g2=%d or=%d\n",
                  player->upgrades1, player->upgrades2,
                  player->rooms1, player->rooms2, player->rooms3,
                  player->flags, player->total_time, player->current_room,
                  player->rockets, player->bombs, player->gun1, player->gun2,
                  player->ordnance) < 0) return false;
    } else {
      if (fprintf(file, "@N\n") < 0) return false;
    }
  }
  return true;
}

bool az_save_games_to_file(const az_saved_games_t *games,
                           const char *filepath) {
  assert(games != NULL);
  FILE *file = fopen(filepath, "w");
  if (file == NULL) return false;
  const bool ok = write_games(games, file);
  fclose(file);
  return ok;
}

/*===========================================================================*/
