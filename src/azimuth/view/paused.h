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
#ifndef AZIMUTH_VIEW_PAUSED_H_
#define AZIMUTH_VIEW_PAUSED_H_

#include <stdbool.h>

#include "azimuth/state/planet.h"
#include "azimuth/state/ship.h"
#include "azimuth/state/upgrade.h"
#include "azimuth/util/audio.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/prefs.h"
#include "azimuth/view/button.h"
#include "azimuth/view/prefs.h"

/*===========================================================================*/

typedef struct {
  const az_planet_t *planet;
  const az_preferences_t *prefs;
  az_ship_t *ship;
  az_soundboard_t soundboard;
  az_clock_t clock;
  enum {
    AZ_PAUSE_DRAWER_MAP = 0,
    AZ_PAUSE_DRAWER_OPTIONS,
    AZ_PAUSE_DRAWER_UPGRADES
  } current_drawer;
  double drawer_slide; // -1.0 (prefs) to 0.0 (map) to 1.0 (upgrades)
  double scroll_y, scroll_y_min;
  enum {
    AZ_PAUSE_HOVER_NOTHING = 0,
    AZ_PAUSE_HOVER_UPGRADE,
    AZ_PAUSE_HOVER_SHIP
  } hovering;
  az_upgrade_t hovered_upgrade;
  az_prefs_pane_t prefs_pane;
  az_button_t options_drawer_handle, upgrade_drawer_handle;
  az_button_t quit_button;
  bool confirming_quit, do_quit;
  az_button_t confirm_button, cancel_button;
  double quitting_fade_alpha; // 0.0 to 1.0
} az_paused_state_t;

void az_init_paused_state(
    az_paused_state_t *state, const az_planet_t *planet,
    const az_preferences_t *prefs, az_ship_t *ship);

void az_paused_draw_screen(const az_paused_state_t *state);

void az_tick_paused_state(az_paused_state_t *state, double time);

void az_paused_on_hover(az_paused_state_t *state, int x, int y);
void az_paused_on_click(az_paused_state_t *state, int x, int y);

/*===========================================================================*/

#endif // AZIMUTH_VIEW_PAUSED_H_
