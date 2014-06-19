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

#include "azimuth/view/title.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>

#include <GL/gl.h>

#include "azimuth/constants.h"
#include "azimuth/gui/event.h"
#include "azimuth/state/player.h"
#include "azimuth/state/save.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/polygon.h"
#include "azimuth/util/vector.h"
#include "azimuth/view/cursor.h"
#include "azimuth/view/cutscene.h"
#include "azimuth/view/string.h"
#include "azimuth/view/util.h"

/*===========================================================================*/

static void do_polygon(GLenum mode, az_polygon_t polygon) {
  glBegin(mode); {
    for (int i = 0; i < polygon.num_vertices; ++i) {
      az_gl_vertex(polygon.vertices[i]);
    }
  } glEnd();
}

/*===========================================================================*/

static void draw_title_letter(char ch, float hilight) {
  glColor4f(0.25f + hilight, 0.35f + 0.5f * hilight, 0.35f, 0.5f);
  switch (ch) {
    case 'A':
      glBegin(GL_TRIANGLE_STRIP); {
        glVertex2f(-28, 25); glVertex2f(-15, 25);
        glVertex2f(-5, -25); glVertex2f(0, -10); glVertex2f(5, -25);
        glVertex2f(15, 25); glVertex2f(28, 25);
      } glEnd();
      glBegin(GL_TRIANGLE_STRIP); {
        glVertex2f(-5, 0); glVertex2f(5, 0);
        glVertex2f(-9, 10); glVertex2f(9, 10);
      } glEnd();
      glBegin(GL_LINE_LOOP); {
        glColor3f(1, 1, 1);
        glVertex2f(28, 25); glVertex2f(5, -25); glVertex2f(-5, -25);
        glVertex2f(-28, 25); glVertex2f(-15, 25); glVertex2f(-9, 10);
        glVertex2f(9, 10); glVertex2f(15, 25);
      } glEnd();
      glBegin(GL_LINE_LOOP); {
        glColor3f(1, 1, 1);
        glVertex2f(-5, 0); glVertex2f(5, 0); glVertex2f(0, -10);
      } glEnd();
      break;
    case 'Z':
      glBegin(GL_TRIANGLE_STRIP); {
        glVertex2f(-25, -25); glVertex2f(-25, -15); glVertex2f(25, -25);
        glVertex2f(10, -15); glVertex2f(25, -15); glVertex2f(-25, 15);
        glVertex2f(-10, 15); glVertex2f(-25, 25); glVertex2f(25, 15);
        glVertex2f(25, 25);
      } glEnd();
      glBegin(GL_LINE_LOOP); {
        glColor3f(1, 1, 1);
        glVertex2f(25, -25); glVertex2f(-25, -25); glVertex2f(-25, -15);
        glVertex2f(10, -15); glVertex2f(-25, 15); glVertex2f(-25, 25);
        glVertex2f(25, 25); glVertex2f(25, 15); glVertex2f(-10, 15);
        glVertex2f(25, -15);
      } glEnd();
      break;
    case 'I':
      glBegin(GL_TRIANGLE_STRIP); {
        glVertex2f(-18, -25); glVertex2f(18, -25);
        glVertex2f(-18, -15); glVertex2f(18, -15);
      } glEnd();
      glBegin(GL_TRIANGLE_STRIP); {
        glVertex2f(-5, -15); glVertex2f(5, -15);
        glVertex2f(-5, 15); glVertex2f(5, 15);
      } glEnd();
      glBegin(GL_TRIANGLE_STRIP); {
        glVertex2f(-18, 15); glVertex2f(18, 15);
        glVertex2f(-18, 25); glVertex2f(18, 25);
      } glEnd();
      glBegin(GL_LINE_LOOP); {
        glColor3f(1, 1, 1);
        glVertex2f(18, -25); glVertex2f(-18, -25); glVertex2f(-18, -15);
        glVertex2f(-5, -15); glVertex2f(-5, 15); glVertex2f(-18, 15);
        glVertex2f(-18, 25); glVertex2f(18, 25); glVertex2f(18, 15);
        glVertex2f(5, 15); glVertex2f(5, -15); glVertex2f(18, -15);
      } glEnd();
      break;
    case 'M':
      glBegin(GL_TRIANGLE_STRIP); {
        glVertex2f(-25, 25); glVertex2f(-15, 25); glVertex2f(-25, -25);
        glVertex2f(-15, -10); glVertex2f(-15, -25); glVertex2f(0, 5);
        glVertex2f(0, -10); glVertex2f(15, -10); glVertex2f(15, -25);
      } glEnd();
      glBegin(GL_TRIANGLE_STRIP); {
        glVertex2f(15, -25); glVertex2f(25, -25);
        glVertex2f(15, 25); glVertex2f(25, 25);
      } glEnd();
      glBegin(GL_LINE_LOOP); {
        glColor3f(1, 1, 1);
        glVertex2f(25, -25); glVertex2f(15, -25); glVertex2f(0, -10);
        glVertex2f(-15, -25); glVertex2f(-25, -25); glVertex2f(-25, 25);
        glVertex2f(-15, 25); glVertex2f(-15, -10); glVertex2f(0, 5);
        glVertex2f(15, -10); glVertex2f(15, 25); glVertex2f(25, 25);
      } glEnd();
      break;
    case 'U':
      glBegin(GL_TRIANGLE_STRIP); {
        glVertex2f(-25, -25); glVertex2f(-15, -25); glVertex2f(-25, 25);
        glVertex2f(-15, 15); glVertex2f(25, 25); glVertex2f(15, 15);
        glVertex2f(25, -25); glVertex2f(15, -25);
      } glEnd();
      glBegin(GL_LINE_LOOP); {
        glColor3f(1, 1, 1);
        glVertex2f(-25, -25); glVertex2f(-25, 25); glVertex2f(25, 25);
        glVertex2f(25, -25); glVertex2f(15, -25); glVertex2f(15, 15);
        glVertex2f(-15, 15); glVertex2f(-15, -25);
      } glEnd();
      break;
    case 'T':
      glBegin(GL_TRIANGLE_STRIP); {
        glVertex2f(-25, -25); glVertex2f(25, -25);
        glVertex2f(-25, -15); glVertex2f(25, -15);
      } glEnd();
      glBegin(GL_TRIANGLE_STRIP); {
        glVertex2f(-5, -15); glVertex2f(5, -15);
        glVertex2f(-5, 25); glVertex2f(5, 25);
      } glEnd();
      glBegin(GL_LINE_LOOP); {
        glColor3f(1, 1, 1);
        glVertex2f(25, -25); glVertex2f(-25, -25); glVertex2f(-25, -15);
        glVertex2f(-5, -15); glVertex2f(-5, 25); glVertex2f(5, 25);
        glVertex2f(5, -15); glVertex2f(25, -15);
      } glEnd();
      break;
    case 'H':
      glBegin(GL_TRIANGLE_STRIP); {
        glVertex2f(-25, -25); glVertex2f(-15, -25);
        glVertex2f(-25, 25); glVertex2f(-15, 25);
      } glEnd();
      glBegin(GL_TRIANGLE_STRIP); {
        glVertex2f(-15, -5); glVertex2f(15, -5);
        glVertex2f(-15, 5); glVertex2f(15, 5);
      } glEnd();
      glBegin(GL_TRIANGLE_STRIP); {
        glVertex2f(15, -25); glVertex2f(25, -25);
        glVertex2f(15, 25); glVertex2f(25, 25);
      } glEnd();
      glBegin(GL_LINE_LOOP); {
        glColor3f(1, 1, 1);
        glVertex2f(25, -25); glVertex2f(15, -25); glVertex2f(15, -5);
        glVertex2f(-15, -5); glVertex2f(-15, -25); glVertex2f(-25, -25);
        glVertex2f(-25, 25); glVertex2f(-15, 25); glVertex2f(-15, 5);
        glVertex2f(15, 5); glVertex2f(15, 25); glVertex2f(25, 25);
      } glEnd();
      break;
    default: break;
  }
}

