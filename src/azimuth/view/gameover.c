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

#include "azimuth/view/gameover.h"

#include <assert.h>
#include <math.h>
#include <string.h>

#include <GL/gl.h>

#include "azimuth/constants.h"
#include "azimuth/util/audio.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/polygon.h"
#include "azimuth/view/cursor.h"
#include "azimuth/view/string.h"

/*===========================================================================*/

#define BUTTONS_TOP 280
#define BUTTON_HEIGHT 30
#define BUTTON_SPACING 30
#define BUTTON_MARGIN 16

typedef enum {
  AZ_BUTTON_RETRY = 0,
  AZ_BUTTON_RETURN = 1,
  AZ_BUTTON_QUIT = 2
} az_button_id_t;

typedef struct {
  const char *label;
  int top;
  az_vector_t vertices[6];
  az_polygon_t polygon;
} az_button_info_t;

static az_button_info_t button_info[3];

static void init_button_info(az_button_id_t button_id, const char *label) {
  const int top = BUTTONS_TOP + button_id * (BUTTON_HEIGHT + BUTTON_SPACING);
  const int width = 2 * BUTTON_MARGIN + 16 * strlen(label);
  const int left = (AZ_SCREEN_WIDTH - width) / 2;

  button_info[button_id] = (az_button_info_t){
    .label = label, .top = top,
    .vertices = {
      {left + width - 7.5, top + 0.5},
      {left + width - 0.5, top + 0.5 * BUTTON_HEIGHT},
      {left + width - 7.5, top + BUTTON_HEIGHT - 0.5},
      {left + 7.5, top + BUTTON_HEIGHT - 0.5},
      {left + 0.5, top + 0.5 * BUTTON_HEIGHT},
      {left + 7.5, top + 0.5}
    },
    .polygon = AZ_INIT_POLYGON(button_info[button_id].vertices)
  };
}

static bool gameover_view_initialized = false;

void az_init_gameover_view(void) {
  if (gameover_view_initialized) return;
  gameover_view_initialized = true;
  init_button_info(AZ_BUTTON_RETRY, "Try again from last save point");
  init_button_info(AZ_BUTTON_RETURN, "Return to title screen");
  init_button_info(AZ_BUTTON_QUIT, "Quit game");
}

/*===========================================================================*/

static void do_polygon(GLenum mode, az_polygon_t polygon) {
  glBegin(mode); {
    for (int i = 0; i < polygon.num_vertices; ++i) {
      glVertex2d(polygon.vertices[i].x, polygon.vertices[i].y);
    }
  } glEnd();
}

static void draw_button(
    const az_gameover_state_t *state, const az_gameover_button_t *button,
    az_button_id_t button_id) {
  assert(gameover_view_initialized);
  const az_button_info_t *info = &button_info[button_id];
  const GLfloat pulse = button->hover_pulse;
  glColor4f(0.8f * pulse, pulse, pulse, 0.7f);
  do_polygon(GL_POLYGON, info->polygon);
  glColor3f(0.75, 0.75, 0.75); // light gray
  do_polygon(GL_LINE_LOOP, info->polygon);
  if (state->mode == AZ_GMODE_FADE_IN || state->mode == AZ_GMODE_NORMAL) {
    glColor3f(1, 1, 1); // white
  } else glColor3f(0.25, 0.25, 0.25); // dark gray
  az_draw_string(16, AZ_ALIGN_CENTER, AZ_SCREEN_WIDTH/2,
                 info->top + BUTTON_HEIGHT/2 - 8, info->label);
}

void az_gameover_draw_screen(const az_gameover_state_t *state) {
  // Draw pulsing background:
  glBegin(GL_QUADS); {
    glColor4f(0.5, 0, 1, 0);
    glVertex2i(0, 0);
    glVertex2i(AZ_SCREEN_WIDTH, 0);
    glColor4f(0.5, 0, 1, 0.1f + 0.003f * az_clock_zigzag(40, 2, state->clock));
    glVertex2i(AZ_SCREEN_WIDTH, AZ_SCREEN_HEIGHT);
    glVertex2i(0, AZ_SCREEN_HEIGHT);
  } glEnd();

  // Draw fixed text:
  glColor3f(1, 1, 1);
  az_draw_string(32, AZ_ALIGN_CENTER, AZ_SCREEN_WIDTH/2 - 1, 100, "GAME OVER");
  az_draw_string(32, AZ_ALIGN_CENTER, AZ_SCREEN_WIDTH/2 + 1, 100, "GAME OVER");
  az_draw_string(16, AZ_ALIGN_CENTER, AZ_SCREEN_WIDTH/2, 180,
                 "Rescue the Zenith colonists!");
  az_draw_string(16, AZ_ALIGN_CENTER, AZ_SCREEN_WIDTH/2, 210, "Try again?");

  // Draw buttons:
  draw_button(state, &state->retry_button, AZ_BUTTON_RETRY);
  draw_button(state, &state->return_button, AZ_BUTTON_RETURN);
  draw_button(state, &state->quit_button, AZ_BUTTON_QUIT);

  // Fade to black, if applicable:
  double fade_alpha = 0.0;
  switch (state->mode) {
    case AZ_GMODE_FADE_IN:
      fade_alpha = 1.0 - state->mode_progress;
      break;
    case AZ_GMODE_NORMAL: break;
    default:
      fade_alpha = state->mode_progress;
      break;
  }
  if (fade_alpha > 0.0) {
    glColor4f(0, 0, 0, fade_alpha);
    glBegin(GL_QUADS); {
      glVertex2i(0, 0);
      glVertex2i(AZ_SCREEN_WIDTH, 0);
      glVertex2i(AZ_SCREEN_WIDTH, AZ_SCREEN_HEIGHT);
      glVertex2i(0, AZ_SCREEN_HEIGHT);
    } glEnd();
  }

  az_draw_cursor();
}

