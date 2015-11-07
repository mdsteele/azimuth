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
#ifndef AZIMUTH_STATE_CUTSCENE_H_
#define AZIMUTH_STATE_CUTSCENE_H_

/*===========================================================================*/

// The number of different cutscenes there are, not counting AZ_SCENE_NOTHING
// and AZ_SCENE_TEXT:
#define AZ_NUM_SCENES 8

typedef enum {
  AZ_SCENE_TEXT = -1,
  AZ_SCENE_NOTHING = 0,
  AZ_SCENE_CRUISING,
  AZ_SCENE_MOVE_OUT,
  AZ_SCENE_ARRIVAL,
  AZ_SCENE_ZENITH,
  AZ_SCENE_ESCAPE,
  AZ_SCENE_HOMECOMING,
  AZ_SCENE_BLACK,
  AZ_SCENE_SAPIAIS,
} az_scene_t;

typedef struct {
  az_scene_t scene;
  az_scene_t next;
  const char *scene_text;
  const char *next_text;
  double fade_alpha; // 0.0 to 1.0
  double time;
  double param1, param2;
  int step;
} az_cutscene_state_t;

/*===========================================================================*/

#endif // AZIMUTH_STATE_CUTSCENE_H_