static void draw_background(const az_title_state_t *state) {
  az_draw_planet_starfield(state->clock);
  az_draw_zenith_planet(state->clock);

  // Draw game title:
  glPushMatrix(); {
    glTranslatef(AZ_SCREEN_WIDTH / 2, 150, 0);
    glTranslatef(-3 * 80 + 0.5, 0.5, 0);
    for (int i = 0; i < 7; ++i) {
      glPushMatrix(); {
        if (state->mode == AZ_TMODE_INTRO) {
          const GLfloat angle = (i % 2 ? -45 : 45);
          glRotatef(angle, 0, 0, 1);
          glScalef(state->mode_data.intro.progress, 1, 1);
          glRotatef(-angle, 0, 0, 1);
        }
        draw_title_letter("AZIMUTH"[i], fmax(0.0, 0.005 * (az_clock_zigzag(
            200, 1, state->clock + 10 * (7 - i)) - 140)));
      } glPopMatrix();
      glTranslatef(80, 0, 0);
    }
  } glPopMatrix();
}

/*===========================================================================*/

#define PREFS_BOX_LEFT ((AZ_SCREEN_WIDTH - AZ_PREFS_BOX_WIDTH) / 2)
#define PREFS_BOX_TOP 220

static const az_vector_t prefs_box_vertices[] = {
  {PREFS_BOX_LEFT + 0.5, PREFS_BOX_TOP + 0.5},
  {PREFS_BOX_LEFT + AZ_PREFS_BOX_WIDTH - 0.5, PREFS_BOX_TOP + 0.5},
  {PREFS_BOX_LEFT + AZ_PREFS_BOX_WIDTH - 0.5,
   PREFS_BOX_TOP + AZ_PREFS_BOX_HEIGHT - 0.5},
  {PREFS_BOX_LEFT + 0.5, PREFS_BOX_TOP + AZ_PREFS_BOX_HEIGHT - 0.5}
};
static const az_polygon_t prefs_box_polygon =
  AZ_INIT_POLYGON(prefs_box_vertices);

