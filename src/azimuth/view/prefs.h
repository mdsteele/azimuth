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
#ifndef AZIMUTH_VIEW_PREFS_H_
#define AZIMUTH_VIEW_PREFS_H_

#include "azimuth/util/audio.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/prefs.h"
#include "azimuth/view/button.h"

/*===========================================================================*/

#define AZ_PREFS_BOX_WIDTH 512
#define AZ_PREFS_BOX_HEIGHT 186

typedef struct {
  int x, y;
  az_button_t handle;
  float value;
  bool grabbed;
} az_prefs_slider_t;

typedef struct {
  az_button_t button;
  az_key_id_t key;
} az_prefs_key_picker_t;

typedef struct {
  az_button_t button;
  bool checked;
} az_prefs_checkbox_t;

typedef struct {
  int x, y;
  az_prefs_slider_t music_slider, sound_slider;
  az_prefs_key_picker_t pickers[AZ_PREFS_NUM_KEYS];
  az_prefs_checkbox_t speedrun_timer_checkbox;
  az_prefs_checkbox_t fullscreen_checkbox;
  az_prefs_checkbox_t enable_hints_checkbox;
  int selected_key_picker_index; // -1 for none
} az_prefs_pane_t;

void az_init_prefs_pane(az_prefs_pane_t *pane, int x, int y,
                        const az_preferences_t *prefs);

void az_draw_prefs_pane(const az_prefs_pane_t *pane);

// Call once per frame, before drawing the screen.
void az_tick_prefs_pane(az_prefs_pane_t *pane, bool visible, double time,
                        az_clock_t clock, az_soundboard_t *soundboard);

// Call when the mouse button clicks the given position.
void az_prefs_pane_on_click(az_prefs_pane_t *pane, int x, int y,
                            az_soundboard_t *soundboard);

void az_prefs_try_pick_key(az_prefs_pane_t *pane, az_key_id_t key_id,
                           az_soundboard_t *soundboard);

/*===========================================================================*/

#endif // AZIMUTH_VIEW_PREFS_H_
