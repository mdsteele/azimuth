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
#include <math.h> // for sqrt

#include <GL/gl.h>

#include "azimuth/constants.h"
#include "azimuth/state/ship.h"
#include "azimuth/state/space.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/vector.h"
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
    az_draw_string(8, AZ_ALIGN_LEFT, 0, 1, "SHIELD");
    az_draw_string(8, AZ_ALIGN_LEFT, 0, 16, "ENERGY");

    glColor3f(0, 0.75, 0.75); // cyan
    draw_hud_bar(50, 0, player->shields, player->max_shields);
    glColor3f(0.75, 0, 0.75); // magenta
    draw_hud_bar(50, 15, player->energy, player->max_energy);
  } glPopMatrix();
}

/*===========================================================================*/

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
    const char *name = az_gun_name(gun);
    az_draw_string(8, AZ_ALIGN_CENTER, left + 28, top + 2, name);
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
  az_draw_printf(8, AZ_ALIGN_RIGHT, left + 32, top + 2, "%d", cur);

  // Draw icon.
  if (is_rockets) {
    glColor3f(1, 1, 1);
    az_draw_string(8, AZ_ALIGN_LEFT, left + 36, top + 2, "R");
  } else {
    glColor3f(1, 1, 1);
    az_draw_string(8, AZ_ALIGN_LEFT, left + 36, top + 2, "B");
  }
}