static void draw_prefs_box(const az_title_state_t *state) {
  glColor4f(0, 0, 0, 0.7); // black tint
  do_polygon(GL_POLYGON, prefs_box_polygon);
  glColor3f(0.75, 0.75, 0.75); // light gray
  do_polygon(GL_LINE_LOOP, prefs_box_polygon);
  az_draw_prefs_pane(&state->prefs_pane);
}

/*===========================================================================*/

#define ABOUT_BOX_TOP 260
#define ABOUT_BOX_WIDTH 512
#define ABOUT_BOX_HEIGHT 146

static const az_vector_t about_box_vertices[] = {
  {0.5, 0.5}, {ABOUT_BOX_WIDTH - 0.5, 0.5},
  {ABOUT_BOX_WIDTH - 0.5, ABOUT_BOX_HEIGHT - 0.5},
  {0.5, ABOUT_BOX_HEIGHT - 0.5}
};
static const az_polygon_t about_box_polygon =
  AZ_INIT_POLYGON(about_box_vertices);

static const char *about_box_lines[] = {
  "Copyright 2012 Matthew D Steele <mdsteele@alum.mit.edu>",
  "",
  "This program comes with ABSOLUTELY NO WARRANTY.",
  "This program is free software: you can redistribute it and/or",
  "modify it under the terms of the GNU General Public License as",
  "published by the Free Software Foundation, either version 3 of",
  "the License, or (at your option) any later version.",
  "",
  "Thanks for playing!"
};

