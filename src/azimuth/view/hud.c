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

#include "azimuth/view/hud.h"

#include <assert.h>
#include <math.h>
#include <string.h>

#include <GL/gl.h>

#include "azimuth/constants.h"
#include "azimuth/state/dialog.h"
#include "azimuth/state/ship.h"
#include "azimuth/state/space.h"
#include "azimuth/state/upgrade.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/color.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h"
#include "azimuth/view/dialog.h"
#include "azimuth/view/node.h"
#include "azimuth/view/string.h"
#include "azimuth/view/util.h"

/*===========================================================================*/

static void draw_mbox(double left, double top, double width, double height) {
  glColor4f(0, 0, 0, 0.875);
  glBegin(GL_QUADS); {
    glVertex2d(left, top);
    glVertex2d(left + width, top);
    glVertex2d(left + width, top + height);
    glVertex2d(left, top + height);
  } glEnd();
  glColor3f(0, 0, 0);
  glBegin(GL_LINE_LOOP); {
    glVertex2d(left, top);
    glVertex2d(left + width, top);
    glVertex2d(left + width, top + height);
    glVertex2d(left, top + height);
  } glEnd();
}

static void draw_dbox(double left, double top, double width, double height) {
  glColor4f(0, 0, 0, 0.875);
  glBegin(GL_QUADS); {
    glVertex2d(left - 2, top - 2);
    glVertex2d(left + width + 2, top - 2);
    glVertex2d(left + width + 2, top + height + 2);
    glVertex2d(left - 2, top + height + 2);
  } glEnd();
  glColor3f(0, 0.5, 0);
  glBegin(GL_LINE_LOOP); {
    glVertex2d(left - 0.5, top - 0.5);
    glVertex2d(left + width + 0.5, top - 0.5);
    glVertex2d(left + width + 0.5, top + height + 0.5);
    glVertex2d(left - 0.5, top + height + 0.5);
  } glEnd();
  glBegin(GL_LINE_LOOP); {
    glVertex2d(left - 2.5, top - 2.5);
    glVertex2d(left + width + 2.5, top - 2.5);
    glVertex2d(left + width + 2.5, top + height + 2.5);
    glVertex2d(left - 2.5, top + height + 2.5);
  } glEnd();
}

static void tint_hud_rect(GLfloat width, GLfloat height) {
  glColor4f(0, 0, 0, 0.75);
  glBegin(GL_QUADS); {
    glVertex2f(0, 0);
    glVertex2f(0, height);
    glVertex2f(width, height);
    glVertex2f(width, 0);
  } glEnd();
}

static int count_linebreaks(const char *str) {
  int count = 0;
  for (const char *ch = str; *ch != '\0'; ++ch) {
    if (*ch == '\n') ++count;
  }
  return count;
}

/*===========================================================================*/

#define HUD_MARGIN 2
#define HUD_PADDING 4
#define HUD_BAR_HEIGHT 9
#define SHIELDS_ENERGY_HEIGHT (25 + 2 * HUD_PADDING)

static void draw_hud_bar(az_color_t inner, az_color_t outer,
                         float left, float top, float cur, float max) {
  // Draw bar:
  glBegin(GL_TRIANGLE_STRIP); {
    const float right = left + cur;
    const float middle = top + 0.45f * HUD_BAR_HEIGHT;
    const float bottom = top + HUD_BAR_HEIGHT;
    az_gl_color(outer); glVertex2f(left, top);    glVertex2f(right, top);
    az_gl_color(inner); glVertex2f(left, middle); glVertex2f(right, middle);
    az_gl_color(outer); glVertex2f(left, bottom); glVertex2f(right, bottom);
  } glEnd();
  // Draw outline:
  glColor3f(1, 1, 1); // white
  glBegin(GL_LINE_LOOP); {
    glVertex2f(left + 0.5f, top + 0.5f);
    glVertex2f(left + 0.5f, top + HUD_BAR_HEIGHT + 0.5f);
    glVertex2f(left + max + 0.5f, top + HUD_BAR_HEIGHT + 0.5f);
    glVertex2f(left + max + 0.5f, top + 0.5f);
  } glEnd();
}

