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

#include "azimuth/tick/camera.h"

#include <math.h>

#include "azimuth/state/camera.h"
#include "azimuth/state/space.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

void az_tick_camera(az_space_state_t *state, double time) {
  az_camera_t *camera = &state->camera;
  const az_camera_bounds_t *bounds =
    &state->planet->rooms[state->ship.player.current_room].camera_bounds;
  az_track_camera_towards(
      camera, az_clamp_to_bounds(bounds, state->ship.position), time);
  if (camera->wobble_goal > camera->wobble_intensity) {
    camera->wobble_intensity += time;
  } else {
    camera->wobble_goal = 0.0;
    camera->wobble_intensity = fmax(0.0, camera->wobble_intensity - time);
  }
  camera->wobble_theta = az_mod2pi(camera->wobble_theta + AZ_TWO_PI * time);
}

/*===========================================================================*/