static void draw_about_box(void) {
  glPushMatrix(); {
    glTranslated(0.5 * (AZ_SCREEN_WIDTH - ABOUT_BOX_WIDTH), ABOUT_BOX_TOP, 0);
    glColor4f(0, 0, 0, 0.7); // black tint
    do_polygon(GL_POLYGON, about_box_polygon);
    glColor3f(0.75, 0.75, 0.75); // light gray
    do_polygon(GL_LINE_LOOP, about_box_polygon);

    glColor3f(1, 1, 1); // white
    for (int i = 0; i < AZ_ARRAY_SIZE(about_box_lines); ++i) {
      az_draw_string(8, AZ_ALIGN_LEFT, 8, 8 + 15 * i, about_box_lines[i]);
    }
  } glPopMatrix();
}

/*===========================================================================*/

#define SAVE_SLOT_WIDTH 160
#define SAVE_SLOT_HEIGHT 70
#define SAVE_SLOT_SPACING 20
#define SAVE_SLOTS_PER_ROW 3
#define SAVE_SLOTS_TOP 250
#define SAVE_SLOTS_LEFT \
  ((AZ_SCREEN_WIDTH - \
    SAVE_SLOTS_PER_ROW * (SAVE_SLOT_WIDTH + SAVE_SLOT_SPACING) + \
    SAVE_SLOT_SPACING) / 2)

static const az_vector_t save_slot_vertices[] = {
  {0.5, SAVE_SLOT_HEIGHT - 0.5}, {0.5, 0.5}, {SAVE_SLOT_WIDTH - 68.5, 0.5},
  {SAVE_SLOT_WIDTH - 50.5, 18.5}, {SAVE_SLOT_WIDTH - 0.5, 18.5},
  {SAVE_SLOT_WIDTH - 0.5, SAVE_SLOT_HEIGHT - 0.5}
};
static const az_polygon_t save_slot_polygon =
  AZ_INIT_POLYGON(save_slot_vertices);

static const az_vector_t save_slot_erase_vertices[] = {
  {SAVE_SLOT_WIDTH - 0.5, 0.5}, {SAVE_SLOT_WIDTH - 0.5, 16.5},
  {SAVE_SLOT_WIDTH - 49.5, 16.5}, {SAVE_SLOT_WIDTH - 65.5, 0.5}
};
static const az_polygon_t save_slot_erase_polygon =
  AZ_INIT_POLYGON(save_slot_erase_vertices);

static const az_vector_t save_slot_confirm_cancel_vertices[] = {
  {24, 12}, {-24, 12}, {-24, -12}, {24, -12}
};
static const az_polygon_t save_slot_confirm_cancel_polygon =
  AZ_INIT_POLYGON(save_slot_confirm_cancel_vertices);

/*===========================================================================*/

