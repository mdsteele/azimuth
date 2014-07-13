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

#include "azimuth/view/gravfield.h"

#include <assert.h>
#include <math.h>

#include <GL/gl.h>

#include "azimuth/state/gravfield.h"
#include "azimuth/state/space.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/random.h"
#include "azimuth/util/vector.h"
#include "azimuth/view/util.h"

/*===========================================================================*/

static void draw_trapezoid_gravfield(const az_gravfield_t *gravfield) {
  assert(gravfield->kind == AZ_GRAV_TRAPEZOID);
  if (gravfield->strength == 0.0) return;
  const double alpha_mult = fmin(1.0, fabs(gravfield->strength) / 200.0);
  const double semilength = gravfield->size.trapezoid.semilength;
  const double front_offset = gravfield->size.trapezoid.front_offset;
  const double front_semiwidth = gravfield->size.trapezoid.front_semiwidth;
  const double rear_semiwidth = gravfield->size.trapezoid.rear_semiwidth;
  const double stride = 100.0;
  const double offset =
    az_signmod(0.5 * gravfield->age, stride, gravfield->strength);
  glBegin(GL_QUADS); {
    const double xlimit =
      (gravfield->strength < 0.0 ? -semilength - stride : -semilength);
    for (double x0 = semilength + offset; x0 > xlimit; x0 -= stride) {
      const double x1 = x0 - copysign(stride, gravfield->strength);
      const double x0a = fmin(fmax(x0, -semilength), semilength);
      const double x1a = fmin(fmax(x1, -semilength), semilength);
      const double y0b = front_offset - front_semiwidth;
      const double y0t = front_offset + front_semiwidth;
      const double bottom_ratio = (y0b + rear_semiwidth) / (2.0 * semilength);
      const double top_ratio = (y0t - rear_semiwidth) / (2.0 * semilength);
      glColor4f(0, 0.25, 0.5, alpha_mult * fabs(x0a - x1) / stride);
      glVertex2d(x0a, -rear_semiwidth + (x0a + semilength) * bottom_ratio);
      glVertex2d(x0a, rear_semiwidth + (x0a + semilength) * top_ratio);
      glColor4f(0, 0, 0.5, alpha_mult * fabs(x1a - x1) / stride);
      glVertex2d(x1a, rear_semiwidth + (x1a + semilength) * top_ratio);
      glVertex2d(x1a, -rear_semiwidth + (x1a + semilength) * bottom_ratio);
    }
  } glEnd();
}

static void draw_sector_pull_gravfield(const az_gravfield_t *gravfield) {
  assert(gravfield->kind == AZ_GRAV_SECTOR_PULL);
  if (gravfield->strength == 0.0) return;
  const double alpha_mult = fmin(1.0, fabs(gravfield->strength) / 200.0);
  const double stride = 100.0;
  const double inner_radius = gravfield->size.sector.inner_radius;
  const double outer_radius = inner_radius + gravfield->size.sector.thickness;
  const double interior_angle = az_sector_interior_angle(&gravfield->size);
  const double offset =
    az_signmod(0.5 * gravfield->age, stride, gravfield->strength);
  const double rlimit =
    (gravfield->strength < 0.0 ? outer_radius + stride : outer_radius);
  for (double r0 = inner_radius - offset; r0 < rlimit; r0 += stride) {
    const double r1 = r0 + copysign(stride, gravfield->strength);
    const double r0a = fmin(fmax(r0, inner_radius), outer_radius);
    const double r1a = fmin(fmax(r1, inner_radius), outer_radius);
    const GLfloat alpha0 = alpha_mult * fabs(r1 - r0a) / stride;
    const GLfloat alpha1 = alpha_mult * fabs(r1 - r1a) / stride;
    const int limit = ceil(interior_angle / AZ_DEG2RAD(6));
    const double step = interior_angle / limit;
    glBegin(GL_QUAD_STRIP); {
      for (int i = 0; i <= limit; ++i) {
        const double t = i * step;
        glColor4f(0, 0.25, 0.5, alpha0); // dark bluish tint
        glVertex2d(r0a * cos(t), r0a * sin(t));
        glColor4f(0, 0, 0.5, alpha1); // dark blue tint
        glVertex2d(r1a * cos(t), r1a * sin(t));
      }
    } glEnd();
  }
}

