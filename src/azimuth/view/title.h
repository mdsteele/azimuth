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
#include "azimuth/util/clock.h"

/*===========================================================================*/

typedef struct {
  enum { AZ_TSS_HOVER_NONE, AZ_TSS_HOVER_MAIN, AZ_TSS_HOVER_ERASE } hover;
  az_clock_t hover_start;
  double main_hover_pulse, erase_hover_pulse;
} az_title_save_slot_t;

typedef struct {
  bool hovering;
  az_clock_t hover_start;
  double hover_pulse;
} az_title_button_t;

typedef struct {
  const az_saved_games_t *saved_games;
  az_clock_t clock;

  enum {
    // TODO intro sequence
    AZ_TMODE_NORMAL = 0,
    AZ_TMODE_ABOUT,
    AZ_TMODE_ERASING,
    AZ_TMODE_STARTING,
    AZ_TMODE_QUITTING
  } mode;
  union {
    struct { int slot_index; bool do_erase; } erasing;
    struct { double progress; int slot_index; } starting;
  } mode_data;

  az_title_save_slot_t slots[AZ_NUM_SAVED_GAME_SLOTS];
  az_title_button_t about_button, quit_button, confirm_button, cancel_button;
} az_title_state_t;

void az_title_draw_screen(const az_title_state_t *state);

void az_tick_title_state(az_title_state_t *state, double time);

void az_title_on_hover(az_title_state_t *state, int x, int y);
void az_title_on_click(az_title_state_t *state, int x, int y);

/*===========================================================================*/

#endif // AZIMUTH_VIEW_TITLE_H_