static void draw_save_slot(const az_title_state_t *state, int index) {
  assert(index >= 0);
  assert(index < AZ_NUM_SAVED_GAME_SLOTS);
  const az_saved_game_t *saved_game = &state->saved_games->games[index];
  const az_title_save_slot_t *slot = &state->slots[index];
  const bool normal = (state->mode == AZ_TMODE_NORMAL);
  const bool erasing = (state->mode == AZ_TMODE_ERASING &&
                        state->mode_data.erasing.slot_index == index);
  const bool starting = (state->mode == AZ_TMODE_STARTING &&
                         state->mode_data.starting.slot_index == index);
  glPushMatrix(); {
    glTranslatef(slot->x, slot->y, 0);

    // Draw slot frame and title:
    az_draw_standard_button(&slot->main);
    if (normal || erasing || starting) glColor3f(1, 1, 1); // white
    else glColor3f(0.25, 0.25, 0.25); // dark gray
    az_draw_printf(8, AZ_ALIGN_LEFT, 6, 6, "HOPPER %d", index + 1);

    // Draw "ERASE" button:
    if (saved_game->present) {
      az_draw_dangerous_button(&slot->erase);
      if (normal) glColor3f(0.75, 0, 0); // red
      else glColor3f(0.25, 0.25, 0.25); // dark gray
      az_draw_string(8, AZ_ALIGN_RIGHT, SAVE_SLOT_WIDTH - 5, 5, "ERASE");
    }

    // If the save slot is empty, mark it as a new game.
    if (!saved_game->present) {
      if (normal) glColor3f(1, 1, 1); // white
      else glColor3f(0.25, 0.25, 0.25); // dark gray
      az_draw_string(8, AZ_ALIGN_CENTER, SAVE_SLOT_WIDTH/2,
                     SAVE_SLOT_HEIGHT/2 + 4, "New Game");
    }
    // If we're in ERASING mode for this save slot, draw the confirmation UI:
    else if (erasing) {
      glColor3f(1, 1, 1); // white
      az_draw_string(8, AZ_ALIGN_CENTER, SAVE_SLOT_WIDTH/2,
                     SAVE_SLOT_HEIGHT/2 - 4, "Are you sure?");
      if (slot->confirm.hovering) {
        glColor3f(1, slot->confirm.hover_pulse, 0);
      } else glColor3f(0.3, 0, 0);
      az_draw_string(8, AZ_ALIGN_CENTER, slot->confirm.x, slot->confirm.y - 4,
                     "ERASE");
      if (slot->cancel.hovering) {
        glColor3f(slot->cancel.hover_pulse, 1, 0);
      } else glColor3f(0, 0.3, 0);
      az_draw_string(8, AZ_ALIGN_CENTER, slot->cancel.x, slot->cancel.y - 4,
                     "CANCEL");
    }
    // Otherwise, draw save file info:
    else {
      const az_player_t *player = &saved_game->player;

      // Draw the name of the zone the player is in for this save file:
      const az_room_t *room = &state->planet->rooms[player->current_room];
      assert(0 <= room->zone_key);
      assert(room->zone_key < state->planet->num_zones);
      const az_zone_t *zone = &state->planet->zones[room->zone_key];
      if (normal || starting) az_gl_color(zone->color);
      else glColor3ub(zone->color.r / 3, zone->color.g / 3, zone->color.b / 3);
      az_draw_string(8, AZ_ALIGN_LEFT, 6, 24, zone->name);

      // Draw the elapsed time of this save file, as "hours:minutes":
      if (normal || starting) glColor3f(1, 1, 1); // white
      else glColor3f(0.25, 0.25, 0.25); // dark gray
      assert(player->total_time >= 0.0);
      const int seconds = floor(player->total_time);
      az_draw_printf(8, AZ_ALIGN_RIGHT, SAVE_SLOT_WIDTH - 6, 24, "%d:%02d",
                     seconds / 3600, (seconds / 60) % 60);

      // Draw the save file's maximum shields and energy:
      az_draw_printf(8, AZ_ALIGN_LEFT, 6, 39, "Shield:%.0f",
                     player->max_shields);
      az_draw_printf(8, AZ_ALIGN_LEFT, 6, 54, "Energy:%.0f",
                     player->max_energy);

      // Draw the save file's ammo reserves:
      if (player->max_rockets > 0) {
        az_draw_printf(8, AZ_ALIGN_RIGHT, SAVE_SLOT_WIDTH - 6, 39, "%d R",
                       player->rockets);
      }
      if (player->max_bombs > 0) {
        az_draw_printf(8, AZ_ALIGN_RIGHT, SAVE_SLOT_WIDTH - 6, 54, "%d B",
                       player->bombs);
      }
    }
  } glPopMatrix();
}

/*===========================================================================*/

#define BUTTON_WIDTH 100
#define BUTTON_HEIGHT 20
#define BUTTON_SPACING 54

static const az_vector_t bottom_button_vertices[] = {
  {BUTTON_WIDTH - 5.5, 0.5}, {BUTTON_WIDTH - 0.5, 0.5 * BUTTON_HEIGHT},
  {BUTTON_WIDTH - 5.5, BUTTON_HEIGHT - 0.5},
  {5.5, BUTTON_HEIGHT - 0.5}, {0.5, 0.5 * BUTTON_HEIGHT}, {5.5, 0.5}
};
static const az_polygon_t bottom_button_polygon =
  AZ_INIT_POLYGON(bottom_button_vertices);

