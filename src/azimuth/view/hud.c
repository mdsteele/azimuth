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
#include <stdio.h> // for sprintf
#include <string.h> // for strlen

#include <OpenGL/gl.h>

#include "azimuth/screen.h"
#include "azimuth/state/space.h"
#include "azimuth/vector.h"
#include "azimuth/view/string.h"

/*===========================================================================*/

#define HUD_MARGIN 5
#define HUD_PADDING 5
#define HUD_BAR_HEIGHT 9

static void draw_hud_bar(int left, int top, double cur, double max) {
  // Draw bar:
  glBegin(GL_QUADS);
  glVertex2d(left, top);
  glVertex2d(left, top + HUD_BAR_HEIGHT);
  glVertex2d(left + cur, top + HUD_BAR_HEIGHT);
  glVertex2d(left + cur, top);
  glEnd();
  // Draw outline:
  glColor3f(1, 1, 1); // white
  glBegin(GL_LINE_LOOP);
  glVertex2f(left + 0.5, top + 0.5);
  glVertex2f(left + 0.5, top + HUD_BAR_HEIGHT + 0.5);
  glVertex2f(left + max + 0.5, top + HUD_BAR_HEIGHT + 0.5);
  glVertex2f(left + max + 0.5, top + 0.5);
  glEnd();
}

static void draw_hud_shields_energy(const az_player_t *player) {
  const int max_power = (player->max_shields > player->max_energy ?
                         player->max_shields : player->max_energy);
  const int height = 25 + 2 * HUD_PADDING;
  const int width = 50 + 2 * HUD_PADDING + max_power;

  glPushMatrix(); {
    glTranslated(HUD_MARGIN, HUD_MARGIN, 0);

    glColor4f(0, 0, 0, 0.75); // tinted-black
    glBegin(GL_QUADS);
    glVertex2i(0, 0);
    glVertex2i(0, height);
    glVertex2i(width, height);
    glVertex2i(width, 0);
    glEnd();

    glTranslated(HUD_PADDING, HUD_PADDING, 0);

    glColor3f(1, 1, 1); // white
    az_draw_string((az_vector_t){0,1}, 8, "SHIELD");
    az_draw_string((az_vector_t){0,16}, 8, "ENERGY");

    glColor3f(0, 0.75, 0.75); // cyan
    draw_hud_bar(50, 0, player->shields, player->max_shields);
    glColor3f(0.75, 0, 0.75); // magenta
    draw_hud_bar(50, 15, player->energy, player->max_energy);
  } glPopMatrix();
}

static const char *gun_name(az_gun_t gun) {
  switch (gun) {
    case AZ_GUN_CHARGE: return "CHARGE";
    case AZ_GUN_FREEZE: return "FREEZE";
    case AZ_GUN_TRIPLE: return "TRIPLE";
    case AZ_GUN_HOMING: return "HOMING";
    case AZ_GUN_BEAM:   return "BEAM";
    case AZ_GUN_PHASE:  return "PHASE";
    case AZ_GUN_BURST:  return "BURST";
    case AZ_GUN_PIERCE: return "PIERCE";
    default: assert(false);
  }
  return "XXXXXX";
}

static void set_gun_color(az_gun_t gun) {
  switch (gun) {
    case AZ_GUN_CHARGE: glColor3f(1, 1, 1); break;
    case AZ_GUN_FREEZE: glColor3f(0, 1, 1); break;
    case AZ_GUN_TRIPLE: glColor3f(0, 1, 0); break;
    case AZ_GUN_HOMING: glColor3f(0, 0, 1); break;
    case AZ_GUN_BEAM:   glColor3f(1, 0, 0); break;
    case AZ_GUN_PHASE:  glColor3f(1, 1, 0); break;
    case AZ_GUN_BURST:  glColor3f(0.5, 0.5, 0.5); break;
    case AZ_GUN_PIERCE: glColor3f(1, 0, 1); break;
    default: assert(false);
  }
}

static void draw_hud_gun_name(int left, int top, az_gun_t gun) {
  if (gun != AZ_GUN_NONE) {
    glColor3f(1, 1, 1);
    glBegin(GL_LINE_STRIP);
    glVertex2d(left + 3.5, top + 0.5);
    glVertex2d(left + 0.5, top + 0.5);
    glVertex2d(left + 0.5, top + 10.5);
    glVertex2d(left + 3.5, top + 10.5);
    glEnd();
    glBegin(GL_LINE_STRIP);
    glVertex2d(left + 51.5, top + 0.5);
    glVertex2d(left + 54.5, top + 0.5);
    glVertex2d(left + 54.5, top + 10.5);
    glVertex2d(left + 51.5, top + 10.5);
    glEnd();

    set_gun_color(gun);
    const char *name = gun_name(gun);
    az_draw_string((az_vector_t){left + 28 - strlen(name) * 4, top + 2},
                   8, name);
  }
}

