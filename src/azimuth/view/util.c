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

#include "azimuth/view/util.h"

#include <math.h>

#include <GL/gl.h>

#include "azimuth/util/color.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

void az_gl_color(az_color_t color) {
  glColor4ub(color.r, color.g, color.b, color.a);
}

void az_gl_rotated(double radians) {
  glRotated(AZ_RAD2DEG(radians), 0, 0, 1);
}

void az_gl_translated(az_vector_t v) {
  glTranslated(v.x, v.y, 0);
}

void az_gl_vertex(az_vector_t v) {
  glVertex2d(v.x, v.y);
}

void az_draw_cracks(az_random_seed_t *seed, az_vector_t origin, double angle,
                    double length) {
  if (length <= 0.0) return;
  glColor4f(0, 0, 0, 0.25);
  glBegin(GL_LINES); {
    az_gl_vertex(origin);
    az_vpluseq(&origin,
               az_vpolar((4.0 + az_rand_sdouble(seed)) * fmin(1, length),
                         angle + az_rand_sdouble(seed) * AZ_DEG2RAD(40)));
    az_gl_vertex(origin);
    length -= 1.0;
    int sign = az_rand_udouble(seed) < 0.5 ? -1 : 1;
    while (length > 0.0) {
      double angle_diverge =
        sign * (AZ_DEG2RAD(45) + az_rand_sdouble(seed) * AZ_DEG2RAD(30));
      az_gl_vertex(origin);
      az_gl_vertex(az_vadd(origin, az_vpolar((4.0 + az_rand_sdouble(seed)) *
                                             cbrt(length),
                                             angle + angle_diverge)));
      angle_diverge -=
        sign * (AZ_DEG2RAD(90) + az_rand_sdouble(seed) * AZ_DEG2RAD(30));
      az_gl_vertex(origin);
      az_vpluseq(&origin, az_vpolar((4.0 + az_rand_sdouble(seed)) *
                                    fmin(1, length),
                                    angle + angle_diverge));
      az_gl_vertex(origin);
      length -= 1.0;
      sign = -sign;
    }
  } glEnd();
}

/*===========================================================================*/