/*===========================================================================*/

static bool prefs_about_buttons_active(const az_title_state_t *state) {
  return (state->mode == AZ_TMODE_NORMAL || state->mode == AZ_TMODE_PREFS ||
          state->mode == AZ_TMODE_ABOUT);
}

static bool quit_button_active(const az_title_state_t *state) {
  return (state->mode == AZ_TMODE_NORMAL || state->mode == AZ_TMODE_PREFS ||
          state->mode == AZ_TMODE_ABOUT || state->mode == AZ_TMODE_PICK_KEY ||
          state->mode == AZ_TMODE_ERASING);
}

static void draw_bottom_button(const az_button_t *button, bool active,
                               const char *label) {
  az_draw_standard_button(button);
  if (active) glColor3f(1, 1, 1); // white
  else glColor3f(0.25, 0.25, 0.25); // dark gray
  az_draw_string(8, AZ_ALIGN_CENTER, button->x + BUTTON_WIDTH/2,
                 button->y + BUTTON_HEIGHT/2 - 4, label);
}

/*===========================================================================*/

void az_init_title_state(az_title_state_t *state, const az_planet_t *planet,
                         const az_saved_games_t *saved_games,
                         const az_preferences_t *prefs) {
  AZ_ZERO_OBJECT(state);
  state->planet = planet;
  state->saved_games = saved_games;

  for (int i = 0; i < AZ_ARRAY_SIZE(state->slots); ++i) {
    az_title_save_slot_t *save_slot = &state->slots[i];
    const int row = i / SAVE_SLOTS_PER_ROW, col = i % SAVE_SLOTS_PER_ROW;
    save_slot->x = SAVE_SLOTS_LEFT +
      col * (SAVE_SLOT_WIDTH + SAVE_SLOT_SPACING);
    save_slot->y = SAVE_SLOTS_TOP +
      row * (SAVE_SLOT_HEIGHT + SAVE_SLOT_SPACING);
    az_init_button(&save_slot->main, save_slot_polygon, 0, 0);
    az_init_button(&save_slot->erase, save_slot_erase_polygon, 0, 0);
    const int confirm_cancel_y = SAVE_SLOT_HEIGHT/2 + 18;
    az_init_button(&save_slot->confirm, save_slot_confirm_cancel_polygon,
                   SAVE_SLOT_WIDTH/4, confirm_cancel_y);
    az_init_button(&save_slot->cancel, save_slot_confirm_cancel_polygon,
                   SAVE_SLOT_WIDTH*3/4, confirm_cancel_y);
  }

  const int bottom_buttons_left =
    (AZ_SCREEN_WIDTH - 3 * BUTTON_WIDTH - 2 * BUTTON_SPACING) / 2;
  const int bottom_buttons_top =
    SAVE_SLOTS_TOP + 2 * (SAVE_SLOT_HEIGHT + SAVE_SLOT_SPACING);
  az_init_button(&state->prefs_button, bottom_button_polygon,
                 bottom_buttons_left, bottom_buttons_top);
  az_init_button(&state->about_button, bottom_button_polygon,
                 bottom_buttons_left + BUTTON_WIDTH + BUTTON_SPACING,
                 bottom_buttons_top);
  az_init_button(&state->quit_button, bottom_button_polygon,
                 bottom_buttons_left + 2 * (BUTTON_WIDTH + BUTTON_SPACING),
                 bottom_buttons_top);

  az_init_prefs_pane(&state->prefs_pane, PREFS_BOX_LEFT, PREFS_BOX_TOP,
                     prefs);
}

/*===========================================================================*/

