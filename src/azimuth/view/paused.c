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
#include <math.h>

#include <GL/gl.h>

#include "azimuth/constants.h"
#include "azimuth/state/camera.h"
#include "azimuth/state/planet.h"
#include "azimuth/state/player.h"
#include "azimuth/state/upgrade.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/vector.h"
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

/*===========================================================================*/

#define MINIMAP_ZOOM 100.0

static void draw_minimap_rooms(az_paused_state_t *state) {
  // Draw planet surface outline:
  glColor3f(1, 1, 0); // yellow
  glBegin(GL_LINE_LOOP); {
    for (int i = 0; i < 360; i += 3) {
      glVertex2d(AZ_PLANETOID_RADIUS * cos(AZ_DEG2RAD(i)),
                 AZ_PLANETOID_RADIUS * sin(AZ_DEG2RAD(i)));
    }
  } glEnd();

  // Draw explored rooms:
  const az_planet_t *planet = state->planet;
  const az_player_t *player = state->player;
  for (int i = 0; i < planet->num_rooms; ++i) {
    if (!az_test_room_visited(player, i)) continue;
    const az_room_t *room = &planet->rooms[i];
    const az_camera_bounds_t *bounds = &room->camera_bounds;
    const az_color_t zone_color = planet->zones[room->zone_index].color;

    const double min_r = bounds->min_r - AZ_SCREEN_HEIGHT/2;
    const double max_r = min_r + bounds->r_span + AZ_SCREEN_HEIGHT;
    const double min_theta = bounds->min_theta;
    const double max_theta = min_theta + bounds->theta_span;

    const az_vector_t offset1 =
      az_vpolar(AZ_SCREEN_WIDTH/2, min_theta - AZ_HALF_PI);
    const az_vector_t offset2 =
      az_vpolar(AZ_SCREEN_WIDTH/2, max_theta + AZ_HALF_PI);
    const double step = fmax(AZ_DEG2RAD(0.1), bounds->theta_span * 0.05);

    // Fill room with color:
    if (i == player->current_room && az_clock_mod(2, 20, state->clock)) {
      glColor3f(1, 1, 1);
    } else glColor3ub(zone_color.r / 2, zone_color.g / 2, zone_color.b / 2);
    glBegin(GL_QUAD_STRIP); {
      glVertex2d(min_r * cos(min_theta) + offset1.x,
                 min_r * sin(min_theta) + offset1.y);
      glVertex2d(max_r * cos(min_theta) + offset1.x,
                 max_r * sin(min_theta) + offset1.y);
      for (double theta = min_theta; theta <= max_theta; theta += step) {
        glVertex2d(min_r * cos(theta), min_r * sin(theta));
        glVertex2d(max_r * cos(theta), max_r * sin(theta));
      }
      glVertex2d(min_r * cos(max_theta) + offset2.x,
                 min_r * sin(max_theta) + offset2.y);
      glVertex2d(max_r * cos(max_theta) + offset2.x,
                 max_r * sin(max_theta) + offset2.y);
    } glEnd();

    // Draw outline:
    glColor3f(1, 1, 1); // white
    glBegin(GL_LINE_LOOP); {
      glVertex2d(min_r * cos(min_theta) + offset1.x,
                 min_r * sin(min_theta) + offset1.y);
      glVertex2d(max_r * cos(min_theta) + offset1.x,
                 max_r * sin(min_theta) + offset1.y);
      for (double theta = min_theta; theta <= max_theta; theta += step) {
        glVertex2d(max_r * cos(theta), max_r * sin(theta));
      }
      glVertex2d(max_r * cos(max_theta) + offset2.x,
                 max_r * sin(max_theta) + offset2.y);
      glVertex2d(min_r * cos(max_theta) + offset2.x,
                 min_r * sin(max_theta) + offset2.y);
      for (double theta = max_theta; theta >= min_theta; theta -= step) {
        glVertex2d(min_r * cos(theta), min_r * sin(theta));
      }
    } glEnd();
  }
}

static void draw_minimap(az_paused_state_t *state) {
  const az_room_key_t current_room = state->player->current_room;
  assert(current_room < state->planet->num_rooms);
  const az_vector_t center =
    az_bounds_center(&state->planet->rooms[current_room].camera_bounds);

  glEnable(GL_SCISSOR_TEST); {
    glScissor(190, 110, 260, 260);
    glPushMatrix(); {
      // Make positive Y be up instead of down.
      glScaled(1, -1, 1);
      // Center the screen on position (0, 0).
      glTranslated(AZ_SCREEN_WIDTH/2, -AZ_SCREEN_HEIGHT/2, 0);
      // Zoom out.
      glScaled(1.0 / MINIMAP_ZOOM, 1.0 / MINIMAP_ZOOM, 1);
      // Move the screen to the camera pose.
      glTranslated(0, -az_vnorm(center), 0);
      glRotated(90.0 - AZ_RAD2DEG(az_vtheta(center)), 0, 0, 1);
      // Draw what the camera sees.
      draw_minimap_rooms(state);
    } glPopMatrix();
  } glDisable(GL_SCISSOR_TEST);

  glColor3f(1, 1, 1);
  draw_rect(190, 110, 260, 260);
}

/*===========================================================================*/

static void set_weapon_box_color(bool selected) {
  if (selected) glColor3f(1, 1, 1);
  else glColor3f(1, 0, 1);
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

static void draw_upgrades(az_paused_state_t *state) {
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
    const az_gun_t gun = AZ_GUN_CHARGE + i;
    set_weapon_box_color(state->player->gun1 == gun ||
                         state->player->gun2 == gun);
    draw_rect(185 + 70 * col, 26 + 20 * row, 60, 15);
    if (az_has_upgrade(player, AZ_UPG_GUN_CHARGE + i)) {
      az_draw_string(8, AZ_ALIGN_CENTER, 215 + 70 * col, 30 + 20 * row,
                     az_gun_name(gun));
    }
  }

  set_weapon_box_color(state->player->ordnance == AZ_ORDN_ROCKETS);
  draw_rect(470, 26, 150, 15);
  if (player->max_rockets > 0) {
    az_draw_printf(8, AZ_ALIGN_CENTER, 545, 30, "ROCKETS:%3d/%-3d",
                   player->rockets, player->max_rockets);
  }
  set_weapon_box_color(state->player->ordnance == AZ_ORDN_BOMBS);
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
}

/*===========================================================================*/

void az_paused_draw_screen(az_paused_state_t *state) {
  draw_minimap(state);
  draw_upgrades(state);
  az_draw_cursor();
}

void az_tick_paused_state(az_paused_state_t *state, double time) {
  ++state->clock;
}

/*===========================================================================*/