/*===========================================================================*/

#define HOVER_PULSE_FRAMES 20
#define HOVER_PULSE_MIN 0.35
#define HOVER_PULSE_MAX 0.55
#define HOVER_PULSE_CLICK 1.0
#define HOVER_DECAY_TIME 0.7

static void tick_button(
    az_gameover_state_t *state, az_gameover_button_t *button, double time) {
  if ((state->mode == AZ_GMODE_FADE_IN || state->mode == AZ_GMODE_NORMAL) &&
      button->hovering) {
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

#define FADE_TIME 2.0

void az_tick_gameover_state(az_gameover_state_t *state, double time) {
  ++state->clock;

  if (state->mode == AZ_GMODE_NORMAL) {
    assert(state->mode_progress == 0.0);
  } else {
    state->mode_progress = fmin(1.0, state->mode_progress + time / FADE_TIME);
    if (state->mode == AZ_GMODE_FADE_IN && state->mode_progress >= 1.0) {
      state->mode = AZ_GMODE_NORMAL;
      state->mode_progress = 0.0;
    }
  }

  tick_button(state, &state->retry_button, time);
  tick_button(state, &state->return_button, time);
  tick_button(state, &state->quit_button, time);
}

/*===========================================================================*/

static bool point_in_button(
    const az_gameover_state_t *state, az_button_id_t button_id, int x, int y) {
  assert(gameover_view_initialized);
  return az_polygon_contains(button_info[button_id].polygon,
                             (az_vector_t){x, y});
}

static void button_on_hover(
    az_gameover_state_t *state, az_gameover_button_t *button,
    az_button_id_t button_id, int x, int y) {
  if ((state->mode == AZ_GMODE_FADE_IN || state->mode == AZ_GMODE_NORMAL) &&
      point_in_button(state, button_id, x, y)) {
    if (!button->hovering) {
      button->hovering = true;
      button->hover_start = state->clock;
      button->hover_pulse = HOVER_PULSE_MAX;
    }
  } else {
    button->hovering = false;
  }
}

void az_gameover_on_hover(az_gameover_state_t *state, int x, int y) {
  button_on_hover(state, &state->retry_button, AZ_BUTTON_RETRY, x, y);
  button_on_hover(state, &state->return_button, AZ_BUTTON_RETURN, x, y);
  button_on_hover(state, &state->quit_button, AZ_BUTTON_QUIT, x, y);
}

/*===========================================================================*/

static void button_on_click(
    az_gameover_state_t *state, az_button_id_t button_id, int x, int y) {
  if ((state->mode == AZ_GMODE_FADE_IN || state->mode == AZ_GMODE_NORMAL) &&
      point_in_button(state, button_id, x, y)) {
    switch (button_id) {
      case AZ_BUTTON_RETRY:
        state->retry_button.hover_pulse = HOVER_PULSE_CLICK;
        az_stop_music(&state->soundboard, FADE_TIME);
        state->mode = AZ_GMODE_RETRYING;
        break;
      case AZ_BUTTON_RETURN:
        state->return_button.hover_pulse = HOVER_PULSE_CLICK;
        state->mode = AZ_GMODE_RETURNING;
        break;
      case AZ_BUTTON_QUIT:
        state->quit_button.hover_pulse = HOVER_PULSE_CLICK;
        az_stop_music(&state->soundboard, FADE_TIME);
        state->mode = AZ_GMODE_QUITTING;
        break;
    }
  }
}

void az_gameover_on_click(az_gameover_state_t *state, int x, int y) {
  button_on_click(state, AZ_BUTTON_RETRY, x, y);
  button_on_click(state, AZ_BUTTON_RETURN, x, y);
  button_on_click(state, AZ_BUTTON_QUIT, x, y);
}

/*===========================================================================*/