static void fade_screen_black(GLfloat alpha) {
  glColor4f(0, 0, 0, alpha);
  glBegin(GL_QUADS); {
    glVertex2i(0, 0);
    glVertex2i(AZ_SCREEN_WIDTH, 0);
    glVertex2i(AZ_SCREEN_WIDTH, AZ_SCREEN_HEIGHT);
    glVertex2i(0, AZ_SCREEN_HEIGHT);
  } glEnd();
}

void az_title_draw_screen(const az_title_state_t *state) {
  // Draw background:
  draw_background(state);
  if (state->mode == AZ_TMODE_INTRO) return;
  else if (state->mode == AZ_TMODE_READY) {
    glColor4f(0, 0.75, 0.5,
              0.01 * az_clock_zigzag(100, 2, state->clock -
                                     state->mode_data.ready.start));
    az_draw_string(16, AZ_ALIGN_CENTER, AZ_SCREEN_WIDTH/2, 430,
                   "Press any key");
    return;
  }

  // Draw save slots:
  if (state->mode == AZ_TMODE_PREFS || state->mode == AZ_TMODE_PICK_KEY) {
    draw_prefs_box(state);
  } else if (state->mode == AZ_TMODE_ABOUT) {
    draw_about_box();
  } else {
    for (int i = 0; i < AZ_ARRAY_SIZE(state->saved_games->games); ++i) {
      draw_save_slot(state, i);
    }
  }

  // Draw bottom three buttons:
  const bool prefs_and_about_active = prefs_about_buttons_active(state);
  draw_bottom_button(&state->prefs_button, prefs_and_about_active,
                     (state->mode == AZ_TMODE_PREFS ||
                      state->mode == AZ_TMODE_PICK_KEY ? "Done" : "Options"));
  draw_bottom_button(&state->about_button, prefs_and_about_active,
                     (state->mode == AZ_TMODE_ABOUT ? "Done" : "About"));
  draw_bottom_button(&state->quit_button, quit_button_active(state), "Quit");

  // Fade out screen:
  if (state->mode == AZ_TMODE_STARTING) {
    fade_screen_black(state->mode_data.starting.progress);
  }

  az_draw_cursor();
}

/*===========================================================================*/

#define INTRO_TIME 3.0
#define STARTING_TIME 0.9

static void tick_mode(az_title_state_t *state, double time) {
  switch (state->mode) {
    case AZ_TMODE_INTRO:
      state->mode_data.intro.progress =
        fmin(1.0, state->mode_data.intro.progress + time / INTRO_TIME);
      if (state->mode_data.intro.progress >= 1.0) {
        az_title_skip_intro(state);
      }
      break;
    case AZ_TMODE_STARTING:
      state->mode_data.starting.progress =
        fmin(1.0, state->mode_data.starting.progress + time / STARTING_TIME);
      break;
    default: break;
  }
}

static void tick_save_slot(az_title_state_t *state, int index, double time,
                           az_clock_t clock, az_soundboard_t *soundboard) {
  az_title_save_slot_t *slot = &state->slots[index];
  az_tick_button(&slot->main, slot->x, slot->y,
                 (state->mode == AZ_TMODE_NORMAL), time, clock, soundboard);
  az_tick_button(&slot->erase, slot->x, slot->y,
                 (state->mode == AZ_TMODE_NORMAL &&
                  state->saved_games->games[index].present),
                 time, clock, soundboard);
  const bool erasing = (state->mode == AZ_TMODE_ERASING &&
                        state->mode_data.erasing.slot_index == index);
  az_tick_button(&slot->confirm,
                 slot->x, slot->y, erasing, time, clock, soundboard);
  az_tick_button(&slot->cancel,
                 slot->x, slot->y, erasing, time, clock, soundboard);
}