static void draw_hud_ordnance(int left, int top, bool is_rockets,
                              int cur, int max, az_ordnance_t selected) {
  // Draw nothing if the player doesn't have this kind of ordnance yet.
  if (max <= 0) return;

  // Draw the selection indicator.
  if ((is_rockets && selected == AZ_ORDN_ROCKETS) ||
      (!is_rockets && selected == AZ_ORDN_BOMBS)) {
    glColor3f(1, 1, 1);
    glBegin(GL_LINE_STRIP);
    glVertex2d(left + 3.5, top + 0.5);
    glVertex2d(left + 0.5, top + 0.5);
    glVertex2d(left + 0.5, top + 10.5);
    glVertex2d(left + 3.5, top + 10.5);
    glEnd();
    glBegin(GL_LINE_STRIP);
    glVertex2d(left + 51.5, top + 0.5);
    glVertex2d(left + 54.5, top + 0.5);
    glVertex2d(left + 54.5, top + 10.5);
    glVertex2d(left + 51.5, top + 10.5);
    glEnd();
  }

  // Draw quantity string.
  if (cur >= max) glColor3f(1, 1, 0);
  else glColor3f(1, 1, 1);
  char buffer[4];
  const int len = sprintf(buffer, "%d", az_imin(999, cur));
  az_draw_chars((az_vector_t){left + 32 - 8 * len, top + 2}, 8, buffer, len);

  // Draw icon.
  if (is_rockets) {
    glColor3f(1, 1, 1);
    az_draw_string((az_vector_t){left + 36, top + 2}, 8, "R");
  } else {
    glColor3f(1, 1, 1);
    az_draw_string((az_vector_t){left + 36, top + 2}, 8, "B");
  }
}

static void draw_hud_weapons_selection(const az_player_t *player) {
  const int height = 25 + 2 * HUD_PADDING;
  const int width = 115 + 2 * HUD_PADDING;
  glPushMatrix(); {
    glTranslated(SCREEN_WIDTH - HUD_MARGIN - width, HUD_MARGIN, 0);

    glColor4f(0, 0, 0, 0.75); // tinted-black
    glBegin(GL_QUADS);
    glVertex2i(0, 0);
    glVertex2i(0, height);
    glVertex2i(width, height);
    glVertex2i(width, 0);
    glEnd();

    glTranslated(HUD_PADDING, HUD_PADDING, 0);

    draw_hud_gun_name(0, 0, player->gun1);
    draw_hud_gun_name(0, 15, player->gun2);
    draw_hud_ordnance(60, 0, true, player->rockets, player->max_rockets,
                      player->ordnance);
    draw_hud_ordnance(60, 15, false, player->bombs, player->max_bombs,
                      player->ordnance);
  } glPopMatrix();
}

static void draw_hud_timer(const az_timer_t *timer, unsigned long clock) {
  if (timer->active_for < 0) return;
  glPushMatrix(); {
    const int width = 2 * HUD_PADDING + 7 * 24;
    const int height = 2 * HUD_PADDING + 24;

    const int xstart = SCREEN_WIDTH/2 - width/2;
    const int ystart = SCREEN_HEIGHT/2 + 75 - height/2;
    const int xend = SCREEN_WIDTH - HUD_MARGIN - width;
    const int yend = SCREEN_HEIGHT - HUD_MARGIN - height;
    const double speed = 150.0; // pixels per second
    const double offset = az_dmax(0.0, timer->active_for - 2.5) * speed;
    glTranslated(az_imin(xend, xstart + offset),
                 az_imin(yend, ystart + offset), 0);

    glColor4f(0, 0, 0, 0.75); // tinted-black
    glBegin(GL_QUADS);
    glVertex2i(0, 0);
    glVertex2i(0, height);
    glVertex2i(width, height);
    glVertex2i(width, 0);
    glEnd();

    assert(timer->time_remaining >= 0.0);
    if (timer->time_remaining >= 10.0) {
      glColor3f(1, 1, 1); // white
    } else {
      if (clock % 6 < 3) glColor3f(1, 1, 0); // yellow
      else glColor3f(1, 0, 0); // red
    }
    char buffer[8];
    const int minutes = az_imin(9, ((int)timer->time_remaining) / 60);
    const int seconds = ((int)timer->time_remaining) % 60;
    const int jiffies = ((int)(timer->time_remaining * 100)) % 100;
    const int len = sprintf(buffer, "%d:%02d:%02d", minutes, seconds, jiffies);
    az_draw_chars((az_vector_t){HUD_PADDING, HUD_PADDING}, 24, buffer, len);
  } glPopMatrix();
}

void az_draw_hud(const az_space_state_t *state) {
  const az_player_t *player = &state->ship.player;
  draw_hud_shields_energy(player);
  draw_hud_weapons_selection(player);
  // TODO: draw boss health bar, when relevant
  draw_hud_timer(&state->timer, state->clock);
}

/*===========================================================================*/