static void draw_hud_shields_energy(const az_player_t *player,
                                    az_clock_t clock) {
  const int max_power = (player->max_shields > player->max_energy ?
                         player->max_shields : player->max_energy);
  glPushMatrix(); {
    glTranslatef(HUD_MARGIN, HUD_MARGIN, 0);
    tint_hud_rect(50 + 2 * HUD_PADDING + max_power, SHIELDS_ENERGY_HEIGHT);
    glTranslatef(HUD_PADDING, HUD_PADDING, 0);

    glColor3f(1, 1, 1); // white
    az_draw_string(8, AZ_ALIGN_LEFT, 0, 1, "SHIELD");
    az_draw_string(8, AZ_ALIGN_LEFT, 0, 16, "ENERGY");

    az_color_t inner = {0, 192, 192, 255};
    az_color_t outer = {0, 154, 154, 255};
    if (player->shields <= AZ_SHIELDS_VERY_LOW_THRESHOLD) {
      if (az_clock_mod(2, 3, clock)) {
        inner = outer = (az_color_t){255, 0, 0, 255};
      } else {
        inner = outer = (az_color_t){51, 0, 0, 255};
      }
    } else if (player->shields <= AZ_SHIELDS_LOW_THRESHOLD) {
      inner = (az_color_t){125 + 26 * az_clock_zigzag(6, 5, clock), 0, 0, 255};
      outer = (az_color_t){(4 * (int)inner.r) / 5, 0, 0, 255};
    }
    draw_hud_bar(inner, outer, 50, 0, player->shields, player->max_shields);
    draw_hud_bar((az_color_t){192, 0, 192, 255},
                 (az_color_t){154, 0, 154, 255},
                 50, 15, player->energy, player->max_energy);
  } glPopMatrix();
}

/*===========================================================================*/

