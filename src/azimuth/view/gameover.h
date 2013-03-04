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
#ifndef AZIMUTH_VIEW_GAMEOVER_H_
#define AZIMUTH_VIEW_GAMEOVER_H_

#include "azimuth/util/audio.h"
#include "azimuth/util/clock.h"

/*===========================================================================*/

typedef struct {
  bool hovering;
  az_clock_t hover_start;
  double hover_pulse;
} az_gameover_button_t;

typedef struct {
  az_clock_t clock;
  az_soundboard_t soundboard;

  enum {
    AZ_GMODE_FADE_IN = 0,
    AZ_GMODE_NORMAL,
    AZ_GMODE_RETRYING,
    AZ_GMODE_RETURNING,
    AZ_GMODE_QUITTING
  } mode;
  double mode_progress;

  az_gameover_button_t retry_button, return_button, quit_button;
} az_gameover_state_t;

// Initialize static data for the Game Over view.  This must be called at least
// once before using the Game Over view (it is idempotent and may safely be
// called more than once).
void az_init_gameover_view(void);

void az_gameover_draw_screen(const az_gameover_state_t *state);

void az_tick_gameover_state(az_gameover_state_t *state, double time);

void az_gameover_on_hover(az_gameover_state_t *state, int x, int y);
void az_gameover_on_click(az_gameover_state_t *state, int x, int y);

/*===========================================================================*/

#endif // AZIMUTH_VIEW_GAMEOVER_H_
