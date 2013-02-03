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

#include <GL/gl.h>

#include "azimuth/constants.h"
#include "azimuth/state/dialog.h"
#include "azimuth/state/ship.h"
#include "azimuth/state/space.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h"
#include "azimuth/view/string.h"

/*===========================================================================*/

#define HUD_MARGIN 2
#define HUD_PADDING 4
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

static void draw_hud_shields_energy(const az_player_t *player,
                                    az_clock_t clock) {
  const int max_power = (player->max_shields > player->max_energy ?
                         player->max_shields : player->max_energy);
  const int height = 25 + 2 * HUD_PADDING;
  const int width = 50 + 2 * HUD_PADDING + max_power;

  glPushMatrix(); {
    glTranslatef(HUD_MARGIN, HUD_MARGIN, 0);

    glColor4f(0, 0, 0, 0.75); // tinted-black
    glBegin(GL_QUADS);
    glVertex2i(0, 0);
    glVertex2i(0, height);
    glVertex2i(width, height);
    glVertex2i(width, 0);
    glEnd();

    glTranslatef(HUD_PADDING, HUD_PADDING, 0);

    glColor3f(1, 1, 1); // white
    az_draw_string(8, AZ_ALIGN_LEFT, 0, 1, "SHIELD");
    az_draw_string(8, AZ_ALIGN_LEFT, 0, 16, "ENERGY");

    if (player->shields <= AZ_SHIELDS_VERY_LOW_THRESHOLD) {
      if (az_clock_mod(2, 3, clock)) glColor3f(1, 0, 0);
      else glColor3f(0.2, 0, 0);
    } else if (player->shields <= AZ_SHIELDS_LOW_THRESHOLD) {
      glColor3f(0.5 + 0.1 * az_clock_zigzag(6, 5, clock), 0, 0);
    } else glColor3f(0, 0.75, 0.75); // cyan
    draw_hud_bar(50, 0, player->shields, player->max_shields);
    glColor3f(0.75, 0, 0.75); // magenta
    draw_hud_bar(50, 15, player->energy, player->max_energy);
  } glPopMatrix();
}

/*===========================================================================*/

