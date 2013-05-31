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
#include "azimuth/util/vector.h"

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

static void draw_water_gravfield(const az_gravfield_t *gravfield) {
  assert(gravfield->kind == AZ_GRAV_WATER);
  const double semilength = gravfield->size.trapezoid.semilength;
  const double front_offset = gravfield->size.trapezoid.front_offset;
  const double front_semiwidth = gravfield->size.trapezoid.front_semiwidth;
  const double rear_semiwidth = gravfield->size.trapezoid.rear_semiwidth;
  glBegin(GL_QUAD_STRIP); {
    glColor4f(1, 1, 1, 0);
    glVertex2d(semilength + 6, -front_semiwidth + front_offset);
    glVertex2d(semilength + 6, front_semiwidth + front_offset);
    glColor4f(0.3, 0.6, 1, 0.3);
    glVertex2d(semilength, -front_semiwidth + front_offset);
    glVertex2d(semilength, front_semiwidth + front_offset);
    glColor4f(0, 0, 0.4, 0.75);
    glVertex2d(-semilength, -rear_semiwidth);
    glVertex2d(-semilength, rear_semiwidth);
  } glEnd();
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
  }
}

void az_draw_gravfield(const az_gravfield_t *gravfield) {
  assert(gravfield->kind != AZ_GRAV_NOTHING);
  glPushMatrix(); {
    glTranslated(gravfield->position.x, gravfield->position.y, 0);
    glRotated(AZ_RAD2DEG(gravfield->angle), 0, 0, 1);
    az_draw_gravfield_no_transform(gravfield);
  } glPopMatrix();
}

void az_draw_gravfields(const az_space_state_t *state) {
  AZ_ARRAY_LOOP(gravfield, state->gravfields) {
    if (gravfield->kind == AZ_GRAV_NOTHING) continue;
    if (gravfield->kind == AZ_GRAV_WATER) continue;
    az_draw_gravfield(gravfield);
  }
}

void az_draw_water(const az_space_state_t *state) {
  AZ_ARRAY_LOOP(gravfield, state->gravfields) {
    if (gravfield->kind == AZ_GRAV_WATER) {
      az_draw_gravfield(gravfield);
    }
  }
}

/*===========================================================================*/
