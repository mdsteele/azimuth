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

#include "azimuth/state/sound.h"
#include "azimuth/state/space.h"
#include "azimuth/tick/particle.h"
#include "azimuth/tick/script.h"
#include "azimuth/util/random.h"

/*===========================================================================*/

#define FADE_TIME 0.75

static void next_scene(az_space_state_t *state, bool reset) {
  az_cutscene_state_t *cutscene = &state->cutscene;
  const az_scene_t next = cutscene->next;
  assert(reset || next != AZ_SCENE_NOTHING);
  if (reset) {
    cutscene->fade_alpha = 1.0;
    cutscene->time = cutscene->param1 = cutscene->param2 = 0.0;
    cutscene->step = 0;
    cutscene->step_timer = 0.0;
    cutscene->step_progress = 0.0;
  }
  cutscene->scene = cutscene->next;
  cutscene->scene_text = cutscene->next_text;
  if (state->sync_vm.script != NULL) az_resume_script(state, &state->sync_vm);
}

static void fade_to_next_scene(az_space_state_t *state, double time) {
  az_cutscene_state_t *cutscene = &state->cutscene;
  cutscene->fade_alpha = fmin(1.0, cutscene->fade_alpha + time / FADE_TIME);
  if (cutscene->fade_alpha == 1.0) next_scene(state, true);
}

static void fade_in(az_cutscene_state_t *cutscene, double time) {
  cutscene->fade_alpha = fmax(0.0, cutscene->fade_alpha - time / FADE_TIME);
}

static bool progress_step(az_cutscene_state_t *cutscene,
                          double time_for_step) {
  if (cutscene->step_timer >= time_for_step) {
    ++cutscene->step;
    cutscene->step_timer = 0.0;
    cutscene->step_progress = 0.0;
    return true;
  }
  cutscene->step_progress = cutscene->step_timer / time_for_step;
  return false;
}

static void tick_particles(az_cutscene_state_t *cutscene, double time) {
  AZ_ARRAY_LOOP(particle, cutscene->fg_particles) {
    az_tick_particle(particle, time);
  }
  AZ_ARRAY_LOOP(particle, cutscene->bg_particles) {
    az_tick_particle(particle, time);
  }
}

/*===========================================================================*/

static void add_laser(az_cutscene_state_t *cutscene, bool foreground,
                      double size, double y_pos, double speed) {
  az_cutscene_add_particle(
      cutscene, foreground, AZ_PAR_BEAM, (az_color_t){255, 128, 128, 255},
      (az_vector_t){(speed > 0 ? -700 - size : 700 + size), y_pos},
      (az_vector_t){speed, 0}, (speed > 0 ? 0 : AZ_PI),
      (size + 1400) / speed, size, 0.05 * size);
}

/*===========================================================================*/

