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
#include "azimuth/gui/event.h" // for az_is_mouse_held
#include "azimuth/state/player.h"
#include "azimuth/state/save.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/polygon.h"
#include "azimuth/util/vector.h"
#include "azimuth/view/cursor.h"
#include "azimuth/view/string.h"

/*===========================================================================*/

static void do_polygon(GLenum mode, az_polygon_t polygon) {
  glBegin(mode); {
    for (int i = 0; i < polygon.num_vertices; ++i) {
      glVertex2d(polygon.vertices[i].x, polygon.vertices[i].y);
    }
  } glEnd();
}

/*===========================================================================*/

#define PLANET_RADIUS 130.0
#define STAR_SPACING 12

// Below is a simple random number generator based on the MWC algorithm from:
//   http://www.bobwheeler.com/statistics/Password/MarsagliaPost.txt
// Using this rather than az_random() allows us to easily produce the same
// "random" sequence over and over, which we use to generate a starfield that
// looks random, but can be consistently regenerated on each frame.
static uint32_t sr_z, sr_w;
static void reset_simple_random(void) { sr_z = sr_w = 1; }
static double simple_random(void) {
  sr_z = 36969 * (sr_z & 0xffff) + (sr_z >> 16);
  sr_w = 18000 * (sr_w & 0xffff) + (sr_w >> 16);
  return ((sr_z << 16) + sr_w + 1.0) * 2.328306435454494e-10;
}

static void draw_background(const az_title_state_t *state) {
  // Draw stars:
  glBegin(GL_POINTS); {
    reset_simple_random();
    int i = 0;
    for (int xoff = 0; xoff < AZ_SCREEN_WIDTH; xoff += STAR_SPACING) {
      for (int yoff = 0; yoff < AZ_SCREEN_HEIGHT; yoff += STAR_SPACING) {
        const int twinkle = az_clock_zigzag(10, 4, state->clock + i);
        glColor4f(1, 1, 1, (twinkle * 0.02) + 0.3 * simple_random());
        const double x = xoff + 3 * STAR_SPACING * simple_random();
        const double y = yoff + 3 * STAR_SPACING * simple_random();
        glVertex2d(x, y);
        ++i;
      }
    }
  } glEnd();

  // Draw the planet:
  glPushMatrix(); {
    glTranslated(AZ_SCREEN_WIDTH/2, AZ_SCREEN_HEIGHT/2, 0);

    // Draw the planet itself:
    glBegin(GL_TRIANGLE_FAN); {
      glColor4f(0.1, 0, 0.1, 1);
      glVertex2d(0.15 * PLANET_RADIUS, -0.23 * PLANET_RADIUS);
      glColor4f(0.25, 0.15, 0.15, 1);
      for (int theta = 0; theta <= 360; theta += 6) {
        glVertex2d(PLANET_RADIUS * cos(AZ_DEG2RAD(theta)),
                   PLANET_RADIUS * sin(AZ_DEG2RAD(theta)));
      }
    } glEnd();

    // Draw planet "atmosphere":
    const double atmosphere_thickness =
      18.0 + az_clock_zigzag(10, 8, state->clock);
    glBegin(GL_TRIANGLE_STRIP); {
      for (int theta = 0; theta <= 360; theta += 6) {
        glColor4f(0, 1, 1, 0.5);
        glVertex2d(PLANET_RADIUS * cos(AZ_DEG2RAD(theta)),
                   PLANET_RADIUS * sin(AZ_DEG2RAD(theta)));
        glColor4f(0, 1, 1, 0);
        glVertex2d((PLANET_RADIUS + atmosphere_thickness) *
                   cos(AZ_DEG2RAD(theta + 3)),
                   (PLANET_RADIUS + atmosphere_thickness) *
                   sin(AZ_DEG2RAD(theta + 3)));
      }
    } glEnd();
  } glPopMatrix();

  // Draw game title:
  glColor3f(1, 1, 1); // white
  az_draw_string(80, AZ_ALIGN_CENTER, 320, 100, "Azimuth");
}

