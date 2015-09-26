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
#include <stdbool.h>
#include <string.h>

#include <GL/gl.h>

#include "azimuth/constants.h"
#include "azimuth/state/sound.h"
#include "azimuth/util/audio.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/polygon.h"
#include "azimuth/util/vector.h"
#include "azimuth/view/cursor.h"
#include "azimuth/view/cutscene.h"
#include "azimuth/view/string.h"

/*===========================================================================*/

#define BUTTONS_TOP 280
#define BUTTON_WIDTH 408
#define BUTTON_HEIGHT 30
#define BUTTON_SPACING 30

static const char retry_button_label[] = "Continue from last save";
static const char return_button_label[] = "Return to title screen";
static const char quit_button_label[] = "Quit game";

static const az_vector_t button_vertices[] = {
  {BUTTON_WIDTH - 7.5, 0.5}, {BUTTON_WIDTH - 0.5, 0.5 * BUTTON_HEIGHT},
  {BUTTON_WIDTH - 7.5, BUTTON_HEIGHT - 0.5}, {7.5, BUTTON_HEIGHT - 0.5},
  {0.5, 0.5 * BUTTON_HEIGHT}, {7.5, 0.5}
};

void az_init_gameover_state(az_gameover_state_t *state) {
  AZ_ZERO_OBJECT(state);
  const int button_x = (AZ_SCREEN_WIDTH - BUTTON_WIDTH) / 2;
  const az_polygon_t button_polygon = AZ_INIT_POLYGON(button_vertices);
  az_init_button(&state->retry_button, button_polygon, button_x, BUTTONS_TOP);
  az_init_button(&state->return_button, button_polygon, button_x,
                 BUTTONS_TOP + BUTTON_HEIGHT + BUTTON_SPACING);
  az_init_button(&state->quit_button, button_polygon, button_x,
                 BUTTONS_TOP + 2 * (BUTTON_HEIGHT + BUTTON_SPACING));
}

/*===========================================================================*/

static bool buttons_are_active(const az_gameover_state_t *state) {
  return state->mode == AZ_GMODE_FADE_IN || state->mode == AZ_GMODE_NORMAL;
}

static void draw_button(const az_gameover_state_t *state,
                        const az_button_t *button, const char *label) {
  az_draw_standard_button(button);
  if (buttons_are_active(state)) {
    glColor3f(1, 1, 1);
  } else glColor3f(0.25, 0.25, 0.25);
  az_draw_string(16, AZ_ALIGN_CENTER, button->x + BUTTON_WIDTH/2,
                 button->y + BUTTON_HEIGHT/2 - 8, label);
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

  az_draw_planet_starfield(state->clock);

  // Draw fixed text:
  glColor3f(1, 1, 1);
  az_draw_string(32, AZ_ALIGN_CENTER, AZ_SCREEN_WIDTH/2 - 1, 100, "GAME OVER");
  az_draw_string(32, AZ_ALIGN_CENTER, AZ_SCREEN_WIDTH/2 + 1, 100, "GAME OVER");
  az_draw_string(16, AZ_ALIGN_CENTER, AZ_SCREEN_WIDTH/2, 180,
                 "Rescue the Zenith colonists!");
  az_draw_string(16, AZ_ALIGN_CENTER, AZ_SCREEN_WIDTH/2, 210, "Try again?");

  // Draw buttons:
  draw_button(state, &state->retry_button, retry_button_label);
  draw_button(state, &state->return_button, return_button_label);
  draw_button(state, &state->quit_button, quit_button_label);

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

// How long it takes the screen to fade in/out:
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

  const bool active = buttons_are_active(state);
  az_tick_button(&state->retry_button,
                 0, 0, active, time, state->clock, &state->soundboard);
  az_tick_button(&state->return_button,
                 0, 0, active, time, state->clock, &state->soundboard);
  az_tick_button(&state->quit_button,
                 0, 0, active, time, state->clock, &state->soundboard);
}

void az_gameover_on_click(az_gameover_state_t *state, int x, int y) {
  if (!buttons_are_active(state)) return;
  if (az_button_on_click(&state->retry_button, x, y, &state->soundboard)) {
    state->mode = AZ_GMODE_RETRYING;
    az_stop_music(&state->soundboard, FADE_TIME);
  }
  if (az_button_on_click(&state->return_button, x, y, &state->soundboard)) {
    state->mode = AZ_GMODE_RETURNING;
  }
  if (az_button_on_click(&state->quit_button, x, y, &state->soundboard)) {
    state->mode = AZ_GMODE_QUITTING;
    az_stop_music(&state->soundboard, FADE_TIME);
  }
}

/*===========================================================================*/