static void draw_sector_spin_gravfield(const az_gravfield_t *gravfield) {
  assert(gravfield->kind == AZ_GRAV_SECTOR_SPIN);
  if (gravfield->strength == 0.0) return;
  const double alpha_mult = fmin(1.0, fabs(gravfield->strength) / 200.0);
  const double inner_radius = gravfield->size.sector.inner_radius;
  const double outer_radius = inner_radius + gravfield->size.sector.thickness;
  const double interior_angle = az_sector_interior_angle(&gravfield->size);
  const double stride =
    AZ_TWO_PI / fmax(1.0, floor(0.5 + outer_radius * AZ_TWO_PI / 200.0));
  const double offset =
    az_signmod(gravfield->age / outer_radius, stride, gravfield->strength);
  const double tlimit = (gravfield->strength < 0.0 ? -stride : 0.0);
  for (double t0 = interior_angle + offset; t0 > tlimit; t0 -= stride) {
    const double t1 = t0 - copysign(stride, gravfield->strength);
    const double t0a = fmin(fmax(t0, 0.0), interior_angle);
    const double t1a = fmin(fmax(t1, 0.0), interior_angle);
    if (t0a == t1a) continue;
    const int limit = ceil(fabs(t0a - t1a) / AZ_DEG2RAD(5));
    const double step = (t0a - t1a) / limit;
    assert(step != 0.0);
    glBegin(GL_QUAD_STRIP); {
      for (int i = 0; i <= limit; ++i) {
        const double t = t0a - i * step;
        const double alpha = alpha_mult * fabs(t - t1) / stride;
        glColor4f(0, 0.25 * alpha, 0.5, alpha); // dark bluish tint
        glVertex2d(inner_radius * cos(t), inner_radius * sin(t));
        glVertex2d(outer_radius * cos(t), outer_radius * sin(t));
      }
    } glEnd();
  }
}

static void draw_lava_bubble(double max_radius, double duration, double period,
                             double timer) {
  assert(duration > 0.0);
  assert(period >= duration);
  assert(timer >= 0.0);
  const double phase = fmod(timer, period);
  if (phase > duration) return;
  const double mod = phase / duration;
  glBegin(GL_TRIANGLE_FAN); {
    glColor4f(1, 0.6, 0.4, 0.4 - 0.3 * mod);
    glVertex2f(0, 0);
    glColor4f(0.5, 0.3, 0.2, 0.5 - 0.3 * mod);
    const double radius = max_radius * mod;
    for (int i = -90; i < 90; i += 10) {
      glVertex2d(radius * cos(AZ_DEG2RAD(i)), radius * sin(AZ_DEG2RAD(i)));
    }
    for (int i = 90; i <= 270; i += 10) {
      glVertex2d(0.25 * radius * cos(AZ_DEG2RAD(i)),
                 radius * sin(AZ_DEG2RAD(i)));
    }
  } glEnd();
}

