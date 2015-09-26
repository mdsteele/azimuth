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

#include "azimuth/view/button.h"

#include <math.h>

#include <GL/gl.h>

#include "azimuth/gui/event.h"
#include "azimuth/state/sound.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/polygon.h"

/*===========================================================================*/

#define HOVER_PULSE_FRAMES 20
#define HOVER_PULSE_MIN 0.35
#define HOVER_PULSE_MAX 0.55
#define HOVER_PULSE_CLICK 1.0
#define HOVER_DECAY_TIME 0.7

void az_init_button(az_button_t *button, az_polygon_t polygon, int x, int y) {
  AZ_ZERO_OBJECT(button);
  button->polygon = polygon;
  button->x = x;
  button->y = y;
}

static void do_button_polygon(GLenum mode, const az_button_t *button) {
  glBegin(mode); {
    for (int i = 0; i < button->polygon.num_vertices; ++i) {
      glVertex2d(button->polygon.vertices[i].x + button->x,
                 button->polygon.vertices[i].y + button->y);
    }
  } glEnd();
}

void az_draw_borderless_button(const az_button_t *button) {
  glColor4f(0.525, 0.75, 0.75, 0.5 * button->hover_pulse);
  do_button_polygon(GL_TRIANGLE_FAN, button);
}

void az_draw_standard_button(const az_button_t *button) {
  glColor4f(0.7 * button->hover_pulse, button->hover_pulse,
            button->hover_pulse, 0.7);
  do_button_polygon(GL_TRIANGLE_FAN, button);
  glColor3f(0.75, 0.75, 0.75);
  do_button_polygon(GL_LINE_LOOP, button);
}

void az_draw_dangerous_button(const az_button_t *button) {
  glColor4f(button->hover_pulse, 0, 0, 0.7);
  do_button_polygon(GL_TRIANGLE_FAN, button);
  glColor3f(0.75, 0.75, 0.75);
  do_button_polygon(GL_LINE_LOOP, button);
}

void az_tick_button(az_button_t *button, int xoff, int yoff, bool is_active,
                    double time, az_clock_t clock,
                    az_soundboard_t *soundboard) {
  int x, y;
  if (is_active && az_get_mouse_position(&x, &y) &&
      az_polygon_contains(button->polygon,
                          (az_vector_t){x - xoff - button->x,
                                        y - yoff - button->y})) {
    if (!button->hovering) {
      button->hover_pulse = fmax(button->hover_pulse, HOVER_PULSE_MAX);
      button->hover_start = clock;
      button->hovering = true;
      az_play_sound(soundboard, AZ_SND_MENU_HOVER);
    }
  } else {
    button->hovering = false;
  }
  if (button->hovering) {
    if (button->hover_pulse > HOVER_PULSE_MAX) {
      button->hover_pulse -= time / HOVER_DECAY_TIME;
      if (button->hover_pulse <= HOVER_PULSE_MAX) {
        button->hover_pulse = HOVER_PULSE_MAX;
        button->hover_start = clock;
      }
    } else {
      button->hover_pulse = HOVER_PULSE_MIN +
        (HOVER_PULSE_MAX - HOVER_PULSE_MIN) *
        (double)az_clock_zigzag(HOVER_PULSE_FRAMES, 1,
                                clock - button->hover_start) /
        (double)HOVER_PULSE_FRAMES;
    }
  } else {
    button->hover_pulse =
      fmax(0.0, button->hover_pulse - time / HOVER_DECAY_TIME);
  }
}

bool az_button_on_click(az_button_t *button, int x, int y,
                        az_soundboard_t *soundboard) {
  if (az_polygon_contains(button->polygon,
                          (az_vector_t){x - button->x, y - button->y})) {
    button->hover_pulse = HOVER_PULSE_CLICK;
    az_play_sound(soundboard, AZ_SND_MENU_CLICK);
    return true;
  } else return false;
}

void az_press_button(az_button_t *button, az_soundboard_t *soundboard) {
  button->hover_pulse = HOVER_PULSE_CLICK;
  az_play_sound(soundboard, AZ_SND_MENU_CLICK);
}

/*===========================================================================*/
