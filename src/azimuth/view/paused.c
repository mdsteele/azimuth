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
#include "azimuth/state/ship.h"
#include "azimuth/state/upgrade.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/vector.h"
#include "azimuth/view/cursor.h"
#include "azimuth/view/hud.h"
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

#define MINIMAP_ZOOM 75.0

static void begin_map_marker(az_color_t color, az_vector_t center) {
  glPushMatrix();
  glTranslated(center.x, center.y, 0);
  glScaled(MINIMAP_ZOOM, MINIMAP_ZOOM, 0);
  glColor4ub(color.r, color.g, color.b, color.a);
  glBegin(GL_QUADS); {
    glVertex2f(4, 4); glVertex2f(-4, 4); glVertex2f(-4, -4); glVertex2f(4, -4);
  } glEnd();
  glColor3f(0, 0, 0);
  glBegin(GL_LINE_STRIP);
}

static void end_map_marker(void) {
  glEnd();
  glPopMatrix();
}

static void draw_minimap_rooms(const az_paused_state_t *state) {
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
  const az_ship_t *ship = state->ship;
  const az_player_t *player = &ship->player;
  for (int i = 0; i < planet->num_rooms; ++i) {
    const az_room_t *room = &planet->rooms[i];
    const bool visited = az_test_room_visited(player, i);
    const bool mapped = !(room->properties & AZ_ROOMF_UNMAPPED) &&
      az_test_zone_mapped(player, room->zone_key);
    if (!visited && !mapped) continue;

    az_draw_minimap_room(planet, room, visited, false);

    // Draw a console marker (if any).  We mark all save rooms on the map, but
    // only mark refill/comm rooms if we've actually visited them.
    if (az_clock_mod(2, 30, state->clock) == 0) {
      const az_vector_t center = az_bounds_center(&room->camera_bounds);
      if (room->properties & AZ_ROOMF_WITH_SAVE) {
        begin_map_marker((az_color_t){128, 255, 128, 255}, center); {
          glVertex2f(2, 2); glVertex2f(-2, 2); glVertex2f(-2, 0);
          glVertex2f(2, 0); glVertex2f(2, -2); glVertex2f(-2, -2);
        } end_map_marker();
      } else if (visited) {
        if (room->properties & AZ_ROOMF_WITH_REFILL) {
          begin_map_marker((az_color_t){255, 255, 128, 255}, center); {
            glVertex2f(-2, -2); glVertex2f(-2, 2); glVertex2f(2, 2);
            glVertex2f(2, 0); glVertex2f(-2, 0); glVertex2f(2, -2);
          } end_map_marker();
        } else if (room->properties & AZ_ROOMF_WITH_COMM) {
          begin_map_marker((az_color_t){128, 128, 255, 255}, center); {
            glVertex2f(2, 2); glVertex2f(-2, 2);
            glVertex2f(-2, -2); glVertex2f(2, -2);
          } end_map_marker();
        }
      }
    }
  }

  // Draw a marker for the current ship position:
  if (az_clock_mod(2, 12, state->clock) == 0) {
    glPushMatrix(); {
      glTranslated(ship->position.x, ship->position.y, 0);
      glRotated(AZ_RAD2DEG(ship->angle), 0, 0, 1);
      glBegin(GL_LINE_LOOP); {
        glColor3f(0, 1, 1);
        glVertex2f(700, 0); glVertex2f(100, 100);
        glVertex2f(0, 700); glVertex2f(-100, 100);
        glVertex2f(-700, 0); glVertex2f(-100, -100);
        glVertex2f(0, -700); glVertex2f(100, -100);
      } glEnd();
    } glPopMatrix();
  }
}