void az_tick_cutscene(az_space_state_t *state, double time) {
  az_cutscene_state_t *cutscene = &state->cutscene;
  if (cutscene->scene != AZ_SCENE_NOTHING) {
    cutscene->time += time;
    cutscene->step_timer += time;
    tick_particles(cutscene, time);
  }
  bool ready_for_next_scene = true;
  switch (cutscene->scene) {
    case AZ_SCENE_NOTHING:
      assert(cutscene->scene_text == NULL);
      assert(cutscene->next == AZ_SCENE_NOTHING);
      assert(cutscene->next_text == NULL);
      assert(cutscene->fade_alpha == 0.0);
      assert(cutscene->time == 0.0);
      assert(cutscene->param1 == 0.0);
      assert(cutscene->param2 == 0.0);
      assert(cutscene->step == 0);
      assert(cutscene->step_timer == 0.0);
      assert(cutscene->step_progress == 0.0);
      return;
    case AZ_SCENE_TEXT:
      assert(cutscene->scene_text != NULL);
      break;
    case AZ_SCENE_CRUISING:
      if (cutscene->next == AZ_SCENE_MOVE_OUT) {
        next_scene(state, false);
        return;
      }
      break;
    case AZ_SCENE_MOVE_OUT:
      if (cutscene->param1 == 0.0) {
        az_play_sound(&state->soundboard, AZ_SND_CPLUS_ACTIVE);
      }
      cutscene->param1 = fmin(1.0, cutscene->param1 + time / 1.5);
      if (cutscene->param1 >= 0.5) {
        cutscene->param2 = fmin(1.0, cutscene->param2 + time / 1.5);
      }
      ready_for_next_scene = (cutscene->param2 >= 1.0);
      break;
    case AZ_SCENE_ARRIVAL:
      if (cutscene->param1 == 0.0) {
        az_play_sound(&state->soundboard, AZ_SND_ENTER_ATMOSPHERE);
      }
      cutscene->param1 = fmin(1.0, cutscene->param1 + time / 7.0);
      ready_for_next_scene = (cutscene->param1 >= 1.0);
      break;
    case AZ_SCENE_ZENITH:
      if (cutscene->next == AZ_SCENE_ARRIVAL ||
          cutscene->next == AZ_SCENE_ESCAPE) {
        next_scene(state, false);
        return;
      }
      break;
    case AZ_SCENE_ESCAPE:
      ready_for_next_scene = false;
      switch (cutscene->step) {
        case 0: // deform
          if (cutscene->step_progress == 0.0) {
            az_play_sound(&state->soundboard, AZ_SND_PLANET_DEFORM);
          }
          if (progress_step(cutscene, 4.0)) {
            az_play_sound(&state->soundboard, AZ_SND_PLANET_EXPLODE);
            az_play_sound(&state->soundboard, AZ_SND_EXPLODE_MEGA_BOMB);
          }
          break;
        case 1: // explode
          progress_step(cutscene, 1.5);
          break;
        case 2: progress_step(cutscene, 2.0); break; // fade
        case 3: progress_step(cutscene, 2.5); break; // silence
        default: ready_for_next_scene = true; break;
      }
      break;
    case AZ_SCENE_HOMECOMING:
      // TODO: implement homecoming scene
      break;
    case AZ_SCENE_BLACK:
      break;
    case AZ_SCENE_SAPIAIS:
      break;
    case AZ_SCENE_UHP_SHIPS:
      ready_for_next_scene = false;
      switch (cutscene->step) {
        case 0:
          if (progress_step(cutscene,  3.0)) {
            add_laser(cutscene, true, 100, 50, 1200);
            az_play_sound(&state->soundboard, AZ_SND_FIRE_GUN_PIERCE);
          }
          break;
        case 1:
          if (progress_step(cutscene, 15.0)) {
            add_laser(cutscene, false, 60, -125, 1200);
            az_play_sound_with_volume(&state->soundboard,
                                      AZ_SND_FIRE_GUN_PIERCE, 0.5);
          } break;
        case 2: progress_step(cutscene, 2.0); break;
        case 3: progress_step(cutscene, 2.0); break;
        case 4: progress_step(cutscene, 2.0); break;
        case 5:
          if (progress_step(cutscene, 5.0)) {
            az_cutscene_add_particle(cutscene, true, AZ_PAR_EXPLOSION,
                                     (az_color_t){255, 233, 211, 192},
                                     AZ_VZERO, AZ_VZERO, 0, 0.9, 200, 0);
            az_random_seed_t seed = {1, 1};
            for (int i = 0; i < 20; ++i) {
              const double pos_r = 50 * az_rand_udouble(&seed);
              const double pos_theta = AZ_PI * az_rand_sdouble(&seed);
              const double lifetime = 3 + 2 * az_rand_udouble(&seed);
              const double size = 2.5 + az_rand_udouble(&seed);
              const double spin = 10 * az_rand_sdouble(&seed);
              az_cutscene_add_particle(
                  cutscene, true, AZ_PAR_SHARD,
                  (az_color_t){128, 128, 128, 255},
                  az_vpolar(pos_r, pos_theta),
                  az_vadd(az_vpolar(2 * pos_r, pos_theta),
                          (az_vector_t){-10, 50}),
                  pos_theta, lifetime, size, spin);
            }
            az_play_sound(&state->soundboard, AZ_SND_EXPLODE_MEGA_BOMB);
          }
          break;
        case 6:
          cutscene->param2 += time / 10.0;
          ready_for_next_scene = (cutscene->param2 >= 1.0);
          break;
      }
      break;
  }
  if (ready_for_next_scene && cutscene->next != cutscene->scene) {
    fade_to_next_scene(state, time);
  } else {
    fade_in(cutscene, time);
  }
}

/*===========================================================================*/
