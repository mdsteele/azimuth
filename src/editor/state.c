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

#include "editor/state.h"

#include "azimuth/util/vector.h"

/*===========================================================================*/

#define SCROLL_SPEED 5.0
#define SPIN_RADIUS 80.0

void az_tick_editor_state(az_editor_state_t *state) {
  if (state->controls.up && !state->controls.down) {
    if (state->spin_camera) {
      state->camera = az_vadd(state->camera,
                              az_vpolar(SCROLL_SPEED,
                                        az_vtheta(state->camera)));
    } else {
      state->camera.y += SCROLL_SPEED;
    }
  }
  if (state->controls.down && !state->controls.up) {
    if (state->spin_camera) {
      if (az_vnorm(state->camera) <= SCROLL_SPEED) {
        state->camera = az_vpolar(0.0001, az_vtheta(state->camera));
      } else {
        state->camera = az_vsub(state->camera,
                                az_vpolar(SCROLL_SPEED,
                                          az_vtheta(state->camera)));
      }
    } else {
      state->camera.y -= SCROLL_SPEED;
    }
  }
  if (state->controls.left && !state->controls.right) {
    if (state->spin_camera) {
      state->camera = az_vrotate(state->camera, SCROLL_SPEED /
                                 (SPIN_RADIUS + az_vnorm(state->camera)));
    } else {
      state->camera.x -= SCROLL_SPEED;
    }
  }
  if (state->controls.right && !state->controls.left) {
    if (state->spin_camera) {
      state->camera = az_vrotate(state->camera, -SCROLL_SPEED /
                                 (SPIN_RADIUS + az_vnorm(state->camera)));
    } else {
      state->camera.x += SCROLL_SPEED;
    }
  }
}

/*===========================================================================*/