void az_tick_title_state(az_title_state_t *state, double time) {
  ++state->clock;
  tick_mode(state, time);
  for (int i = 0; i < AZ_NUM_SAVED_GAME_SLOTS; ++i) {
    tick_save_slot(state, i, time, state->clock, &state->soundboard);
  }
  const bool prefs_and_about_active = prefs_about_buttons_active(state);
  az_tick_button(&state->prefs_button, 0, 0, prefs_and_about_active,
                 time, state->clock, &state->soundboard);
  az_tick_button(&state->about_button, 0, 0, prefs_and_about_active,
                 time, state->clock, &state->soundboard);
  az_tick_button(&state->quit_button, 0, 0, quit_button_active(state),
                 time, state->clock, &state->soundboard);
  az_tick_prefs_pane(&state->prefs_pane,
                     (state->mode == AZ_TMODE_PREFS ||
                      state->mode == AZ_TMODE_PICK_KEY),
                     time, state->clock, &state->soundboard);
}

/*===========================================================================*/

void az_title_skip_intro(az_title_state_t *state) {
  assert(state->mode == AZ_TMODE_INTRO);
  state->mode = AZ_TMODE_READY;
  state->mode_data.ready.start = state->clock;
}

void az_title_start_game(az_title_state_t *state, int slot_index) {
  assert(state->mode == AZ_TMODE_NORMAL);
  az_press_button(&state->slots[slot_index].main);
  state->mode = AZ_TMODE_STARTING;
  state->mode_data.starting.progress = 0.0;
  state->mode_data.starting.slot_index = slot_index;
  az_stop_music(&state->soundboard, STARTING_TIME);
}

/*===========================================================================*/

static void save_slot_on_click(az_title_state_t *state, int index,
                               int x, int y) {
  az_title_save_slot_t *slot = &state->slots[index];
  if (state->mode == AZ_TMODE_NORMAL) {
    if (az_button_on_click(&slot->main, x - slot->x, y - slot->y)) {
      az_title_start_game(state, index);
    }
    if (state->saved_games->games[index].present) {
      if (az_button_on_click(&slot->erase, x - slot->x, y - slot->y)) {
        state->mode = AZ_TMODE_ERASING;
        state->mode_data.erasing.slot_index = index;
        state->mode_data.erasing.do_erase = false;
      }
    }
  } else if (state->mode == AZ_TMODE_ERASING &&
             state->mode_data.erasing.slot_index == index) {
    if (az_button_on_click(&slot->confirm, x - slot->x, y - slot->y)) {
      state->mode_data.erasing.do_erase = true;
    }
    if (az_button_on_click(&slot->cancel, x - slot->x, y - slot->y)) {
      state->mode = AZ_TMODE_NORMAL;
    }
  }
}

void az_title_on_click(az_title_state_t *state, int x, int y) {
  if (state->mode == AZ_TMODE_INTRO) {
    az_title_skip_intro(state);
    return;
  }
  if (state->mode == AZ_TMODE_READY) {
    state->mode = AZ_TMODE_NORMAL;
    return;
  }

  for (int i = 0; i < AZ_NUM_SAVED_GAME_SLOTS; ++i) {
    save_slot_on_click(state, i, x, y);
  }

  if (prefs_about_buttons_active(state)) {
    if (az_button_on_click(&state->prefs_button, x, y)) {
      state->mode = (state->mode == AZ_TMODE_PREFS ? AZ_TMODE_NORMAL :
                     AZ_TMODE_PREFS);
    }
    if (az_button_on_click(&state->about_button, x, y)) {
      state->mode = (state->mode == AZ_TMODE_ABOUT ? AZ_TMODE_NORMAL :
                     AZ_TMODE_ABOUT);
    }
  }

  if (quit_button_active(state) &&
      az_button_on_click(&state->quit_button, x, y)) {
    state->mode = AZ_TMODE_QUITTING;
  }

  if (state->mode == AZ_TMODE_PREFS || state->mode == AZ_TMODE_PICK_KEY) {
    az_prefs_pane_on_click(&state->prefs_pane, x, y);
    if (state->prefs_pane.selected_key_picker_index >= 0) {
      state->mode = AZ_TMODE_PICK_KEY;
    }
  }
}

/*===========================================================================*/
