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

#pragma once
#ifndef AZIMUTH_VIEW_BUTTON_H_
#define AZIMUTH_VIEW_BUTTON_H_

#include "azimuth/util/audio.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/polygon.h"

/*===========================================================================*/

typedef struct {
  az_polygon_t polygon;
  int x, y;
  double hover_pulse;
  az_clock_t hover_start;
  bool hovering;
} az_button_t;

void az_init_button(az_button_t *button, az_polygon_t polygon, int x, int y);

void az_draw_standard_button(const az_button_t *button);
void az_draw_dangerous_button(const az_button_t *button);

// Call once per frame, before drawing the screen.
void az_tick_button(az_button_t *button, int xoff, int yoff, bool is_active,
                    double time, az_clock_t clock,
                    az_soundboard_t *soundboard);

// Call when the mouse button clicks the given position; returns true if the
// button was clicked.
bool az_button_on_click(az_button_t *button, int x, int y);

// Sets the button's hover_pulse as if it had been clicked.  Useful for when
// the button is activated via a hotkey.
void az_press_button(az_button_t *button);

/*===========================================================================*/

#endif // AZIMUTH_VIEW_BUTTON_H_
