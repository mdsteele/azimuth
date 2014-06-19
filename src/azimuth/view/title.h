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
#ifndef AZIMUTH_VIEW_TITLE_H_
#define AZIMUTH_VIEW_TITLE_H_

#include "azimuth/constants.h"
#include "azimuth/state/save.h"
#include "azimuth/util/audio.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/prefs.h"
#include "azimuth/view/button.h"
#include "azimuth/view/prefs.h"

/*===========================================================================*/

typedef struct {
  int x, y;
  az_button_t main, erase, confirm, cancel;
} az_title_save_slot_t;

typedef struct {
  const az_planet_t *planet;
  const az_saved_games_t *saved_games;
  az_clock_t clock;
  az_soundboard_t soundboard;

  enum {
    AZ_TMODE_INTRO = 0,
    AZ_TMODE_READY,
    AZ_TMODE_NORMAL,
    AZ_TMODE_PREFS,
    AZ_TMODE_ABOUT,
    AZ_TMODE_ERASING,
    AZ_TMODE_PICK_KEY,
    AZ_TMODE_STARTING,
    AZ_TMODE_QUITTING
  } mode;
  union {
    struct { double progress; } intro;
    struct { az_clock_t start; } ready;
    struct { int slot_index; bool do_erase; } erasing;
    struct { double progress; int slot_index; } starting;
  } mode_data;

  az_title_save_slot_t slots[AZ_NUM_SAVED_GAME_SLOTS];
  az_button_t prefs_button, about_button, quit_button;
  az_prefs_pane_t prefs_pane;
} az_title_state_t;

void az_init_title_state(az_title_state_t *state, const az_planet_t *planet,
                         const az_saved_games_t *saved_games,
                         const az_preferences_t *prefs);

void az_title_draw_screen(const az_title_state_t *state);

void az_tick_title_state(az_title_state_t *state, double time);

// Skip to READY mode.  The UI must currently be in INTRO mode.
void az_title_skip_intro(az_title_state_t *state);

// Put the UI into STARTING mode, to start the game for the given slot index.
// The UI must currently be in NORMAL mode.
void az_title_start_game(az_title_state_t *state, int slot_index);

void az_title_on_click(az_title_state_t *state, int x, int y);

/*===========================================================================*/

#endif // AZIMUTH_VIEW_TITLE_H_