/*===========================================================================*/

#define PREFS_BOX_TOP 260
#define PREFS_BOX_WIDTH 512
#define PREFS_BOX_HEIGHT 146

#define SLIDER_WIDTH 300
#define SLIDER_THICKNESS 10
#define SLIDER_HANDLE_WIDTH 10
#define SLIDER_HANDLE_HEIGHT 20

typedef enum {
  AZ_SLIDER_MUSIC = 0,
  AZ_SLIDER_SOUND = 1
} az_slider_id_t;

static const az_vector_t slider_handle_vertices[] = {
  {0, 0.5 * SLIDER_HANDLE_HEIGHT},
  {-0.5 * SLIDER_HANDLE_WIDTH, 0.5 * SLIDER_HANDLE_HEIGHT - 5.0},
  {-0.5 * SLIDER_HANDLE_WIDTH, -0.5 * SLIDER_HANDLE_HEIGHT + 5.0},
  {0, -0.5 * SLIDER_HANDLE_HEIGHT},
  {0.5 * SLIDER_HANDLE_WIDTH, -0.5 * SLIDER_HANDLE_HEIGHT + 5.0},
  {0.5 * SLIDER_HANDLE_WIDTH, 0.5 * SLIDER_HANDLE_HEIGHT - 5.0}
};
static const az_polygon_t slider_handle_polygon =
  AZ_INIT_POLYGON(slider_handle_vertices);

static az_vector_t slider_midleft(az_slider_id_t slider_id) {
  const double x = 0.5 * (AZ_SCREEN_WIDTH - SLIDER_WIDTH);
  double y = PREFS_BOX_TOP + 50.0;
  switch (slider_id) {
    case AZ_SLIDER_MUSIC: break;
    case AZ_SLIDER_SOUND: y += 50.0; break;
  }
  return (az_vector_t){x, y};
}

static const az_title_slider_t *get_slider(const az_title_state_t *state,
                                           az_slider_id_t slider_id) {
  switch (slider_id) {
    case AZ_SLIDER_MUSIC: return &state->music_slider;
    case AZ_SLIDER_SOUND: return &state->sound_slider;
  }
  AZ_ASSERT_UNREACHABLE();
}

static bool point_in_slider_handle(const az_title_state_t *state,
                                   az_slider_id_t slider_id, int x, int y) {
  az_vector_t pt = {x, y};
  pt = az_vsub(pt, slider_midleft(slider_id));
  pt.x -= get_slider(state, slider_id)->value * SLIDER_WIDTH;
  return az_polygon_contains(slider_handle_polygon, pt);
}

static void draw_slider(const az_title_state_t *state,
                        az_slider_id_t slider_id) {
  const az_vector_t midleft = slider_midleft(slider_id);
  glPushMatrix(); {
    glTranslated(midleft.x, midleft.y, 0);
    const az_title_slider_t *slider = get_slider(state, slider_id);
    // Slider track:
    glBegin(GL_QUADS); {
      glColor3f(0.5, 0.5, 0.5); // gray
      glVertex2f(0, -2); glVertex2f(SLIDER_WIDTH, -2);
      glColor3f(0.25, 0.25, 0.25); // dark gray
      glVertex2f(SLIDER_WIDTH, 2); glVertex2f(0, 2);
    } glEnd();
    // Label:
    if (slider_id == AZ_SLIDER_MUSIC) glColor3f(0.75, 1, 1); // cyan
    else glColor3f(1, 0.75, 1); // magenta
    az_draw_string(8, AZ_ALIGN_RIGHT, -12, -4,
                   (slider_id == AZ_SLIDER_MUSIC ? "Music" : "Sounds"));
    // Numeric slider value:
    glColor3f(sqrt(1.0 - slider->value), sqrt(slider->value), 0);
    if (slider->value > 0) {
      az_draw_printf(8, AZ_ALIGN_RIGHT, SLIDER_WIDTH + 42, -4,
                     "%3d%%", 50 + (int)(100.0f * slider->value - 50.0f));
    } else {
      az_draw_string(8, AZ_ALIGN_RIGHT, SLIDER_WIDTH + 40, -4, "OFF");
    }
    // Slider handle:
    glPushMatrix(); {
      glTranslatef(slider->value * SLIDER_WIDTH, 0, 0);
      const GLfloat pulse = slider->handle.hover_pulse;
      glColor4f(0.7f * pulse, pulse, pulse, 0.7f);
      do_polygon(GL_POLYGON, slider_handle_polygon);
      glColor3f(0.75, 0.75, 0.75); // light gray
      do_polygon(GL_LINE_LOOP, slider_handle_polygon);
    } glPopMatrix();
  } glPopMatrix();
}

