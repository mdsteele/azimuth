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

#include "azimuth/tick/cutscene.h"

#include <assert.h>
#include <math.h>

#include "azimuth/state/space.h"
#include "azimuth/tick/script.h"

/*===========================================================================*/

#define FADE_TIME 0.75

static void next_scene(az_space_state_t *state, bool reset) {
  az_cutscene_state_t *cutscene = &state->cutscene;
  const az_scene_t next = cutscene->next;
  assert(reset || next != AZ_SCENE_NOTHING);
  if (reset) {
    cutscene->fade_alpha = 1.0;
    cutscene->time = cutscene->param1 = cutscene->param2 = 0.0;
  }
  cutscene->scene = cutscene->next;
  if (cutscene->vm.script != NULL) az_resume_script(state, &cutscene->vm);
}

static void fade_to_next_scene(az_space_state_t *state, double time) {
  az_cutscene_state_t *cutscene = &state->cutscene;
  cutscene->fade_alpha = fmin(1.0, cutscene->fade_alpha + time / FADE_TIME);
  if (cutscene->fade_alpha == 1.0) next_scene(state, true);
}

static void fade_in(az_cutscene_state_t *cutscene, double time) {
  cutscene->fade_alpha = fmax(0.0, cutscene->fade_alpha - time / FADE_TIME);
}

void az_tick_cutscene(az_space_state_t *state, double time) {
  az_cutscene_state_t *cutscene = &state->cutscene;
  switch (cutscene->scene) {
    case AZ_SCENE_NOTHING:
      assert(cutscene->next == AZ_SCENE_NOTHING);
      assert(cutscene->time == 0.0);
      assert(cutscene->param1 == 0.0);
      assert(cutscene->param2 == 0.0);
      break;
    case AZ_SCENE_CRUISING:
      cutscene->time += time;
      switch (cutscene->next) {
        case AZ_SCENE_CRUISING:
          fade_in(cutscene, time);
          break;
        case AZ_SCENE_MOVE_OUT:
          next_scene(state, false);
          break;
        default:
          fade_to_next_scene(state, time);
          break;
      }
      break;
    case AZ_SCENE_MOVE_OUT:
      cutscene->time += time;
      cutscene->param1 = fmin(1.0, cutscene->param1 + time / 1.5);
      if (cutscene->param1 >= 0.5) {
        cutscene->param2 = fmin(1.0, cutscene->param2 + time / 1.5);
      }
      if (cutscene->next != AZ_SCENE_MOVE_OUT && cutscene->param2 >= 1.0) {
        fade_to_next_scene(state, time);
      } else fade_in(cutscene, time);
      break;
    case AZ_SCENE_ARRIVAL:
      cutscene->param1 = fmin(1.0, cutscene->param1 + time / 5.0);
      if (cutscene->next != AZ_SCENE_ARRIVAL && cutscene->param1 >= 1.0) {
        fade_to_next_scene(state, time);
      } else fade_in(cutscene, time);
      break;
    // TODO: implement other cutscenes
    case AZ_SCENE_ESCAPE:
    case AZ_SCENE_HOMECOMING:
      if (cutscene->next == cutscene->scene) fade_in(cutscene, time);
      else fade_to_next_scene(state, time);
      break;
  }
}

/*===========================================================================*/