void az_draw_minimap_room(const az_planet_t *planet, const az_room_t *room,
                          bool visited, bool blink) {
  const az_camera_bounds_t *bounds = &room->camera_bounds;
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
  const az_color_t zone_color = planet->zones[room->zone_key].color;
  if (blink) {
    glColor3f(0.75, 0.75, 0.75);
  } else if (!visited) {
    glColor3ub(zone_color.r / 4, zone_color.g / 4, zone_color.b / 4);
  } else glColor3ub(zone_color.r, zone_color.g, zone_color.b);
  if (bounds->theta_span >= 6.28) {
    glBegin(GL_POLYGON); {
      for (double theta = 0.0; theta < AZ_TWO_PI; theta += step) {
        glVertex2d(max_r * cos(theta), max_r * sin(theta));
      }
    } glEnd();
  } else {
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
  }

  // Draw outline:
  glColor3f(0.9, 0.9, 0.9); // white
  glBegin(GL_LINE_LOOP); {
    if (bounds->theta_span >= 6.28) {
      for (double theta = 0.0; theta < AZ_TWO_PI; theta += step) {
        glVertex2d(max_r * cos(theta), max_r * sin(theta));
      }
    } else {
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

#define MINIMAP_ZOOM 100.0
#define MINIMAP_WIDTH 44
#define MINIMAP_HEIGHT 44
#define MINIMAP_TOP 3
#define MINIMAP_LEFT (AZ_SCREEN_WIDTH - MINIMAP_WIDTH - 3)

static void draw_minimap_rooms(const az_space_state_t *state) {
  const az_planet_t *planet = state->planet;
  const az_ship_t *ship = &state->ship;
  const az_player_t *player = &ship->player;
  const double camera_radius =
    0.5 * MINIMAP_ZOOM * hypot(MINIMAP_WIDTH, MINIMAP_HEIGHT);
  const double camera_rho = az_vnorm(state->camera.center);
  const double camera_theta = az_vtheta(state->camera.center);
  const double camera_theta_span = (camera_rho <= camera_radius ? AZ_TWO_PI :
                                    2.0 * asin(camera_radius / camera_rho));
  // Draw planet surface:
  if (camera_rho + camera_radius >= AZ_PLANETOID_RADIUS &&
      camera_rho - camera_radius <= AZ_PLANETOID_RADIUS) {
    glBegin(GL_LINE_STRIP); {
      glColor3f(1, 1, 0);
      const double step = AZ_DEG2RAD(0.5);
      for (double theta = camera_theta - 0.5 * camera_theta_span,
                  limit = camera_theta + 0.5 * camera_theta_span + step;
           theta < limit; theta += step) {
        az_gl_vertex(az_vpolar(AZ_PLANETOID_RADIUS, theta));
      }
    } glEnd();
  }
  // Draw explored rooms:
  for (int i = 0; i < planet->num_rooms; ++i) {
    const az_room_t *room = &planet->rooms[i];

    // If the room isn't on our map yet, don't draw it.
    const bool visited = az_test_room_visited(player, i);
    const bool mapped = !(room->properties & AZ_ROOMF_UNMAPPED) &&
      az_test_zone_mapped(player, room->zone_key);
    if (!visited && !mapped) continue;
    const az_camera_bounds_t *bounds = &room->camera_bounds;

    // If the room is definitely not within the rectangle of the minimap view,
    // don't bother drawing it.
    const double min_r = bounds->min_r - AZ_SCREEN_HEIGHT/2;
    const double max_r = min_r + bounds->r_span + AZ_SCREEN_HEIGHT;
    if (min_r > camera_rho + MINIMAP_HEIGHT * MINIMAP_ZOOM ||
        max_r < camera_rho - MINIMAP_HEIGHT * MINIMAP_ZOOM) continue;
    const double extra_theta_span = camera_theta_span +
      (bounds->min_r <= AZ_SCREEN_RADIUS ? AZ_TWO_PI :
       2.0 * asin(AZ_SCREEN_RADIUS / bounds->min_r));
    if (az_mod2pi_nonneg(camera_theta -
                         (bounds->min_theta - 0.5 * extra_theta_span)) >
        bounds->theta_span + extra_theta_span) continue;

    az_draw_minimap_room(planet, room, visited, i == player->current_room &&
                         az_clock_mod(2, 15, state->clock));
  }
}

static void draw_minimap(const az_space_state_t *state) {
  draw_mbox(MINIMAP_LEFT - 0.5, MINIMAP_TOP - 0.5,
            MINIMAP_WIDTH + 1, MINIMAP_HEIGHT + 1);

  glEnable(GL_SCISSOR_TEST); {
    glScissor(MINIMAP_LEFT, AZ_SCREEN_HEIGHT - MINIMAP_TOP - MINIMAP_HEIGHT,
              MINIMAP_WIDTH, MINIMAP_HEIGHT);
    glPushMatrix(); {
      // Make positive Y be up instead of down.
      glScaled(1, -1, 1);
      // Put the center of the minimap on position (0, 0).
      glTranslated(MINIMAP_LEFT + MINIMAP_WIDTH/2,
                   -(MINIMAP_TOP + MINIMAP_HEIGHT/2), 0);
      // Zoom out.
      glScaled(1.0 / MINIMAP_ZOOM, 1.0 / MINIMAP_ZOOM, 1);
      // Move the screen to the camera pose.
      glTranslated(0, -az_vnorm(state->camera.center), 0);
      glRotated(90.0 - AZ_RAD2DEG(az_vtheta(state->camera.center)), 0, 0, 1);
      // Draw what the camera sees.
      draw_minimap_rooms(state);
    } glPopMatrix();
  } glDisable(GL_SCISSOR_TEST);
}

/*===========================================================================*/

#define WEAPONS_TOP HUD_MARGIN
#define WEAPONS_RIGHT (MINIMAP_LEFT - 5)

static void set_gun_color(az_gun_t gun) {
  assert(gun != AZ_GUN_NONE);
  switch (gun) {
    case AZ_GUN_NONE: AZ_ASSERT_UNREACHABLE();
    case AZ_GUN_CHARGE: glColor3f(1, 1, 1); break;
    case AZ_GUN_FREEZE: glColor3f(0, 1, 1); break;
    case AZ_GUN_TRIPLE: glColor3f(0, 1, 0); break;
    case AZ_GUN_HOMING: glColor3f(0, 0.375, 1); break;
    case AZ_GUN_PHASE:  glColor3f(1, 1, 0); break;
    case AZ_GUN_BURST:  glColor3f(0.75, 0.375, 0); break;
    case AZ_GUN_PIERCE: glColor3f(1, 0, 1); break;
    case AZ_GUN_BEAM:   glColor3f(1, 0, 0); break;
  }
}

static void draw_hud_gun_name(float left, float top, az_gun_t gun) {
  if (gun != AZ_GUN_NONE) {
    glColor3f(1, 1, 1);
    glBegin(GL_LINE_STRIP);
    glVertex2f(left + 3.5f, top + 0.5f);
    glVertex2f(left + 0.5f, top + 0.5f);
    glVertex2f(left + 0.5f, top + 10.5f);
    glVertex2f(left + 3.5f, top + 10.5f);
    glEnd();
    glBegin(GL_LINE_STRIP);
    glVertex2f(left + 51.5f, top + 0.5f);
    glVertex2f(left + 54.5f, top + 0.5f);
    glVertex2f(left + 54.5f, top + 10.5f);
    glVertex2f(left + 51.5f, top + 10.5f);
    glEnd();

    set_gun_color(gun);
    const char *name = az_gun_name(gun);
    az_draw_string(8, AZ_ALIGN_CENTER, left + 28, top + 2, name);
  }
}

static void draw_hud_ordnance(float left, float top, bool is_rockets,
                              int cur, int max, az_ordnance_t selected) {
  // Draw nothing if the player doesn't have this kind of ordnance yet.
  if (max <= 0) return;

  // Draw the selection indicator.
  if ((is_rockets && selected == AZ_ORDN_ROCKETS) ||
      (!is_rockets && selected == AZ_ORDN_BOMBS)) {
    glColor3f(1, 1, 1);
    glBegin(GL_LINE_STRIP);
    glVertex2f(left + 3.5f, top + 0.5f);
    glVertex2f(left + 0.5f, top + 0.5f);
    glVertex2f(left + 0.5f, top + 10.5f);
    glVertex2f(left + 3.5f, top + 10.5f);
    glEnd();
    glBegin(GL_LINE_STRIP);
    glVertex2f(left + 51.5f, top + 0.5f);
    glVertex2f(left + 54.5f, top + 0.5f);
    glVertex2f(left + 54.5f, top + 10.5f);
    glVertex2f(left + 51.5f, top + 10.5f);
    glEnd();
  }

  // Draw quantity string.
  if (cur >= max) glColor3f(1, 1, 0);
  else glColor3f(1, 1, 1);
  az_draw_printf(8, AZ_ALIGN_RIGHT, left + 32, top + 2, "%d", cur);

  // Draw icon.
  if (is_rockets) {
    glColor3f(1, 0.25, 0.25);
    az_draw_string(8, AZ_ALIGN_LEFT, left + 36, top + 2, "R");
  } else {
    glColor3f(0.25, 0.25, 1);
    az_draw_string(8, AZ_ALIGN_LEFT, left + 36, top + 2, "B");
  }
}

static void draw_hud_weapons_selection(const az_player_t *player) {
  const bool wide = player->max_rockets > 0 || player->max_bombs > 0;
  if (player->gun1 == AZ_GUN_NONE && !wide) return;
  const bool tall = player->gun2 != AZ_GUN_NONE || player->max_bombs > 0;

  const int height = (tall ? 25 : 10) + 2 * HUD_PADDING;
  const int width = (wide ? 115 : 55) + 2 * HUD_PADDING;
  glPushMatrix(); {
    glTranslatef(WEAPONS_RIGHT - width, WEAPONS_TOP, 0);
    tint_hud_rect(width, height);
    glTranslatef(HUD_PADDING, HUD_PADDING, 0);

    draw_hud_gun_name(0, 0, player->gun1);
    draw_hud_gun_name(0, 15, player->gun2);
    draw_hud_ordnance(60, 0, true, player->rockets, player->max_rockets,
                      player->ordnance);
    draw_hud_ordnance(60, 15, false, player->bombs, player->max_bombs,
                      player->ordnance);
  } glPopMatrix();
}

/*===========================================================================*/

static void draw_hud_speedrun_timer(const az_player_t *player) {
  const int total_seconds = (int)player->total_time;
  const int hours = total_seconds / 3600;
  const int minutes = (total_seconds % 3600) / 60;
  const int seconds = total_seconds % 60;
  const int tenths = ((int)(player->total_time * 10)) % 10;
  const int num_chars = 8 + snprintf(NULL, 0, "%d", hours);
  glPushMatrix(); {
    glTranslatef(HUD_MARGIN, HUD_MARGIN + SHIELDS_ENERGY_HEIGHT + 2, 0);
    tint_hud_rect(HUD_PADDING * 2 + 8 * num_chars, HUD_PADDING * 2 + 7);
    glColor3f(1, 1, 1);
    az_draw_printf(8, AZ_ALIGN_LEFT, HUD_PADDING, HUD_PADDING,
                   "%d:%02d:%02d.%01d", hours, minutes, seconds, tenths);
  } glPopMatrix();
}

/*===========================================================================*/

#define BOSS_BAR_WIDTH 400

static void draw_hud_boss_health(az_space_state_t *state) {
  az_baddie_t *baddie;
  if (az_lookup_baddie(state, state->boss_uid, &baddie)) {
    glPushMatrix(); {
      glTranslatef(HUD_MARGIN, AZ_SCREEN_HEIGHT - HUD_MARGIN -
                   2 * HUD_PADDING - 10, 0);
      tint_hud_rect(2 * HUD_PADDING + 35 + BOSS_BAR_WIDTH,
                    10 + 2 * HUD_PADDING);
      glTranslatef(HUD_PADDING, HUD_PADDING, 0);

      glColor3f(1, 1, 1); // white
      az_draw_string(8, AZ_ALIGN_LEFT, 0, 1, "BOSS");
      assert(baddie->health > 0.0);
      assert(baddie->data->max_health > 0.0);
      draw_hud_bar((az_color_t){255, 64, 0, 255},
                   (az_color_t){204, 51, 0, 255},
                   35, 0, (baddie->health / baddie->data->max_health) *
                   BOSS_BAR_WIDTH, BOSS_BAR_WIDTH);
    } glPopMatrix();
  }
}

static void draw_hud_message(const az_preferences_t *prefs,
                             const az_message_t *message) {
  if (message->time_remaining <= 0.0) {
    assert(message->paragraph == NULL);
    assert(message->num_lines == 0);
    return;
  }
  assert(message->paragraph != NULL);
  assert(message->num_lines > 0);
  glPushMatrix(); {
    const int width = AZ_SCREEN_WIDTH - 20;
    const int height = 6 + 20 * message->num_lines;
    const double slide_time = 0.5; // seconds
    const int top = AZ_SCREEN_HEIGHT -
      (message->time_remaining >= slide_time ? height :
       (int)((double)height * (message->time_remaining / slide_time)));
    glTranslatef((AZ_SCREEN_WIDTH - width) / 2, top, 0);

    glColor4f(0.1, 0.1, 0.1, 0.9); // dark gray tint
    glBegin(GL_QUADS); {
      glVertex2i(0, 0); glVertex2i(0, height);
      glVertex2i(width, height); glVertex2i(width, 0);
    } glEnd();

    az_draw_paragraph(16, AZ_ALIGN_CENTER, width/2, 6, 20, -1,
                      prefs, message->paragraph);
  } glPopMatrix();
}

static void draw_hud_countdown(const az_countdown_t *countdown,
                               az_clock_t clock) {
  if (!countdown->is_active) return;
  glPushMatrix(); {
    const int width = 2 * HUD_PADDING + 7 * 24 - 3;
    const int height = 2 * HUD_PADDING + 20;

    const int xstart = AZ_SCREEN_WIDTH/2 - width/2;
    const int ystart = AZ_SCREEN_HEIGHT/2 + 75 - height/2;
    const int xend = AZ_SCREEN_WIDTH - HUD_MARGIN - width;
    const int yend = AZ_SCREEN_HEIGHT - HUD_MARGIN - height;
    const double speed = 150.0; // pixels per second
    const double offset = fmax(0.0, countdown->active_for - 2.5) * speed;
    glTranslatef(az_imin(xend, xstart + offset),
                 az_imin(yend, ystart + offset), 0);
    tint_hud_rect(width, height);

    assert(countdown->time_remaining >= 0.0);
    if (countdown->time_remaining > AZ_COUNTDOWN_TIME_REMAINING_LOW) {
      glColor3f(1, 1, 1); // white
    } else {
      if (az_clock_mod(2, 3, clock) == 0) glColor3f(1, 1, 0); // yellow
      else glColor3f(1, 0, 0); // red
    }
    const int minutes = az_imin(9, ((int)countdown->time_remaining) / 60);
    const int seconds = ((int)countdown->time_remaining) % 60;
    const int jiffies = ((int)(countdown->time_remaining * 100)) % 100;
    az_draw_printf(24, AZ_ALIGN_LEFT, HUD_PADDING, HUD_PADDING,
                   "%d:%02d:%02d", minutes, seconds, jiffies);
  } glPopMatrix();
}

/*===========================================================================*/

#define DIALOG_TEXT_LINE_SPACING 12
#define DIALOG_BOX_WIDTH 404
#define DIALOG_BOX_HEIGHT 116
#define DIALOG_BOX_MARGIN 10
#define PORTRAIT_BOX_WIDTH 150
#define PORTRAIT_BOX_HEIGHT 150
#define PORTRAIT_BOX_MARGIN 5
#define DIALOG_HORZ_SPACING 20
#define DIALOG_VERT_SPACING 50

static void draw_dialogue_frames(double openness) {
  assert(openness >= 0.0);
  assert(openness <= 1.0);
  { // Portraits:
    const double sw = 0.5 * PORTRAIT_BOX_WIDTH * openness;
    const double sh = 0.5 * PORTRAIT_BOX_HEIGHT * openness;
    // Top portrait:
    const int tcx =
      (AZ_SCREEN_WIDTH - DIALOG_HORZ_SPACING - DIALOG_BOX_WIDTH) / 2;
    const int tcy =
      (AZ_SCREEN_HEIGHT - DIALOG_VERT_SPACING - PORTRAIT_BOX_HEIGHT) / 2;
    draw_dbox(tcx - sw, tcy - sh, sw * 2, sh * 2);
    // Bottom portrait:
    const int bcx =
      (AZ_SCREEN_WIDTH + DIALOG_HORZ_SPACING + DIALOG_BOX_WIDTH) / 2;
    const int bcy =
      (AZ_SCREEN_HEIGHT + DIALOG_VERT_SPACING + PORTRAIT_BOX_HEIGHT) / 2;
    draw_dbox(bcx - sw, bcy - sh, sw * 2, sh * 2);
  }
  { // Dialogue boxes:
    const double sw = 0.5 * DIALOG_BOX_WIDTH * openness;
    const double sh = 0.5 * DIALOG_BOX_HEIGHT * openness;
    // Top dialogue box:
    const int tcx =
      (AZ_SCREEN_WIDTH + DIALOG_HORZ_SPACING + PORTRAIT_BOX_WIDTH) / 2;
    const int tcy =
      (AZ_SCREEN_HEIGHT - DIALOG_VERT_SPACING - DIALOG_BOX_HEIGHT) / 2;
    draw_dbox(tcx - sw, tcy - sh, sw * 2, sh * 2);
    // Bottom dialogue box:
    const int bcx =
      (AZ_SCREEN_WIDTH - DIALOG_HORZ_SPACING - PORTRAIT_BOX_WIDTH) / 2;
    const int bcy =
      (AZ_SCREEN_HEIGHT + DIALOG_VERT_SPACING + DIALOG_BOX_HEIGHT) / 2;
    draw_dbox(bcx - sw, bcy - sh, sw * 2, sh * 2);
  }
}

static void draw_dialogue_text(const az_space_state_t *state) {
  const az_dialogue_state_t *dialogue = &state->dialogue;
  assert(dialogue->step != AZ_DLS_INACTIVE);
  assert(dialogue->paragraph != NULL);
  glPushMatrix(); {
    if (dialogue->top_turn) {
      glTranslatef((AZ_SCREEN_WIDTH + DIALOG_HORZ_SPACING - DIALOG_BOX_WIDTH +
                    PORTRAIT_BOX_WIDTH) / 2 + DIALOG_BOX_MARGIN,
                   (AZ_SCREEN_HEIGHT - DIALOG_VERT_SPACING) / 2 -
                   DIALOG_BOX_HEIGHT + DIALOG_BOX_MARGIN, 0);
    } else {
      glTranslatef((AZ_SCREEN_WIDTH - DIALOG_HORZ_SPACING - DIALOG_BOX_WIDTH -
                    PORTRAIT_BOX_WIDTH) / 2 + DIALOG_BOX_MARGIN,
                   (AZ_SCREEN_HEIGHT + DIALOG_VERT_SPACING) / 2 +
                   DIALOG_BOX_MARGIN, 0);
    }

    if (dialogue->step == AZ_DLS_WAIT) {
      if (az_clock_mod(2, 15, state->clock)) glColor4f(0.5, 0.5, 0.5, 0.5);
      else glColor4f(0.25, 0.75, 0.75, 0.5);
      az_draw_string(8, AZ_ALIGN_RIGHT,
                     DIALOG_BOX_WIDTH - 2 * DIALOG_BOX_MARGIN,
                     DIALOG_BOX_HEIGHT - 2 * DIALOG_BOX_MARGIN - 8, "[ENTER]");
    }

    az_draw_paragraph(8, AZ_ALIGN_LEFT, 0, 0, DIALOG_TEXT_LINE_SPACING,
                      dialogue->chars_to_print, state->prefs,
                      dialogue->paragraph);
  } glPopMatrix();
}

static void draw_dialogue_portrait(az_portrait_t portrait, bool is_bottom,
                                   bool talking, az_clock_t clock) {
  if (portrait == AZ_POR_NOTHING) return;
  glPushMatrix(); {
    const GLfloat x_scale =
      (PORTRAIT_BOX_WIDTH - 2 * PORTRAIT_BOX_MARGIN) / 100.0;
    const GLfloat y_scale =
      (PORTRAIT_BOX_HEIGHT - 2 * PORTRAIT_BOX_MARGIN) / 100.0;
    if (!is_bottom) {
      glTranslatef((AZ_SCREEN_WIDTH - DIALOG_HORZ_SPACING - DIALOG_BOX_WIDTH -
                    PORTRAIT_BOX_WIDTH) / 2 + PORTRAIT_BOX_MARGIN,
                   (AZ_SCREEN_HEIGHT - DIALOG_VERT_SPACING) / 2 -
                   PORTRAIT_BOX_MARGIN, 0);
      glScalef(x_scale, -y_scale, 1);
    } else {
      glTranslatef((AZ_SCREEN_WIDTH + DIALOG_HORZ_SPACING + DIALOG_BOX_WIDTH -
                    PORTRAIT_BOX_WIDTH) / 2 + PORTRAIT_BOX_WIDTH -
                   PORTRAIT_BOX_MARGIN,
                   (AZ_SCREEN_HEIGHT + DIALOG_VERT_SPACING) / 2 +
                   PORTRAIT_BOX_HEIGHT - PORTRAIT_BOX_MARGIN, 0);
      glScalef(-x_scale, -y_scale, 1);
    }
    az_draw_portrait(portrait, talking, clock);
  } glPopMatrix();
}

void az_draw_dialogue(const az_space_state_t *state) {
  const az_dialogue_state_t *dialogue = &state->dialogue;
  if (dialogue->hidden) return;
  bool talking = false;
  switch (dialogue->step) {
    case AZ_DLS_INACTIVE: break;
    case AZ_DLS_BEGIN:
      draw_dialogue_frames(dialogue->progress);
      break;
    case AZ_DLS_TALK:
      talking = true;
      // fallthrough
    case AZ_DLS_WAIT:
    case AZ_DLS_BLANK:
      draw_dialogue_frames(1.0);
      if (dialogue->paragraph != NULL) draw_dialogue_text(state);
      draw_dialogue_portrait(dialogue->top, false,
                             (talking && dialogue->top_turn), state->clock);
      draw_dialogue_portrait(dialogue->bottom, true,
                             (talking && !dialogue->top_turn), state->clock);
      break;
    case AZ_DLS_END:
      draw_dialogue_frames(1.0 - dialogue->progress);
      break;
  }
}

/*===========================================================================*/

#define MONOLOGUE_TEXT_LINE_SPACING 24
#define MONOLOGUE_BOX_WIDTH 628
#define MONOLOGUE_BOX_HEIGHT 404
#define MONOLOGUE_BOX_PADDING 10

void az_draw_monologue(const az_space_state_t *state) {
  if (state->monologue.step == AZ_MLS_INACTIVE) return;
  const int left = (AZ_SCREEN_WIDTH - MONOLOGUE_BOX_WIDTH) / 2;
  const int top = (AZ_SCREEN_HEIGHT - MONOLOGUE_BOX_HEIGHT) / 2;
  double tint = 1.0;
  if (state->monologue.step == AZ_MLS_BEGIN) {
    tint = state->monologue.progress;
  } else if (state->monologue.step == AZ_MLS_END) {
    tint = 1.0 - state->monologue.progress;
  }
  glBegin(GL_TRIANGLE_STRIP); {
    glColor4f(0, 0, 0, 0.5 * tint);
    glVertex2i(left, top);
    glVertex2i(left + MONOLOGUE_BOX_WIDTH, top);
    glVertex2i(left, top + MONOLOGUE_BOX_HEIGHT);
    glVertex2i(left + MONOLOGUE_BOX_WIDTH, top + MONOLOGUE_BOX_HEIGHT);
  } glEnd();
  if (state->monologue.paragraph != NULL) {
    az_draw_paragraph(16, AZ_ALIGN_LEFT, left + MONOLOGUE_BOX_PADDING,
                      top + MONOLOGUE_BOX_PADDING, MONOLOGUE_TEXT_LINE_SPACING,
                      state->monologue.chars_to_print, state->prefs,
                      state->monologue.paragraph);
  }
  if (state->monologue.step == AZ_MLS_WAIT) {
    if (az_clock_mod(2, 15, state->clock)) glColor4f(0.5, 0.5, 0.5, 0.5);
    else glColor4f(0.25, 0.75, 0.75, 0.5);
    az_draw_string(16, AZ_ALIGN_RIGHT,
                   left + MONOLOGUE_BOX_WIDTH - MONOLOGUE_BOX_PADDING,
                   top + MONOLOGUE_BOX_HEIGHT - MONOLOGUE_BOX_PADDING - 16,
                   "[ENTER]");
  }
}

/*===========================================================================*/

#define UPGRADE_BOX_WIDTH 500
#define UPGRADE_BOX_BASE_HEIGHT 110
#define UPGRADE_BOX_EXTRA_HEIGHT_PER_LINEBREAK 20

static void draw_upgrade_box_frame(double openness, double max_height) {
  assert(openness >= 0.0);
  assert(openness <= 1.0);
  const double width = sqrt(openness) * UPGRADE_BOX_WIDTH;
  const double height = openness * openness * max_height;
  const double left = 0.5 * (AZ_SCREEN_WIDTH - width);
  const double top = 0.5 * (AZ_SCREEN_HEIGHT - height);
  draw_dbox(left, top, width, height);
}

static void draw_upgrade_box_message(
    const az_preferences_t *prefs, const az_player_t *player,
    az_upgrade_t upgrade, double height, az_clock_t clock) {
  const char *name = az_upgrade_name(upgrade);
  const char *description = az_upgrade_description(upgrade, &player->upgrades);
  const double top = 0.5 * (AZ_SCREEN_HEIGHT - height);
  glPushMatrix(); {
    glTranslated(AZ_SCREEN_WIDTH/2 + 0.5, top + 58.5, 0);
    glScalef(1, -1, 1);
    az_draw_upgrade_icon(upgrade, clock);
  } glPopMatrix();
  glColor3f(1, 1, 1); // white
  az_draw_string(16, AZ_ALIGN_CENTER, AZ_SCREEN_WIDTH/2, top + 18, name);
  az_draw_paragraph(8, AZ_ALIGN_CENTER, AZ_SCREEN_WIDTH/2, top + 86, 20, -1,
                    prefs, description);
}

static void draw_upgrade_box(const az_space_state_t *state) {
  assert(state->mode == AZ_MODE_UPGRADE);
  const az_upgrade_mode_data_t *mode_data = &state->upgrade_mode;
  const double height = UPGRADE_BOX_BASE_HEIGHT +
    UPGRADE_BOX_EXTRA_HEIGHT_PER_LINEBREAK *
    count_linebreaks(az_upgrade_description(mode_data->upgrade,
                                            &state->ship.player.upgrades));
  switch (mode_data->step) {
    case AZ_UGS_OPEN:
      draw_upgrade_box_frame(mode_data->progress, height);
      break;
    case AZ_UGS_MESSAGE:
      draw_upgrade_box_frame(1.0, height);
      draw_upgrade_box_message(state->prefs, &state->ship.player,
                               mode_data->upgrade, height, state->clock);
      break;
    case AZ_UGS_CLOSE:
      draw_upgrade_box_frame(1.0 - mode_data->progress, height);
      break;
  }
}

/*===========================================================================*/

void az_draw_hud(az_space_state_t *state) {
  const az_ship_t *ship = &state->ship;
  if (!az_ship_is_alive(ship)) return;
  if (!ship->autopilot.enabled) {
    draw_minimap(state);
    const az_player_t *player = &ship->player;
    draw_hud_shields_energy(player, state->clock);
    draw_hud_weapons_selection(player);
    if (state->prefs->speedrun_timer) draw_hud_speedrun_timer(player);
  }
  draw_hud_boss_health(state);
  draw_hud_message(state->prefs, &state->message);
  draw_hud_countdown(&state->countdown, state->clock);
  az_draw_dialogue(state);
  az_draw_monologue(state);
  if (state->mode == AZ_MODE_UPGRADE) draw_upgrade_box(state);
}

/*===========================================================================*/