static const az_vector_t prefs_box_vertices[] = {
  {0.5, 0.5}, {PREFS_BOX_WIDTH - 0.5, 0.5},
  {PREFS_BOX_WIDTH - 0.5, PREFS_BOX_HEIGHT - 0.5},
  {0.5, PREFS_BOX_HEIGHT - 0.5}
};
static const az_polygon_t prefs_box_polygon =
  AZ_INIT_POLYGON(prefs_box_vertices);

static void draw_prefs_box(const az_title_state_t *state) {
  glPushMatrix(); {
    glTranslated(0.5 * (AZ_SCREEN_WIDTH - PREFS_BOX_WIDTH), PREFS_BOX_TOP, 0);
    glColor4f(0, 0, 0, 0.7); // black tint
    do_polygon(GL_POLYGON, prefs_box_polygon);
    glColor3f(0.75, 0.75, 0.75); // light gray
    do_polygon(GL_LINE_LOOP, prefs_box_polygon);
  } glPopMatrix();

  draw_slider(state, AZ_SLIDER_MUSIC);
  draw_slider(state, AZ_SLIDER_SOUND);
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

#define SAVE_SLOTS_TOP 250
#define SAVE_SLOT_WIDTH 160
#define SAVE_SLOT_HEIGHT 70
#define SAVE_SLOT_SPACING 20

static const az_vector_t save_slot_vertices[] = {
  {0.5, SAVE_SLOT_HEIGHT - 0.5}, {0.5, 0.5}, {SAVE_SLOT_WIDTH - 68.5, 0.5},
  {SAVE_SLOT_WIDTH - 50.5, 18.5}, {SAVE_SLOT_WIDTH - 0.5, 18.5},
  {SAVE_SLOT_WIDTH - 0.5, SAVE_SLOT_HEIGHT - 0.5}
};
static const az_polygon_t save_slot_polygon =
  AZ_INIT_POLYGON(save_slot_vertices);

static az_vector_t save_slot_topleft(int index) {
  const double margin = 0.5 * (AZ_SCREEN_WIDTH - 3 * SAVE_SLOT_WIDTH -
                               2 * SAVE_SLOT_SPACING);
  const int row = index / 3;
  const int col = index % 3;
  return (az_vector_t){margin + col * (SAVE_SLOT_WIDTH + SAVE_SLOT_SPACING),
      SAVE_SLOTS_TOP + row * (SAVE_SLOT_HEIGHT + SAVE_SLOT_SPACING)};
}

static bool point_in_save_slot(int index, int x, int y) {
  const az_vector_t pt = {x, y};
  return az_polygon_contains(save_slot_polygon,
                             az_vsub(pt, save_slot_topleft(index)));
}

static const az_vector_t save_slot_erase_vertices[] = {
  {SAVE_SLOT_WIDTH - 0.5, 0.5}, {SAVE_SLOT_WIDTH - 0.5, 16.5},
  {SAVE_SLOT_WIDTH - 49.5, 16.5}, {SAVE_SLOT_WIDTH - 65.5, 0.5}
};
static const az_polygon_t save_slot_erase_polygon =
  AZ_INIT_POLYGON(save_slot_erase_vertices);

static bool point_in_save_slot_erase(int index, int x, int y) {
  const az_vector_t pt = {x, y};
  return az_polygon_contains(save_slot_erase_polygon,
                             az_vsub(pt, save_slot_topleft(index)));
}

/*===========================================================================*/

static void draw_save_slot(const az_title_state_t *state, int index) {
  assert(index >= 0);
  assert(index < AZ_NUM_SAVED_GAME_SLOTS);
  const az_saved_game_t *saved_game = &state->saved_games->games[index];
  const az_title_save_slot_t *slot = &state->slots[index];

  const bool active =
    (state->mode == AZ_TMODE_NORMAL ||
     (state->mode == AZ_TMODE_ERASING &&
      state->mode_data.erasing.slot_index == index) ||
     (state->mode == AZ_TMODE_STARTING &&
      state->mode_data.starting.slot_index == index));

  // Draw slot frame and title:
  glColor4f(0.7*slot->main.hover_pulse, slot->main.hover_pulse,
            slot->main.hover_pulse, 0.7);
  do_polygon(GL_TRIANGLE_FAN, save_slot_polygon);
  glColor3f(0.75, 0.75, 0.75); // light gray
  do_polygon(GL_LINE_LOOP, save_slot_polygon);
  if (active) glColor3f(1, 1, 1); // white
  else glColor3f(0.25, 0.25, 0.25); // dark gray
  az_draw_printf(8, AZ_ALIGN_LEFT, 6, 6, "HOPPER %d", index + 1);

  if (saved_game->present) {
    // Draw "ERASE" button:
    glColor4f(slot->erase.hover_pulse, 0, 0, 0.7);
    do_polygon(GL_POLYGON, save_slot_erase_polygon);
    glColor3f(0.75, 0.75, 0.75); // light gray
    do_polygon(GL_LINE_LOOP, save_slot_erase_polygon);
    if (active && state->mode != AZ_TMODE_ERASING) {
      glColor3f(0.75, 0, 0); // red
    } else glColor3f(0.25, 0.25, 0.25); // dark gray
    az_draw_string(8, AZ_ALIGN_RIGHT, SAVE_SLOT_WIDTH - 5, 5, "ERASE");

    // If we're in ERASING mode for this save slot, draw the confirmation UI:
    if (state->mode == AZ_TMODE_ERASING &&
        state->mode_data.erasing.slot_index == index) {
      assert(active);
      glColor3f(1, 1, 1); // white
      az_draw_string(8, AZ_ALIGN_CENTER, SAVE_SLOT_WIDTH/2,
                     SAVE_SLOT_HEIGHT/2 - 4, "Are you sure?");
      if (state->confirm_button.hovering) {
        glColor3f(1, state->confirm_button.hover_pulse, 0);
      } else glColor3f(0.3, 0, 0);
      az_draw_string(8, AZ_ALIGN_CENTER, SAVE_SLOT_WIDTH/4,
                     SAVE_SLOT_HEIGHT/2 + 14, "ERASE");
      if (state->cancel_button.hovering) {
        glColor3f(state->cancel_button.hover_pulse, 1, 0);
      } else glColor3f(0, 0.3, 0);
      az_draw_string(8, AZ_ALIGN_CENTER, 3*SAVE_SLOT_WIDTH/4,
                     SAVE_SLOT_HEIGHT/2 + 14, "CANCEL");
    }
    // Otherwise, draw save file info:
    else {
      const az_player_t *player = &saved_game->player;

      // Draw the name of the zone the player is in for this save file:
      const az_room_t *room = &state->planet->rooms[player->current_room];
      assert(0 <= room->zone_key);
      assert(room->zone_key < state->planet->num_zones);
      const az_zone_t *zone = &state->planet->zones[room->zone_key];
      if (active) glColor3ub(zone->color.r, zone->color.g, zone->color.b);
      else glColor3ub(zone->color.r / 3, zone->color.g / 3, zone->color.b / 3);
      az_draw_string(8, AZ_ALIGN_LEFT, 6, 24, zone->name);

      // Draw the elapsed time of this save file, as "hours:minutes":
      if (active) glColor3f(1, 1, 1); // white
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
  } else {
    if (active) glColor3f(0.5, 0.5, 0.5); // gray
    else glColor3f(0.25, 0.25, 0.25); // dark gray
    az_draw_string(8, AZ_ALIGN_CENTER, SAVE_SLOT_WIDTH/2,
                   SAVE_SLOT_HEIGHT/2 + 4, "New Game");
  }
}

/*===========================================================================*/

#define BUTTON_WIDTH 100
#define BUTTON_HEIGHT 20
#define BUTTON_SPACING 54

typedef enum {
  AZ_BUTTON_PREFS = 0,
  AZ_BUTTON_ABOUT = 1,
  AZ_BUTTON_QUIT = 2,
  AZ_BUTTON_CONFIRM,
  AZ_BUTTON_CANCEL
} az_button_id_t;

static const az_vector_t button_vertices[] = {
  {BUTTON_WIDTH - 5.5, 0.5}, {BUTTON_WIDTH - 0.5, 0.5 * BUTTON_HEIGHT},
  {BUTTON_WIDTH - 5.5, BUTTON_HEIGHT - 0.5},
  {5.5, BUTTON_HEIGHT - 0.5}, {0.5, 0.5 * BUTTON_HEIGHT}, {5.5, 0.5}
};
static const az_polygon_t button_polygon = AZ_INIT_POLYGON(button_vertices);

static az_vector_t bottom_button_topleft(az_button_id_t button_id) {
  assert(button_id == AZ_BUTTON_PREFS || button_id == AZ_BUTTON_ABOUT ||
         button_id == AZ_BUTTON_QUIT);
  const double x =
    0.5 * (AZ_SCREEN_WIDTH - 3 * BUTTON_WIDTH - 2 * BUTTON_SPACING) +
    (int)button_id * (BUTTON_WIDTH + BUTTON_SPACING);
  const double y = SAVE_SLOTS_TOP + 2 * (SAVE_SLOT_HEIGHT + SAVE_SLOT_SPACING);
  return (az_vector_t){x, y};
}

static bool point_in_button(const az_title_state_t *state,
                            az_button_id_t button_id, int x, int y) {
  az_vector_t pt = {x, y};
  switch (button_id) {
    case AZ_BUTTON_PREFS:
    case AZ_BUTTON_ABOUT:
    case AZ_BUTTON_QUIT:
      return az_polygon_contains(button_polygon,
          az_vsub(pt, bottom_button_topleft(button_id)));
    case AZ_BUTTON_CONFIRM:
    case AZ_BUTTON_CANCEL:
      if (state->mode != AZ_TMODE_ERASING) return false;
      pt = az_vsub(pt, save_slot_topleft(state->mode_data.erasing.slot_index));
      pt.y -= SAVE_SLOT_HEIGHT/2 + 14;
      pt.x -= (button_id == AZ_BUTTON_CONFIRM ? SAVE_SLOT_WIDTH/4 :
               3*SAVE_SLOT_WIDTH/4);
      return (az_vnorm(pt) <= 24.0);
  }
  AZ_ASSERT_UNREACHABLE();
}

static bool button_is_active(const az_title_state_t *state,
                             az_button_id_t button_id) {
  switch (button_id) {
    case AZ_BUTTON_PREFS:
    case AZ_BUTTON_ABOUT:
      return (state->mode == AZ_TMODE_NORMAL ||
              state->mode == AZ_TMODE_PREFS ||
              state->mode == AZ_TMODE_ABOUT);
    case AZ_BUTTON_QUIT:
      return (state->mode == AZ_TMODE_NORMAL ||
              state->mode == AZ_TMODE_PREFS ||
              state->mode == AZ_TMODE_ABOUT ||
              state->mode == AZ_TMODE_ERASING);
    case AZ_BUTTON_CONFIRM:
    case AZ_BUTTON_CANCEL:
      return state->mode == AZ_TMODE_ERASING;
  }
  AZ_ASSERT_UNREACHABLE();
}

static void reset_button(az_title_button_t *button) {
  button->hovering = false;
  button->hover_pulse = 0.0;
}

/*===========================================================================*/

static void draw_bottom_button(
    const az_title_state_t *state, const az_title_button_t *button,
    az_button_id_t button_id, const char *label) {
  assert(button_id == AZ_BUTTON_PREFS || button_id == AZ_BUTTON_ABOUT ||
         button_id == AZ_BUTTON_QUIT);
  glPushMatrix(); {
    const az_vector_t topleft = bottom_button_topleft(button_id);
    glTranslated(topleft.x, topleft.y, 0);
    const bool active = button_is_active(state, button_id);
    const GLfloat pulse = button->hover_pulse;
    glColor4f(0.8f * pulse, pulse, pulse, 0.7f);
    do_polygon(GL_POLYGON, button_polygon);
    glColor3f(0.75, 0.75, 0.75); // light gray
    do_polygon(GL_LINE_LOOP, button_polygon);
    if (active) glColor3f(1, 1, 1); // white
    else glColor3f(0.25, 0.25, 0.25); // dark gray
    az_draw_string(8, AZ_ALIGN_CENTER, BUTTON_WIDTH/2, BUTTON_HEIGHT/2 - 4,
                   label);
  } glPopMatrix();
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

  // Draw save slots:
  if (state->mode == AZ_TMODE_PREFS) {
    draw_prefs_box(state);
  } else if (state->mode == AZ_TMODE_ABOUT) {
    draw_about_box();
  } else {
    for (int i = 0; i < AZ_ARRAY_SIZE(state->saved_games->games); ++i) {
      glPushMatrix(); {
        const az_vector_t topleft = save_slot_topleft(i);
        glTranslated(topleft.x, topleft.y, 0);
        draw_save_slot(state, i);
      } glPopMatrix();
    }
  }

  // Draw bottom three buttons:
  draw_bottom_button(state, &state->prefs_button, AZ_BUTTON_PREFS, "Prefs");
  draw_bottom_button(state, &state->about_button, AZ_BUTTON_ABOUT, "About");
  draw_bottom_button(state, &state->quit_button, AZ_BUTTON_QUIT, "Quit");

  // Fade out screen:
  if (state->mode == AZ_TMODE_STARTING) {
    fade_screen_black(state->mode_data.starting.progress);
  }

  az_draw_cursor();
}

/*===========================================================================*/

#define STARTING_TIME 0.9

static void tick_mode(az_title_state_t *state, double time) {
  switch (state->mode) {
    case AZ_TMODE_STARTING:
      state->mode_data.starting.progress =
        fmin(1.0, state->mode_data.starting.progress + time / STARTING_TIME);
      break;
    default: break;
  }
}

#define HOVER_PULSE_FRAMES 20
#define HOVER_PULSE_MIN 0.35
#define HOVER_PULSE_MAX 0.55
#define HOVER_PULSE_CLICK 1.0
#define HOVER_DECAY_TIME 0.7

static void update_hover_pulse(
    az_title_state_t *state, az_title_button_t *button, bool active,
    double time) {
  if (active && button->hovering) {
    if (button->hover_pulse > HOVER_PULSE_MAX) {
      button->hover_pulse -= time / HOVER_DECAY_TIME;
      if (button->hover_pulse <= HOVER_PULSE_MAX) {
        button->hover_pulse = HOVER_PULSE_MAX;
        button->hover_start = state->clock;
      }
    } else {
      button->hover_pulse = HOVER_PULSE_MIN +
        (HOVER_PULSE_MAX - HOVER_PULSE_MIN) *
        (double)az_clock_zigzag(HOVER_PULSE_FRAMES, 1,
                                state->clock - button->hover_start) /
        (double)HOVER_PULSE_FRAMES;
    }
  } else {
    button->hover_pulse =
      fmax(0.0, button->hover_pulse - time / HOVER_DECAY_TIME);
  }
}

static void tick_button(az_title_state_t *state, az_title_button_t *button,
                        az_button_id_t button_id, double time) {
  update_hover_pulse(state, button, button_is_active(state, button_id), time);
}

static void tick_save_slot(az_title_state_t *state, int index, double time) {
  az_title_save_slot_t *slot = &state->slots[index];
  const bool active = (state->mode == AZ_TMODE_NORMAL);
  update_hover_pulse(state, &slot->main, active, time);
  update_hover_pulse(state, &slot->erase, active, time);
}

static void tick_slider(az_title_state_t *state, az_title_slider_t *slider,
                        az_slider_id_t slider_id, double time) {
  if (slider->grabbed) {
    slider->handle.hover_pulse = HOVER_PULSE_CLICK;
    if (!az_is_mouse_held()) {
      slider->grabbed = false;
      int x, y;
      slider->handle.hovering = az_get_mouse_position(&x, &y) &&
        point_in_slider_handle(state, slider_id, x, y);
      if (slider_id == AZ_SLIDER_SOUND) {
        az_play_sound(&state->soundboard, AZ_SND_BLINK_MEGA_BOMB);
      }
    }
  } else {
    update_hover_pulse(state, &slider->handle, state->mode == AZ_TMODE_PREFS,
                       time);
  }
}

void az_tick_title_state(az_title_state_t *state, double time) {
  ++state->clock;
  tick_mode(state, time);
  for (int i = 0; i < AZ_NUM_SAVED_GAME_SLOTS; ++i) {
    if (state->mode != AZ_TMODE_STARTING ||
        state->mode_data.starting.slot_index != i) {
      tick_save_slot(state, i, time);
    }
  }
  tick_button(state, &state->prefs_button, AZ_BUTTON_PREFS, time);
  tick_button(state, &state->about_button, AZ_BUTTON_ABOUT, time);
  tick_button(state, &state->quit_button, AZ_BUTTON_QUIT, time);
  tick_button(state, &state->confirm_button, AZ_BUTTON_CONFIRM, time);
  tick_button(state, &state->cancel_button, AZ_BUTTON_CANCEL, time);
  tick_slider(state, &state->music_slider, AZ_SLIDER_MUSIC, time);
  tick_slider(state, &state->sound_slider, AZ_SLIDER_SOUND, time);
}

/*===========================================================================*/

void az_title_start_game(az_title_state_t *state, int slot_index) {
  assert(state->mode == AZ_TMODE_NORMAL);
  state->slots[slot_index].main.hover_pulse = HOVER_PULSE_CLICK;
  state->mode = AZ_TMODE_STARTING;
  state->mode_data.starting.progress = 0.0;
  state->mode_data.starting.slot_index = slot_index;
  az_stop_music(&state->soundboard, STARTING_TIME);
}

/*===========================================================================*/

static void set_hovering(az_title_state_t *state, az_title_button_t *button,
                         bool active, bool hovering) {
  if (active && hovering) {
    if (!button->hovering) {
      button->hovering = true;
      button->hover_start = state->clock;
      button->hover_pulse = HOVER_PULSE_MAX;
    }
  } else {
    button->hovering = false;
  }
}

static void button_on_hover(az_title_state_t *state, az_title_button_t *button,
                            az_button_id_t button_id, int x, int y) {
  set_hovering(state, button, button_is_active(state, button_id),
               point_in_button(state, button_id, x, y));
}

static void slider_on_hover(az_title_state_t *state, az_title_slider_t *slider,
                            az_slider_id_t slider_id, int x, int y) {
  if (slider->grabbed) {
    const double rel_x = fmin(fmax(0.0, x - slider_midleft(slider_id).x),
                              SLIDER_WIDTH);
    slider->value = rel_x / SLIDER_WIDTH;
  } else {
    set_hovering(state, &slider->handle, state->mode == AZ_TMODE_PREFS,
                 point_in_slider_handle(state, slider_id, x, y));
  }
}

void az_title_on_hover(az_title_state_t *state, int x, int y) {
  for (int i = 0; i < AZ_NUM_SAVED_GAME_SLOTS; ++i) {
    az_title_save_slot_t *slot = &state->slots[i];
    set_hovering(state, &slot->main, state->mode == AZ_TMODE_NORMAL,
                 point_in_save_slot(i, x, y));
    set_hovering(state, &slot->erase, state->mode == AZ_TMODE_NORMAL,
                 point_in_save_slot_erase(i, x, y));
  }
  button_on_hover(state, &state->prefs_button, AZ_BUTTON_PREFS, x, y);
  button_on_hover(state, &state->about_button, AZ_BUTTON_ABOUT, x, y);
  button_on_hover(state, &state->quit_button, AZ_BUTTON_QUIT, x, y);
  button_on_hover(state, &state->confirm_button, AZ_BUTTON_CONFIRM, x, y);
  button_on_hover(state, &state->cancel_button, AZ_BUTTON_CANCEL, x, y);
  slider_on_hover(state, &state->music_slider, AZ_SLIDER_MUSIC, x, y);
  slider_on_hover(state, &state->sound_slider, AZ_SLIDER_SOUND, x, y);
}

/*===========================================================================*/

static void button_on_click(az_title_state_t *state, az_button_id_t button_id,
                            int x, int y) {
  if (button_is_active(state, button_id) &&
      point_in_button(state, button_id, x, y)) {
    switch (button_id) {
      case AZ_BUTTON_PREFS:
        state->prefs_button.hover_pulse = HOVER_PULSE_CLICK;
        state->mode = (state->mode == AZ_TMODE_PREFS ? AZ_TMODE_NORMAL :
                       AZ_TMODE_PREFS);
        break;
      case AZ_BUTTON_ABOUT:
        state->about_button.hover_pulse = HOVER_PULSE_CLICK;
        state->mode = (state->mode == AZ_TMODE_ABOUT ? AZ_TMODE_NORMAL :
                       AZ_TMODE_ABOUT);
        break;
      case AZ_BUTTON_QUIT:
        state->quit_button.hover_pulse = HOVER_PULSE_CLICK;
        state->mode = AZ_TMODE_QUITTING;
        break;
      case AZ_BUTTON_CONFIRM:
        assert(state->mode == AZ_TMODE_ERASING);
        state->mode_data.erasing.do_erase = true;
        break;
      case AZ_BUTTON_CANCEL:
        assert(state->mode == AZ_TMODE_ERASING);
        state->mode = AZ_TMODE_NORMAL;
        break;
    }
  }
}

static void slider_on_click(az_title_state_t *state, az_title_slider_t *slider,
                            az_slider_id_t slider_id, int x, int y) {
  if (state->mode == AZ_TMODE_PREFS &&
      point_in_slider_handle(state, slider_id, x, y)) {
    slider->grabbed = true;
    slider->handle.hover_pulse = HOVER_PULSE_CLICK;
  }
}

void az_title_on_click(az_title_state_t *state, int x, int y) {
  if (state->mode == AZ_TMODE_NORMAL) {
    for (int i = 0; i < AZ_NUM_SAVED_GAME_SLOTS; ++i) {
      if (state->saved_games->games[i].present &&
          point_in_save_slot_erase(i, x, y)) {
        state->slots[i].erase.hover_pulse = HOVER_PULSE_CLICK;
        state->mode = AZ_TMODE_ERASING;
        state->mode_data.erasing.slot_index = i;
        state->mode_data.erasing.do_erase = false;
        reset_button(&state->confirm_button);
        reset_button(&state->cancel_button);
      } else if (point_in_save_slot(i, x, y)) {
        az_title_start_game(state, i);
      }
    }
  }
  button_on_click(state, AZ_BUTTON_PREFS, x, y);
  button_on_click(state, AZ_BUTTON_ABOUT, x, y);
  button_on_click(state, AZ_BUTTON_QUIT, x, y);
  button_on_click(state, AZ_BUTTON_CONFIRM, x, y);
  button_on_click(state, AZ_BUTTON_CANCEL, x, y);
  slider_on_click(state, &state->music_slider, AZ_SLIDER_MUSIC, x, y);
  slider_on_click(state, &state->sound_slider, AZ_SLIDER_SOUND, x, y);
}

/*===========================================================================*/
