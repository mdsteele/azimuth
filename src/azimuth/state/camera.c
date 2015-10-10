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

#include "azimuth/state/camera.h"

#include <math.h>

#include "azimuth/constants.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

az_vector_t az_bounds_center(const az_camera_bounds_t *bounds) {
  return az_vpolar(bounds->min_r + 0.5 * bounds->r_span,
                   bounds->min_theta + 0.5 * bounds->theta_span);
}

az_vector_t az_clamp_to_bounds(const az_camera_bounds_t *bounds,
                               az_vector_t vec) {
  return az_clamp_to_bounds_with_override(bounds, vec, 0);
}

az_vector_t az_clamp_to_bounds_with_override(
    const az_camera_bounds_t *bounds, az_vector_t vec, double r_max_override) {
  const double r = fmin(fmax(bounds->min_r, az_vnorm(vec)),
                        fmax(bounds->min_r + bounds->r_span, r_max_override));
  const double half_span = 0.5 * bounds->theta_span;
  const double mid_theta = bounds->min_theta + half_span;
  const double theta = mid_theta +
    fmin(fmax(-half_span, az_mod2pi(az_vtheta(vec) - mid_theta)),
         half_span);
  return az_vpolar(r, theta);
}

bool az_position_visible(const az_camera_bounds_t *bounds, az_vector_t vec) {
  const double min_rho = bounds->min_r - AZ_SCREEN_HEIGHT/2;
  const double max_rho = bounds->min_r + bounds->r_span + AZ_SCREEN_HEIGHT/2;
  // Right-hand edge of the room:
  const az_vector_t vec1 = az_vrotate(vec, -bounds->min_theta);
  if (vec1.x >= min_rho && vec1.x <= max_rho &&
      vec1.y >= -AZ_SCREEN_WIDTH/2 && vec1.y <= 0.0) return true;
  // Left-hand edge of the room:
  const az_vector_t vec2 =
    az_vrotate(vec, -(bounds->min_theta + bounds->theta_span));
  if (vec2.x >= min_rho && vec2.x <= max_rho &&
      vec2.y >= 0.0 && vec2.y <= AZ_SCREEN_WIDTH/2) return true;
  // Middle of the room:
  const double rho = az_vnorm(vec);
  if (rho < min_rho || rho > max_rho) return false;
  const double rel_theta =
    az_mod2pi_nonneg(az_vtheta(vec) - bounds->min_theta);
  return (rel_theta >= 0 && rel_theta <= bounds->theta_span);
}

/*===========================================================================*/

az_vector_t az_camera_shake_offset(const az_camera_t *camera,
                                   az_clock_t clock) {
  const double vert = camera->shake_vert + camera->quake_vert;
  return (az_vector_t){
    (az_clock_mod(2, 1, clock) ? camera->shake_horz : -camera->shake_horz),
    (az_clock_mod(2, 2, clock) ? vert : -vert)
  };
}

static double decay_shake(double shake, double time) {
  return (shake <= 0.1 ? 0.0 : shake * pow(0.1, time));
}

void az_track_camera_towards(az_camera_t *camera, az_vector_t towards,
                             double time) {
  const double tracking_base = 0.00003; // smaller = faster tracking
  const az_vector_t difference = az_vsub(towards, camera->center);
  const az_vector_t change =
    az_vmul(difference, 1.0 - pow(tracking_base, time));
  az_vpluseq(&camera->center, change);
  camera->shake_horz = decay_shake(camera->shake_horz, time);
  camera->shake_vert = decay_shake(camera->shake_vert, time);
}

void az_shake_camera(az_camera_t *camera, double horz, double vert) {
  camera->shake_horz = fmax(camera->shake_horz, horz);
  camera->shake_vert = fmax(camera->shake_vert, vert);
}

/*===========================================================================*/