static void draw_minimap(az_paused_state_t *state) {
  const az_room_key_t current_room = state->ship->player.current_room;
  assert(current_room < state->planet->num_rooms);
  const az_camera_bounds_t *bounds =
    &state->planet->rooms[current_room].camera_bounds;
  const double rho = fmin(bounds->min_r + 0.5 * bounds->r_span,
                          AZ_PLANETOID_RADIUS - 180 * MINIMAP_ZOOM);
  const az_vector_t center = az_vpolar(rho, AZ_HALF_PI);

  glEnable(GL_SCISSOR_TEST); {
    glScissor(30, 50, 580, 400);
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

  glColor3f(0.75, 0.75, 0.75);
  draw_rect(30, 30, 580, 400);
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

static const az_vector_t drawer_vertices[] = {
  {12.5, 480.5}, {12.5, 18.5}, {22.5, 8.5}, {200.5, 8.5}, {204.5, 12.5},
  {423.5, 12.5}, {427.5, 8.5}, {627.5, 8.5}, {627.5, 480.5}
};

static void draw_upgrades(az_paused_state_t *state) {
  const az_player_t *player = &state->ship->player;

  glColor4f(0, 0, 0, 0.9); // black tint
  glBegin(GL_POLYGON); {
    AZ_ARRAY_LOOP(vertex, drawer_vertices) glVertex2d(vertex->x, vertex->y);
  } glEnd();
  glColor3f(0.75, 0.75, 0.75); // light gray
  glBegin(GL_LINE_LOOP); {
    AZ_ARRAY_LOOP(vertex, drawer_vertices) glVertex2d(vertex->x, vertex->y);
  } glEnd();

  glColor3f(1, 0, 1);
  draw_rect(24, 26, 150, 15);
  az_draw_printf(8, AZ_ALIGN_CENTER, 95, 30, "SHIELD: %3d/%-3d",
                 round_towards_middle(player->shields, player->max_shields),
                 (int)player->max_shields);
  draw_rect(24, 46, 150, 15);
  az_draw_printf(8, AZ_ALIGN_CENTER, 95, 50, "ENERGY: %3d/%-3d",
                 round_towards_middle(player->energy, player->max_energy),
                 (int)player->max_energy);

  for (int i = 0; i < 8; ++i) {
    const int row = i / 4;
    const int col = i % 4;
    const az_gun_t gun = AZ_GUN_CHARGE + i;
    set_weapon_box_color(player->gun1 == gun || player->gun2 == gun);
    draw_rect(191 + 66 * col, 26 + 20 * row, 60, 15);
    if (az_has_upgrade(player, AZ_UPG_GUN_CHARGE + i)) {
      az_draw_string(8, AZ_ALIGN_CENTER, 221 + 66 * col, 30 + 20 * row,
                     az_gun_name(gun));
    }
  }

  set_weapon_box_color(player->ordnance == AZ_ORDN_ROCKETS);
  draw_rect(466, 26, 150, 15);
  if (player->max_rockets > 0) {
    az_draw_printf(8, AZ_ALIGN_CENTER, 545, 30, "ROCKETS:%3d/%-3d",
                   player->rockets, player->max_rockets);
  }
  set_weapon_box_color(player->ordnance == AZ_ORDN_BOMBS);
  draw_rect(466, 46, 150, 15);
  if (player->max_bombs > 0) {
    az_draw_printf(8, AZ_ALIGN_CENTER, 545, 50, "  BOMBS:%3d/%-3d",
                   player->bombs, player->max_bombs);
  }

  if (state->drawer_openness <= 0.0) return;

  draw_upg_box(player, 24,  80, AZ_UPG_HARDENED_ARMOR);
  draw_upg_box(player, 24, 100, AZ_UPG_DYNAMIC_ARMOR);
  draw_upg_box(player, 24, 120, AZ_UPG_THERMAL_ARMOR);
  draw_upg_box(player, 24, 140, AZ_UPG_REACTIVE_ARMOR);

  draw_upg_box(player, 24, 180, AZ_UPG_FUSION_REACTOR);
  draw_upg_box(player, 24, 200, AZ_UPG_QUANTUM_REACTOR);

  draw_upg_box(player, 24, 240, AZ_UPG_TRACTOR_BEAM);
  draw_upg_box(player, 24, 260, AZ_UPG_INFRASCANNER);
  draw_upg_box(player, 24, 280, AZ_UPG_MAGNET_SWEEP);

  draw_upg_box(player, 466,  80, AZ_UPG_HYPER_ROCKETS);
  draw_upg_box(player, 466, 100, AZ_UPG_MEGA_BOMBS);
  draw_upg_box(player, 466, 120, AZ_UPG_HIGH_EXPLOSIVES);
  draw_upg_box(player, 466, 140, AZ_UPG_ATTUNED_EXPLOSIVES);

  draw_upg_box(player, 466, 180, AZ_UPG_RETRO_THRUSTERS);
  draw_upg_box(player, 466, 200, AZ_UPG_CPLUS_DRIVE);
  draw_upg_box(player, 466, 220, AZ_UPG_ORION_BOOSTER);
}

/*===========================================================================*/

void az_paused_draw_screen(az_paused_state_t *state) {
  draw_minimap(state);
  glPushMatrix(); {
    glTranslated(0, 410 * (1 - state->drawer_openness), 0);
    draw_upgrades(state);
  } glPopMatrix();
  az_draw_cursor();
}

void az_tick_paused_state(az_paused_state_t *state, double time) {
  ++state->clock;
  const double drawer_time = 0.3;
  if (state->show_upgrades_drawer) {
    state->drawer_openness =
      fmin(1.0, state->drawer_openness + time / drawer_time);
  } else {
    state->drawer_openness =
      fmax(0.0, state->drawer_openness - time / drawer_time);
  }
}

/*===========================================================================*/