static void draw_liquid_gravfield(
    const az_gravfield_t *gravfield, az_color_t deep_color,
    az_color_t surface_color, az_color_t mist_color) {
  assert(az_is_liquid(gravfield->kind));
  assert(gravfield->strength == 1.0);
  const double semilength = gravfield->size.trapezoid.semilength;
  const double front_offset = gravfield->size.trapezoid.front_offset;
  const double front_semiwidth = gravfield->size.trapezoid.front_semiwidth;
  const double rear_semiwidth = gravfield->size.trapezoid.rear_semiwidth;
  const double position_norm = az_vnorm(gravfield->position);
  const double outer_radius =
    hypot(fmax(front_offset + front_semiwidth, front_offset - front_semiwidth),
          position_norm + semilength);
  const double inner_radius =
    hypot(rear_semiwidth, position_norm - semilength);
  const double outer_start_theta =
    atan2(front_offset - front_semiwidth, position_norm + semilength);
  const double outer_end_theta =
    atan2(front_offset + front_semiwidth, position_norm + semilength);
  const double inner_start_theta =
    atan2(-rear_semiwidth, position_norm - semilength);
  const double inner_end_theta =
    atan2(rear_semiwidth, position_norm - semilength);
  const int num_steps = 20;
  const double outer_step =
    az_mod2pi_nonneg(outer_end_theta - outer_start_theta) / num_steps;
  const double inner_step =
    az_mod2pi_nonneg(inner_end_theta - inner_start_theta) / num_steps;
  // Draw the liquid itself:
  glBegin(GL_QUAD_STRIP); {
    for (int i = 0; i <= num_steps; ++i) {
      const double inner_theta = inner_start_theta + i * inner_step;
      const double outer_theta = outer_start_theta + i * outer_step;
      az_gl_color(deep_color);
      glVertex2d(inner_radius * cos(inner_theta) - position_norm,
                 inner_radius * sin(inner_theta));
      az_gl_color(surface_color);
      glVertex2d(outer_radius * cos(outer_theta) - position_norm,
                 outer_radius * sin(outer_theta));
    }
  } glEnd();
  // Draw layer of mist above the liquid:
  glBegin(GL_QUAD_STRIP); {
    for (int i = 0; i <= num_steps; ++i) {
      const double outer_theta = outer_start_theta + i * outer_step;
      az_gl_color(surface_color);
      glVertex2d(outer_radius * cos(outer_theta) - position_norm,
                 outer_radius * sin(outer_theta));
      az_gl_color(mist_color);
      glVertex2d((outer_radius + 6) * cos(outer_theta) - position_norm,
                 (outer_radius + 6) * sin(outer_theta));
    }
  } glEnd();
  // For lava, draw bubbles:
  if (gravfield->kind == AZ_GRAV_LAVA) {
    const double bubble_spacing = 15.0;
    const double bubble_step = bubble_spacing / outer_radius;
    double timer = gravfield->age;
    az_random_seed_t seed = {1, 1};
    for (double theta = outer_start_theta; theta < outer_end_theta;
         theta += bubble_step) {
      glPushMatrix(); {
        glTranslated(outer_radius * cos(theta) - position_norm,
                     outer_radius * sin(theta), 0);
        az_gl_rotated(theta);
        const double radius = 5.0 + az_rand_sdouble(&seed);
        const double duration = 0.5 + 0.4 * az_rand_udouble(&seed);
        const double period = duration + 0.6 + az_rand_udouble(&seed);
        timer += 3.0 * az_rand_udouble(&seed);
        draw_lava_bubble(radius, duration, period, timer);
      } glPopMatrix();
    }
  }
}

static void draw_water_gravfield(const az_gravfield_t *gravfield) {
  assert(gravfield->kind == AZ_GRAV_WATER);
  draw_liquid_gravfield(gravfield,
                        (az_color_t){26, 26, 102, 153},
                        (az_color_t){77, 153, 255, 77},
                        (az_color_t){255, 255, 255, 0});
}

static void draw_lava_gravfield(const az_gravfield_t *gravfield) {
  assert(gravfield->kind == AZ_GRAV_LAVA);
  draw_liquid_gravfield(gravfield,
                        (az_color_t){102, 26, 26, 200},
                        (az_color_t){255, 120, 77, 100},
                        (az_color_t){255, 200, 150, 0});
}

/*===========================================================================*/

void az_draw_gravfield_no_transform(const az_gravfield_t *gravfield) {
  assert(gravfield->kind != AZ_GRAV_NOTHING);
  switch (gravfield->kind) {
    case AZ_GRAV_NOTHING: AZ_ASSERT_UNREACHABLE();
    case AZ_GRAV_TRAPEZOID:
      draw_trapezoid_gravfield(gravfield);
      break;
    case AZ_GRAV_SECTOR_PULL:
      draw_sector_pull_gravfield(gravfield);
      break;
    case AZ_GRAV_SECTOR_SPIN:
      draw_sector_spin_gravfield(gravfield);
      break;
    case AZ_GRAV_WATER:
      draw_water_gravfield(gravfield);
      break;
    case AZ_GRAV_LAVA:
      draw_lava_gravfield(gravfield);
      break;
  }
}

void az_draw_gravfield(const az_gravfield_t *gravfield) {
  assert(gravfield->kind != AZ_GRAV_NOTHING);
  glPushMatrix(); {
    az_gl_translated(gravfield->position);
    az_gl_rotated(gravfield->angle);
    az_draw_gravfield_no_transform(gravfield);
  } glPopMatrix();
}

void az_draw_gravfields(const az_space_state_t *state) {
  AZ_ARRAY_LOOP(gravfield, state->gravfields) {
    if (gravfield->kind == AZ_GRAV_NOTHING) continue;
    if (az_is_liquid(gravfield->kind)) continue;
    az_draw_gravfield(gravfield);
  }
}

void az_draw_liquid(const az_space_state_t *state) {
  AZ_ARRAY_LOOP(gravfield, state->gravfields) {
    if (az_is_liquid(gravfield->kind)) {
      az_draw_gravfield(gravfield);
    }
  }
}

/*===========================================================================*/