static void set_gun_color(az_gun_t gun) {
  switch (gun) {
    case AZ_GUN_NONE: AZ_ASSERT_UNREACHABLE();
    case AZ_GUN_CHARGE: glColor3f(1, 1, 1); break;
    case AZ_GUN_FREEZE: glColor3f(0, 1, 1); break;
    case AZ_GUN_TRIPLE: glColor3f(0, 1, 0); break;
    case AZ_GUN_HOMING: glColor3f(0, 0, 1); break;
    case AZ_GUN_BEAM:   glColor3f(1, 0, 0); break;
    case AZ_GUN_PHASE:  glColor3f(1, 1, 0); break;
    case AZ_GUN_BURST:  glColor3f(0.5, 0.5, 0.5); break;
    case AZ_GUN_PIERCE: glColor3f(1, 0, 1); break;
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
    glTranslatef(AZ_SCREEN_WIDTH - HUD_MARGIN - width, HUD_MARGIN, 0);

    glColor4f(0, 0, 0, 0.75); // tinted-black
    glBegin(GL_QUADS);
    glVertex2i(0, 0);
    glVertex2i(0, height);
    glVertex2i(width, height);
    glVertex2i(width, 0);
    glEnd();

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

#define BOSS_BAR_WIDTH 400

static void draw_hud_boss_health(az_space_state_t *state) {
  az_baddie_t *baddie;
  if (az_lookup_baddie(state, state->boss_uid, &baddie)) {
    glPushMatrix(); {
      glTranslatef(HUD_MARGIN, AZ_SCREEN_HEIGHT - HUD_MARGIN -
                   2 * HUD_PADDING - 10, 0);

      const int height = 10 + 2 * HUD_PADDING;
      const int width = 2 * HUD_PADDING + 35 + BOSS_BAR_WIDTH;
      glColor4f(0, 0, 0, 0.75); // tinted-black
      glBegin(GL_QUADS);
      glVertex2i(0, 0);
      glVertex2i(0, height);
      glVertex2i(width, height);
      glVertex2i(width, 0);
      glEnd();

      glTranslatef(HUD_PADDING, HUD_PADDING, 0);

      glColor3f(1, 1, 1); // white
      az_draw_string(8, AZ_ALIGN_LEFT, 0, 1, "BOSS");
      assert(baddie->health > 0.0);
      assert(baddie->data->max_health > 0.0);
      glColor3f(1, 0.25, 0); // red-orange
      draw_hud_bar(35, 0, (baddie->health / baddie->data->max_health) *
                   BOSS_BAR_WIDTH, BOSS_BAR_WIDTH);
    } glPopMatrix();
  }
}

static void draw_hud_message(const az_message_t *message) {
  if (message->time_remaining <= 0.0) {
    assert(message->text == NULL);
    return;
  }
  assert(message->text != NULL);
  assert(message->text->num_lines > 0);
  glPushMatrix(); {
    const int width = AZ_SCREEN_WIDTH - 20;
    const int height = 6 + 20 * message->text->num_lines;
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

    for (int i = 0; i < message->text->num_lines; ++i) {
      const az_text_line_t *line = &message->text->lines[i];
      int left = AZ_SCREEN_WIDTH / 2 - 8 * line->total_length;
      for (int j = 0; j < line->num_fragments; ++j) {
        const az_text_fragment_t *fragment = &line->fragments[j];
        const az_color_t color = fragment->color;
        glColor4ub(color.r, color.g, color.b, color.a);
        az_draw_chars(16, AZ_ALIGN_LEFT, left, 6 + 20 * i,
                      fragment->chars, fragment->length);
        left += fragment->length * 16;
      }
    }
  } glPopMatrix();
}

static void draw_hud_countdown(const az_countdown_t *countdown,
                               az_clock_t clock) {
  if (!countdown->is_active) return;
  glPushMatrix(); {
    const int width = 2 * HUD_PADDING + 7 * 24;
    const int height = 2 * HUD_PADDING + 24;

    const int xstart = AZ_SCREEN_WIDTH/2 - width/2;
    const int ystart = AZ_SCREEN_HEIGHT/2 + 75 - height/2;
    const int xend = AZ_SCREEN_WIDTH - HUD_MARGIN - width;
    const int yend = AZ_SCREEN_HEIGHT - HUD_MARGIN - height;
    const double speed = 150.0; // pixels per second
    const double offset = fmax(0.0, countdown->active_for - 2.5) * speed;
    glTranslatef(az_imin(xend, xstart + offset),
                 az_imin(yend, ystart + offset), 0);

    glColor4f(0, 0, 0, 0.75); // tinted-black
    glBegin(GL_QUADS); {
      glVertex2i(0, 0);
      glVertex2i(0, height);
      glVertex2i(width, height);
      glVertex2i(width, 0);
    } glEnd();

    assert(countdown->time_remaining >= 0.0);
    if (countdown->time_remaining >= 10.0) {
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

static void draw_box(double left, double top, double width, double height) {
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

/*===========================================================================*/

#define DIALOG_BOX_WIDTH 404
#define DIALOG_BOX_HEIGHT 100
#define DIALOG_BOX_MARGIN 10
#define PORTRAIT_BOX_WIDTH 150
#define PORTRAIT_BOX_HEIGHT 150
#define PORTRAIT_BOX_MARGIN 15
#define DIALOG_HORZ_SPACING 20
#define DIALOG_VERT_SPACING 50

static void draw_dialog_frames(double openness) {
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
    draw_box(tcx - sw, tcy - sh, sw * 2, sh * 2);
    // Bottom portrait:
    const int bcx =
      (AZ_SCREEN_WIDTH + DIALOG_HORZ_SPACING + DIALOG_BOX_WIDTH) / 2;
    const int bcy =
      (AZ_SCREEN_HEIGHT + DIALOG_VERT_SPACING + PORTRAIT_BOX_HEIGHT) / 2;
    draw_box(bcx - sw, bcy - sh, sw * 2, sh * 2);
  }
  { // Dialog boxes:
    const double sw = 0.5 * DIALOG_BOX_WIDTH * openness;
    const double sh = 0.5 * DIALOG_BOX_HEIGHT * openness;
    // Top dialog box:
    const int tcx =
      (AZ_SCREEN_WIDTH + DIALOG_HORZ_SPACING + PORTRAIT_BOX_WIDTH) / 2;
    const int tcy =
      (AZ_SCREEN_HEIGHT - DIALOG_VERT_SPACING - DIALOG_BOX_HEIGHT) / 2;
    draw_box(tcx - sw, tcy - sh, sw * 2, sh * 2);
    // Bottom dialog box:
    const int bcx =
      (AZ_SCREEN_WIDTH - DIALOG_HORZ_SPACING - PORTRAIT_BOX_WIDTH) / 2;
    const int bcy =
      (AZ_SCREEN_HEIGHT + DIALOG_VERT_SPACING + DIALOG_BOX_HEIGHT) / 2;
    draw_box(bcx - sw, bcy - sh, sw * 2, sh * 2);
  }
}

static void draw_dialog_text_line(const az_text_line_t *line, int num_chars) {
  for (int i = 0, left = 0; i < line->num_fragments && num_chars > 0; ++i) {
    const az_text_fragment_t *fragment = &line->fragments[i];
    const az_color_t color = fragment->color;
    glColor4ub(color.r, color.g, color.b, color.a);
    az_draw_chars(8, AZ_ALIGN_LEFT, left, 0, fragment->chars,
                  az_imin(fragment->length, num_chars));
    left += 8 * fragment->length;
    num_chars -= fragment->length;
  }
}

static void draw_dialog_text(const az_space_state_t *state) {
  assert(state->mode == AZ_MODE_DIALOG);
  glPushMatrix(); {
    if (!state->mode_data.dialog.bottom_next) {
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

    if (state->mode_data.dialog.step == AZ_DLS_PAUSE) {
      if (az_clock_mod(2, 15, state->clock)) glColor4f(0.5, 0.5, 0.5, 0.5);
      else glColor4f(0.25, 0.75, 0.75, 0.5);
      az_draw_string(8, AZ_ALIGN_RIGHT,
                     DIALOG_BOX_WIDTH - 2 * DIALOG_BOX_MARGIN,
                     DIALOG_BOX_HEIGHT - 2 * DIALOG_BOX_MARGIN - 8, "[ENTER]");
    }

    const az_text_t *text = state->mode_data.dialog.text;
    for (int i = 0; i < state->mode_data.dialog.row; ++i) {
      draw_dialog_text_line(&text->lines[i], text->lines[i].total_length);
      glTranslatef(0, 12, 0);
    }
    if (state->mode_data.dialog.row < text->num_lines) {
      draw_dialog_text_line(&text->lines[state->mode_data.dialog.row],
                            state->mode_data.dialog.col);
    }
  } glPopMatrix();
}

static void draw_dialog_portrait(az_portrait_t portrait, bool is_bottom) {
  glPushMatrix(); {
    if (!is_bottom) {
      glTranslatef((AZ_SCREEN_WIDTH - DIALOG_HORZ_SPACING - DIALOG_BOX_WIDTH -
                    PORTRAIT_BOX_WIDTH) / 2 + PORTRAIT_BOX_MARGIN,
                   (AZ_SCREEN_HEIGHT - DIALOG_VERT_SPACING) / 2 -
                   PORTRAIT_BOX_HEIGHT + PORTRAIT_BOX_MARGIN, 0);
    } else {
      glTranslatef((AZ_SCREEN_WIDTH + DIALOG_HORZ_SPACING + DIALOG_BOX_WIDTH -
                    PORTRAIT_BOX_WIDTH) / 2 + PORTRAIT_BOX_MARGIN,
                   (AZ_SCREEN_HEIGHT + DIALOG_VERT_SPACING) / 2 +
                   PORTRAIT_BOX_MARGIN, 0);
    }
    switch (portrait) {
      case AZ_POR_NOTHING: break;
      case AZ_POR_HOPPER: break; // TODO
      case AZ_POR_HQ: break; // TODO
      case AZ_POR_CPU_A: break; // TODO
      case AZ_POR_CPU_B: break; // TODO
      case AZ_POR_CPU_C: break; // TODO
      case AZ_POR_TRICHORD: break; // TODO
    }
  } glPopMatrix();
}

static void draw_dialog(const az_space_state_t *state) {
  assert(state->mode == AZ_MODE_DIALOG);
  switch (state->mode_data.dialog.step) {
    case AZ_DLS_BEGIN:
      draw_dialog_frames(state->mode_data.dialog.progress);
      break;
    case AZ_DLS_TALK:
    case AZ_DLS_PAUSE:
      draw_dialog_frames(1.0);
      draw_dialog_text(state);
      draw_dialog_portrait(state->mode_data.dialog.top, false);
      draw_dialog_portrait(state->mode_data.dialog.bottom, true);
      break;
    case AZ_DLS_END:
      draw_dialog_frames(1.0 - state->mode_data.dialog.progress);
      break;
  }
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
  draw_box(left, top, width, height);
}

static void draw_upgrade_box_message(az_upgrade_t upgrade) {
  const char *name = NULL, *line1 = NULL, *line2 = NULL;
  switch (upgrade) {
    case AZ_UPG_GUN_CHARGE:
      name = "CHARGE GUN";
      line1 = "Hold [V] to charge, release to fire.";
      break;
    case AZ_UPG_GUN_FREEZE:
      name = "FREEZE GUN";
      line1 = "Shots can freeze enemies.";
      line2 = "Press [2] to select, press [V] to fire.";
      break;
    case AZ_UPG_GUN_TRIPLE:
      name = "TRIPLE GUN";
      line1 = "Fires three shots at once.";
      line2 = "Press [3] to select, press [V] to fire.";
      break;
    case AZ_UPG_GUN_HOMING:
      name = "HOMING GUN";
      line1 = "Shots will seek out enemies.";
      line2 = "Press [4] to select, press [V] to fire.";
      break;
    case AZ_UPG_GUN_PHASE:
      name = "PHASE GUN";
      line1 = "Shots will pass through walls.";
      line2 = "Press [5] to select, press [V] to fire.";
      break;
    case AZ_UPG_GUN_BEAM:
      name = "BEAM GUN";
      line1 = "Fires a continuous beam.";
      line2 = "Press [8] to select, press [V] to fire.";
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
    case AZ_UPG_HYPER_ROCKETS:
      name = "HYPER ROCKETS";
      line1 = "Press [9] to select rockets, hold [C] to charge,";
      line2 = "and press [V] to fire.  Uses 5 rockets.";
      break;
    case AZ_UPG_BOMB_AMMO_00:
    case AZ_UPG_BOMB_AMMO_01:
    case AZ_UPG_BOMB_AMMO_02:
    case AZ_UPG_BOMB_AMMO_03:
    case AZ_UPG_BOMB_AMMO_04:
    case AZ_UPG_BOMB_AMMO_05:
    case AZ_UPG_BOMB_AMMO_06:
    case AZ_UPG_BOMB_AMMO_07:
    case AZ_UPG_BOMB_AMMO_08:
    case AZ_UPG_BOMB_AMMO_09:
      name = "BOMBS";
      line1 = "Maximum bombs increased by 3.";
      line2 = "Press [0] to select, hold [C] and press [V] to drop.";
      break;
    case AZ_UPG_MEGA_BOMBS:
      name = "MEGA BOMBS";
      line1 = "Press [0] to select bombs, hold [C] to charge,";
      line2 = "and press [V] to drop.  Uses 5 bombs.";
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

void az_draw_hud(az_space_state_t *state) {
  const az_ship_t *ship = &state->ship;
  if (!az_ship_is_present(ship)) return;
  const az_player_t *player = &ship->player;
  draw_hud_shields_energy(player, state->clock);
  draw_hud_weapons_selection(player);
  draw_hud_boss_health(state);
  draw_hud_message(&state->message);
  draw_hud_countdown(&state->countdown, state->clock);
  if (state->mode == AZ_MODE_DIALOG) {
    draw_dialog(state);
  } else if (state->mode == AZ_MODE_UPGRADE) {
    draw_upgrade_box(state);
  }
}

/*===========================================================================*/
