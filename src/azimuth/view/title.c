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
#include "azimuth/state/player.h"
#include "azimuth/state/save.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/polygon.h"
#include "azimuth/util/vector.h"
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
  glColor4f(0.7*slot->main_hover_pulse, slot->main_hover_pulse,
            slot->main_hover_pulse, 0.7);
  do_polygon(GL_TRIANGLE_FAN, save_slot_polygon);
  glColor3f(0.75, 0.75, 0.75); // light gray
  do_polygon(GL_LINE_LOOP, save_slot_polygon);
  if (active) glColor3f(1, 1, 1); // white
  else glColor3f(0.25, 0.25, 0.25); // dark gray
  az_draw_printf(8, AZ_ALIGN_LEFT, 6, 6, "HOPPER %d", index + 1);

  if (saved_game->present) {
    // Draw "ERASE" button:
    glColor4f(slot->erase_hover_pulse, 0, 0, 0.7);
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
      // TODO don't hardcode name/color for this
      if (active) glColor3f(1, 1, 0); // yellow
      else glColor3f(0.3, 0.3, 0); // dark yellow
      az_draw_string(8, AZ_ALIGN_LEFT, 6, 24, "Nandiar");

      if (active) glColor3f(1, 1, 1); // white
      else glColor3f(0.25, 0.25, 0.25); // dark gray
      // Draw the elapsed time of this save file, as "hours:minutes":
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
#define BUTTON_SPACING 120

typedef enum {
  AZ_BUTTON_ABOUT,
  AZ_BUTTON_QUIT,
  AZ_BUTTON_CONFIRM,
  AZ_BUTTON_CANCEL
} az_button_id_t;

static const az_vector_t button_vertices[] = {
  {BUTTON_WIDTH - 5.5, 0.5}, {BUTTON_WIDTH - 0.5, 0.5 * BUTTON_HEIGHT},
  {BUTTON_WIDTH - 5.5, BUTTON_HEIGHT - 0.5},
  {5.5, BUTTON_HEIGHT - 0.5}, {0.5, 0.5 * BUTTON_HEIGHT}, {5.5, 0.5}
};
static const az_polygon_t button_polygon = AZ_INIT_POLYGON(button_vertices);

static az_vector_t about_quit_button_topleft(bool is_quit) {
  return (az_vector_t){
    0.5 * (AZ_SCREEN_WIDTH - 2 * BUTTON_WIDTH - BUTTON_SPACING) +
      (is_quit ? BUTTON_WIDTH + BUTTON_SPACING : 0),
      SAVE_SLOTS_TOP + 2 * (SAVE_SLOT_HEIGHT + SAVE_SLOT_SPACING)};
}

static bool point_in_button(const az_title_state_t *state,
                            az_button_id_t button_id, int x, int y) {
  az_vector_t pt = {x, y};
  if (button_id == AZ_BUTTON_ABOUT || button_id == AZ_BUTTON_QUIT) {
    return az_polygon_contains(button_polygon,
        az_vsub(pt, about_quit_button_topleft(button_id == AZ_BUTTON_QUIT)));
  } else {
    assert(button_id == AZ_BUTTON_CONFIRM || button_id == AZ_BUTTON_CANCEL);
    if (state->mode != AZ_TMODE_ERASING) return false;
    pt = az_vsub(pt, save_slot_topleft(state->mode_data.erasing.slot_index));
    pt.y -= SAVE_SLOT_HEIGHT/2 + 14;
    pt.x -= (button_id == AZ_BUTTON_CONFIRM ? SAVE_SLOT_WIDTH/4 :
             3*SAVE_SLOT_WIDTH/4);
    return (az_vnorm(pt) <= 24.0);
  }
}

static bool button_is_active(const az_title_state_t *state,
                             az_button_id_t button_id) {
  switch (button_id) {
    case AZ_BUTTON_ABOUT:
      return (state->mode == AZ_TMODE_NORMAL || state->mode == AZ_TMODE_ABOUT);
    case AZ_BUTTON_QUIT:
      return (state->mode == AZ_TMODE_NORMAL ||
              state->mode == AZ_TMODE_ABOUT ||
              state->mode == AZ_TMODE_ERASING);
    case AZ_BUTTON_CONFIRM:
    case AZ_BUTTON_CANCEL:
      return state->mode == AZ_TMODE_ERASING;
  }
  assert(false); // unreachable
  return false;
}

static void reset_button(az_title_button_t *button) {
  button->hovering = false;
  button->hover_pulse = 0.0;
}

/*===========================================================================*/

static void draw_about_quit_button(const az_title_state_t *state,
                                   bool is_quit) {
  glPushMatrix(); {
    const az_vector_t topleft = about_quit_button_topleft(is_quit);
    glTranslated(topleft.x, topleft.y, 0);

    const az_title_button_t *button =
      (is_quit ? &state->quit_button : &state->about_button);
    const bool active =
      button_is_active(state, is_quit ? AZ_BUTTON_QUIT : AZ_BUTTON_ABOUT);
    glColor4f(0.8 * button->hover_pulse, button->hover_pulse,
              button->hover_pulse, 0.7);
    do_polygon(GL_POLYGON, button_polygon);
    glColor3f(0.75, 0.75, 0.75); // light gray
    do_polygon(GL_LINE_LOOP, button_polygon);
    if (active) glColor3f(1, 1, 1); // white
    else glColor3f(0.25, 0.25, 0.25); // dark gray
    az_draw_string(8, AZ_ALIGN_CENTER, BUTTON_WIDTH/2, BUTTON_HEIGHT/2 - 4,
                   (is_quit ? "Quit" : "About"));
  } glPopMatrix();
}

/*===========================================================================*/

static void fade_screen_black(float alpha) {
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
  if (state->mode == AZ_TMODE_ABOUT) {
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

  // Draw "About" and "Quit" buttons:
  draw_about_quit_button(state, false);
  draw_about_quit_button(state, true);

  // Fade out screen:
  if (state->mode == AZ_TMODE_STARTING) {
    fade_screen_black(state->mode_data.starting.progress);
  }
}

/*===========================================================================*/

#define STARTING_TIME 0.9
#define QUITTING_TIME 0.5

static void tick_mode(az_title_state_t *state, double time) {
  switch (state->mode) {
    case AZ_TMODE_STARTING:
      state->mode_data.starting.progress =
        az_dmin(1.0, state->mode_data.starting.progress +
                time / STARTING_TIME);
      break;
    default: break;
  }
}

#define HOVER_PULSE_FRAMES 20
#define HOVER_PULSE_MIN 0.35
#define HOVER_PULSE_MAX 0.55
#define HOVER_PULSE_CLICK 1.0
#define HOVER_DECAY_TIME 0.7

static void tick_save_slot(az_title_state_t *state, int index, double time) {
  az_title_save_slot_t *slot = &state->slots[index];
  // Main hover:
  if (state->mode == AZ_TMODE_NORMAL && slot->hover == AZ_TSS_HOVER_MAIN) {
    slot->main_hover_pulse = HOVER_PULSE_MIN +
      (HOVER_PULSE_MAX - HOVER_PULSE_MIN) *
      (double)az_clock_zigzag(HOVER_PULSE_FRAMES, 1,
                              state->clock - slot->hover_start) /
      (double)HOVER_PULSE_FRAMES;
  } else {
    slot->main_hover_pulse =
      az_dmax(0.0, slot->main_hover_pulse - time / HOVER_DECAY_TIME);
  }
  // Erase hover:
  if (state->mode == AZ_TMODE_NORMAL && slot->hover == AZ_TSS_HOVER_ERASE) {
    slot->erase_hover_pulse = HOVER_PULSE_MIN +
      (HOVER_PULSE_MAX - HOVER_PULSE_MIN) *
      (double)az_clock_zigzag(HOVER_PULSE_FRAMES, 1,
                              state->clock - slot->hover_start) /
      (double)HOVER_PULSE_FRAMES;
  } else {
    slot->erase_hover_pulse =
      az_dmax(0.0, slot->erase_hover_pulse - time / HOVER_DECAY_TIME);
  }
}

static void tick_button(az_title_state_t *state, az_title_button_t *button,
                        az_button_id_t button_id, double time) {
  if (button_is_active(state, button_id) && button->hovering) {
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
      az_dmax(0.0, button->hover_pulse - time / HOVER_DECAY_TIME);
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
  tick_button(state, &state->about_button, AZ_BUTTON_ABOUT, time);
  tick_button(state, &state->quit_button, AZ_BUTTON_QUIT, time);
  tick_button(state, &state->confirm_button, AZ_BUTTON_CONFIRM, time);
  tick_button(state, &state->cancel_button, AZ_BUTTON_CANCEL, time);
}

/*===========================================================================*/

static void button_on_hover(az_title_state_t *state, az_title_button_t *button,
                            az_button_id_t button_id, int x, int y) {
  const bool active = button_is_active(state, button_id);
  if (active && point_in_button(state, button_id, x, y)) {
    if (!button->hovering) {
      button->hovering = true;
      button->hover_start = state->clock;
      button->hover_pulse = HOVER_PULSE_MAX;
    }
  } else {
    button->hovering = false;
  }
}

void az_title_on_hover(az_title_state_t *state, int x, int y) {
  for (int i = 0; i < AZ_NUM_SAVED_GAME_SLOTS; ++i) {
    if (state->mode != AZ_TMODE_NORMAL) {
      state->slots[i].hover = AZ_TSS_HOVER_NONE;
    } else if (point_in_save_slot_erase(i, x, y)) {
      if (state->slots[i].hover != AZ_TSS_HOVER_ERASE) {
        state->slots[i].hover = AZ_TSS_HOVER_ERASE;
        state->slots[i].hover_start = state->clock;
        state->slots[i].erase_hover_pulse = HOVER_PULSE_MAX;
      }
    } else if (point_in_save_slot(i, x, y)) {
      if (state->slots[i].hover != AZ_TSS_HOVER_MAIN) {
        state->slots[i].hover = AZ_TSS_HOVER_MAIN;
        state->slots[i].hover_start = state->clock;
        state->slots[i].main_hover_pulse = HOVER_PULSE_MAX;
      }
    } else {
      state->slots[i].hover = AZ_TSS_HOVER_NONE;
    }
  }
  button_on_hover(state, &state->about_button, AZ_BUTTON_ABOUT, x, y);
  button_on_hover(state, &state->quit_button, AZ_BUTTON_QUIT, x, y);
  button_on_hover(state, &state->confirm_button, AZ_BUTTON_CONFIRM, x, y);
  button_on_hover(state, &state->cancel_button, AZ_BUTTON_CANCEL, x, y);
}

static void button_on_click(az_title_state_t *state, az_button_id_t button_id,
                            int x, int y) {
  if (button_is_active(state, button_id) &&
      point_in_button(state, button_id, x, y)) {
    switch (button_id) {
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

void az_title_on_click(az_title_state_t *state, int x, int y) {
  if (state->mode == AZ_TMODE_NORMAL) {
    for (int i = 0; i < AZ_NUM_SAVED_GAME_SLOTS; ++i) {
      if (point_in_save_slot_erase(i, x, y)) {
        state->slots[i].erase_hover_pulse = HOVER_PULSE_CLICK;
        state->mode = AZ_TMODE_ERASING;
        state->mode_data.erasing.slot_index = i;
        state->mode_data.erasing.do_erase = false;
        reset_button(&state->confirm_button);
        reset_button(&state->cancel_button);
      } else if (point_in_save_slot(i, x, y)) {
        state->slots[i].main_hover_pulse = HOVER_PULSE_CLICK;
        state->mode = AZ_TMODE_STARTING;
        state->mode_data.starting.progress = 0.0;
        state->mode_data.starting.slot_index = i;
      }
    }
  }
  button_on_click(state, AZ_BUTTON_ABOUT, x, y);
  button_on_click(state, AZ_BUTTON_QUIT, x, y);
  button_on_click(state, AZ_BUTTON_CONFIRM, x, y);
  button_on_click(state, AZ_BUTTON_CANCEL, x, y);
}

/*===========================================================================*/
