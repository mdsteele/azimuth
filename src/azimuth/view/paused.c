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

#include "azimuth/view/paused.h"

#include <assert.h>

#include <GL/gl.h>

#include "azimuth/state/player.h"
#include "azimuth/state/upgrade.h"
#include "azimuth/view/cursor.h"
#include "azimuth/view/string.h"

/*===========================================================================*/

static void draw_rect(double x, double y, double w, double h) {
  x += 0.5;
  y += 0.5;
  w -= 1.0;
  h -= 1.0;
  glBegin(GL_LINE_LOOP); {
    glVertex2d(x, y);
    glVertex2d(x + w, y);
    glVertex2d(x + w, y + h);
    glVertex2d(x, y + h);
  } glEnd();
}

static void draw_upg_box(const az_player_t *player, double x, double y,
                         az_upgrade_t upgrade) {
  glColor3f(0, 1, 1);
  draw_rect(x, y, 150, 15);
  if (!az_has_upgrade(player, upgrade)) return;
  az_draw_string(8, AZ_ALIGN_CENTER, x + 75, y + 4, az_upgrade_name(upgrade));
}

static int round_towards_middle(double amount, double maximum) {
  assert(amount >= 0.0);
  assert(amount <= maximum);
  const int offset = 0.5 * maximum;
  return (int)(amount - offset) + offset;
}

void az_paused_draw_screen(az_paused_state_t *state) {
  const az_player_t *player = state->player;

  glColor3f(1, 0, 1);
  draw_rect(20, 26, 150, 15);
  az_draw_printf(8, AZ_ALIGN_CENTER, 95, 30, "SHIELD: %3d/%-3d",
                 round_towards_middle(player->shields, player->max_shields),
                 (int)player->max_shields);
  draw_rect(20, 46, 150, 15);
  az_draw_printf(8, AZ_ALIGN_CENTER, 95, 50, "ENERGY: %3d/%-3d",
                 round_towards_middle(player->energy, player->max_energy),
                 (int)player->max_energy);

  for (int i = 0; i < 8; ++i) {
    const int row = i / 4;
    const int col = i % 4;
    draw_rect(185 + 70 * col, 26 + 20 * row, 60, 15);
    if (az_has_upgrade(player, AZ_UPG_GUN_CHARGE + i)) {
      az_draw_string(8, AZ_ALIGN_CENTER, 215 + 70 * col, 30 + 20 * row,
                     az_gun_name(AZ_GUN_CHARGE + i));
    }
  }

  draw_rect(470, 26, 150, 15);
  if (player->max_rockets > 0) {
    az_draw_printf(8, AZ_ALIGN_CENTER, 545, 30, "ROCKETS:%3d/%-3d",
                   player->rockets, player->max_rockets);
  }
  draw_rect(470, 46, 150, 15);
  if (player->max_bombs > 0) {
    az_draw_printf(8, AZ_ALIGN_CENTER, 545, 50, "  BOMBS:%3d/%-3d",
                   player->bombs, player->max_bombs);
  }

  draw_upg_box(player, 20, 80, AZ_UPG_HARDENED_ARMOR);
  draw_upg_box(player, 20, 100, AZ_UPG_THERMAL_ARMOR);
  draw_upg_box(player, 20, 120, AZ_UPG_DYNAMIC_ARMOR);
  draw_upg_box(player, 20, 140, AZ_UPG_GRAVITIC_ARMOR);
  draw_upg_box(player, 20, 160, AZ_UPG_REACTIVE_ARMOR);

  draw_upg_box(player, 20, 200, AZ_UPG_FUSION_REACTOR);
  draw_upg_box(player, 20, 220, AZ_UPG_QUANTUM_REACTOR);

  draw_upg_box(player, 20, 260, AZ_UPG_TRACTOR_BEAM);
  draw_upg_box(player, 20, 280, AZ_UPG_INFRASCANNER);

  draw_upg_box(player, 470, 80, AZ_UPG_HYPER_ROCKETS);
  draw_upg_box(player, 470, 100, AZ_UPG_MEGA_BOMBS);
  draw_upg_box(player, 470, 120, AZ_UPG_HIGH_EXPLOSIVES);
  draw_upg_box(player, 470, 140, AZ_UPG_ATTUNED_EXPLOSIVES);

  draw_upg_box(player, 470, 180, AZ_UPG_RETRO_THRUSTERS);
  draw_upg_box(player, 470, 200, AZ_UPG_LATERAL_THRUSTERS);
  draw_upg_box(player, 470, 220, AZ_UPG_CPLUS_DRIVE);
  draw_upg_box(player, 470, 240, AZ_UPG_ORION_BOOSTER);

  az_draw_cursor();
}

/*===========================================================================*/