static void draw_hud_weapons_selection(const az_player_t *player) {
  const int height = 25 + 2 * HUD_PADDING;
  const int width = 115 + 2 * HUD_PADDING;
  glPushMatrix(); {
    glTranslated(AZ_SCREEN_WIDTH - HUD_MARGIN - width, HUD_MARGIN, 0);

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

/*===========================================================================*/

static void draw_hud_message(const az_message_t *message) {
  if (message->time_remaining <= 0.0) return;
  glPushMatrix(); {
    const int width = 2 * HUD_PADDING + message->length * 8;
    const int height = 2 * HUD_PADDING + 8;
    glTranslated(HUD_MARGIN, AZ_SCREEN_HEIGHT - HUD_MARGIN - height, 0);
    const double fade_time = 0.8; // seconds
    const double alpha = (message->time_remaining >= fade_time ? 1.0 :
                          message->time_remaining / fade_time);

    glColor4f(0, 0, 0, 0.75 * alpha); // tinted-black
    glBegin(GL_QUADS); {
      glVertex2i(0, 0);
      glVertex2i(0, height);
      glVertex2i(width, height);
      glVertex2i(width, 0);
    } glEnd();

    glColor4f(1, 1, 1, alpha); // white
    az_draw_chars(8, AZ_ALIGN_LEFT, HUD_PADDING, HUD_PADDING,
                  message->string, message->length);
  } glPopMatrix();
}

static void draw_hud_timer(const az_timer_t *timer, az_clock_t clock) {
  if (!timer->is_active) return;
  glPushMatrix(); {
    const int width = 2 * HUD_PADDING + 7 * 24;
    const int height = 2 * HUD_PADDING + 24;

    const int xstart = AZ_SCREEN_WIDTH/2 - width/2;
    const int ystart = AZ_SCREEN_HEIGHT/2 + 75 - height/2;
    const int xend = AZ_SCREEN_WIDTH - HUD_MARGIN - width;
    const int yend = AZ_SCREEN_HEIGHT - HUD_MARGIN - height;
    const double speed = 150.0; // pixels per second
    const double offset = az_dmax(0.0, timer->active_for - 2.5) * speed;
    glTranslated(az_imin(xend, xstart + offset),
                 az_imin(yend, ystart + offset), 0);

    glColor4f(0, 0, 0, 0.75); // tinted-black
    glBegin(GL_QUADS); {
      glVertex2i(0, 0);
      glVertex2i(0, height);
      glVertex2i(width, height);
      glVertex2i(width, 0);
    } glEnd();

    assert(timer->time_remaining >= 0.0);
    if (timer->time_remaining >= 10.0) {
      glColor3f(1, 1, 1); // white
    } else {
      if (az_clock_mod(2, 3, clock) == 0) glColor3f(1, 1, 0); // yellow
      else glColor3f(1, 0, 0); // red
    }
    const int minutes = az_imin(9, ((int)timer->time_remaining) / 60);
    const int seconds = ((int)timer->time_remaining) % 60;
    const int jiffies = ((int)(timer->time_remaining * 100)) % 100;
    az_draw_printf(24, AZ_ALIGN_LEFT, HUD_PADDING, HUD_PADDING,
                   "%d:%02d:%02d", minutes, seconds, jiffies);
  } glPopMatrix();
}

/*===========================================================================*/

#define UPGRADE_BOX_WIDTH 500
#define UPGRADE_BOX_HEIGHT 94

static void draw_upgrade_box_frame(double openness) {
  assert(openness >= 0.0);
  assert(openness <= 1.0);
  const double width = sqrt(openness) * UPGRADE_BOX_WIDTH;
  const double height = openness * openness * UPGRADE_BOX_HEIGHT;
  const double left = 0.5 * (AZ_SCREEN_WIDTH - width);
  const double top = 0.5 * (AZ_SCREEN_HEIGHT - height);
  glColor4f(0, 0, 0, 0.875); // black tint
  glBegin(GL_QUADS); {
    glVertex2d(left, top);
    glVertex2d(left + width, top);
    glVertex2d(left + width, top + height);
    glVertex2d(left, top + height);
  } glEnd();
  glColor3f(0, 1, 0); // green
  glBegin(GL_LINE_LOOP); {
    glVertex2d(left, top);
    glVertex2d(left + width, top);
    glVertex2d(left + width, top + height);
    glVertex2d(left, top + height);
  } glEnd();
}

static void draw_upgrade_box_message(az_upgrade_t upgrade) {
  const char *name = NULL, *line1 = NULL, *line2 = NULL;
  switch (upgrade) {
    case AZ_UPG_GUN_CHARGE:
      name = "CHARGE GUN";
      line1 = "Hold [V] to charge, release to fire.";
      break;
    case AZ_UPG_ROCKET_AMMO_00:
    case AZ_UPG_ROCKET_AMMO_01:
    case AZ_UPG_ROCKET_AMMO_02:
    case AZ_UPG_ROCKET_AMMO_03:
    case AZ_UPG_ROCKET_AMMO_04:
    case AZ_UPG_ROCKET_AMMO_05:
    case AZ_UPG_ROCKET_AMMO_06:
    case AZ_UPG_ROCKET_AMMO_07:
    case AZ_UPG_ROCKET_AMMO_08:
    case AZ_UPG_ROCKET_AMMO_09:
    case AZ_UPG_ROCKET_AMMO_10:
    case AZ_UPG_ROCKET_AMMO_11:
    case AZ_UPG_ROCKET_AMMO_12:
    case AZ_UPG_ROCKET_AMMO_13:
    case AZ_UPG_ROCKET_AMMO_14:
    case AZ_UPG_ROCKET_AMMO_15:
    case AZ_UPG_ROCKET_AMMO_16:
    case AZ_UPG_ROCKET_AMMO_17:
    case AZ_UPG_ROCKET_AMMO_18:
    case AZ_UPG_ROCKET_AMMO_19:
      name = "ROCKETS";
      line1 = "Maximum rockets increased by 5.";
      line2 = "Press [9] to select, hold [C] and press [V] to fire.";
      break;
    case AZ_UPG_REACTIVE_ARMOR:
      name = "REACTIVE ARMOR";
      line1 = "All damage taken is reduced by half.";
      line2 = "Colliding with enemies will now damage them.";
      break;
    default:
      name = "?????";
      line1 = "TODO describe other upgrades";
      break;
  }
  assert(name != NULL);
  assert(line1 != NULL);
  const double top = 0.5 * (AZ_SCREEN_HEIGHT - UPGRADE_BOX_HEIGHT);
  glColor3f(1, 1, 1); // white
  az_draw_string(16, AZ_ALIGN_CENTER, AZ_SCREEN_WIDTH/2, top + 18, name);
  az_draw_string(8, AZ_ALIGN_CENTER, AZ_SCREEN_WIDTH/2, top + 48, line1);
  if (line2 != NULL) {
    az_draw_string(8, AZ_ALIGN_CENTER, AZ_SCREEN_WIDTH/2, top + 68, line2);
  }
}

static void draw_upgrade_box(const az_space_state_t *state) {
  assert(state->mode == AZ_MODE_UPGRADE);
  switch (state->mode_data.upgrade.step) {
    case AZ_UGS_OPEN:
      draw_upgrade_box_frame(state->mode_data.upgrade.progress);
      break;
    case AZ_UGS_MESSAGE:
      draw_upgrade_box_frame(1.0);
      draw_upgrade_box_message(state->mode_data.upgrade.upgrade);
      break;
    case AZ_UGS_CLOSE:
      draw_upgrade_box_frame(1.0 - state->mode_data.upgrade.progress);
      break;
  }
}

/*===========================================================================*/

void az_draw_hud(const az_space_state_t *state) {
  const az_ship_t *ship = &state->ship;
  if (!az_ship_is_present(ship)) return;
  const az_player_t *player = &ship->player;
  draw_hud_shields_energy(player);
  draw_hud_weapons_selection(player);
  // TODO: draw boss health bar, when relevant
  draw_hud_message(&state->message);
  draw_hud_timer(&state->timer, state->clock);
  if (state->mode == AZ_MODE_UPGRADE) {
    draw_upgrade_box(state);
  }
}

/*===========================================================================*/
