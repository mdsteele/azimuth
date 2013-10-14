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

#include "azimuth/view/baddie_oth.h"

#include <assert.h>
#include <math.h>

#include <GL/gl.h>

#include "azimuth/state/baddie.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/misc.h"

/*===========================================================================*/

static const az_vector_t oth_crab_triangles[] = {
  // Body:
  {0, 0}, {15, 10}, {0, 20},
  {15, -10}, {15, 10}, {0, 0},
  {0, -20}, {15, -10}, {0, 0},
  {-15, 7}, {0, 0}, {0, 20},
  {0, 0}, {-15, 7}, {-15, -7},
  {0, -20}, {0, 0}, {-15, -7},
  // Left arm:
  {15, 10}, {5, 16.66}, {24, 17},
  {24, 17}, {5, 16.66}, {22, 25},
  {22, 25}, {24, 17}, {41, 12},
  // Right arm:
  {5, -16.66}, {15, -10}, {24, -17},
  {5, -16.66}, {24, -17}, {22, -25},
  {24, -17}, {22, -25}, {41, -12},
  // Left fang:
  {25, 3}, {15, 10}, {15, 5},
  // Right fang:
  {15, -10}, {25, -3}, {15, -5},
  // Left leg:
  {-5, 16}, {-10, 11}, {-25, 22},
  // Right leg:
  {-10, -11}, {-5, -16}, {-25, -22}
};
AZ_STATIC_ASSERT(AZ_ARRAY_SIZE(oth_crab_triangles) % 3 == 0);

static const az_vector_t oth_orb_triangles[] = {
  {0, 0}, {20, 0}, {14.1, 14.1},
  {0, 0}, {14.1, 14.1}, {0, 20},
  {0, 0}, {0, 20}, {-14.1, 14.1},
  {0, 0}, {-14.1, 14.1}, {-20, 0},
  {0, 0}, {-20, 0}, {-14.1, -14.1},
  {0, 0}, {-14.1, -14.1}, {0, -20},
  {0, 0}, {0, -20}, {14.1, -14.1},
  {0, 0}, {14.1, -14.1}, {20, 0}
};
AZ_STATIC_ASSERT(AZ_ARRAY_SIZE(oth_orb_triangles) % 3 == 0);

static const az_vector_t oth_snapdragon_triangles[] = {
  // Body:
  {0, 0}, {22.5, 15}, {0, 30},
  {22.5, -15}, {22.5, 15}, {0, 0},
  {0, -30}, {22.5, -15}, {0, 0},
  {-22.5, 15}, {0, 0}, {0, 30},
  {0, 0}, {-22.5, 15}, {-22.5, -15},
  {0, -30}, {0, 0}, {-22.5, -15},
  // Tail:
  {-22.5, 0}, {-22.5, 15}, {-37.5, 7.5},
  {-22.5, -15}, {-22.5, 0}, {-37.5, -7.5},
  // Left arm:
  {22.5, 15}, {6, 48}, {7.5, 24.9},
  // Right arm:
  {6, -48}, {22.5, -15}, {7.5, -24.9},
  // Left legs:
  {0, 30}, {-15, 48}, {-7.5, 24.9},
  {-7.5, 24.9}, {-22.5, 37.5}, {-15, 19.95},
  {-15, 19.95}, {-30, 25.5}, {-22.5, 15},
  // Right legs:
  {0, -30}, {-15, -48}, {-7.5, -24.9},
  {-7.5, -24.9}, {-22.5, -37.5}, {-15, -19.95},
  {-15, -19.95}, {-30, -25.5}, {-22.5, -15},
  // Left pincer:
  {22.5, 3}, {33, 9}, {22.5, 15},
  {33, 9}, {37.5, 19.5}, {22.5, 15},
  {48, 3}, {37.5, 19.5}, {33, 9},
  // Right pincer:
  {33, -9}, {22.5, -3}, {22.5, -15},
  {37.5, -19.5}, {33, -9}, {22.5, -15},
  {37.5, -19.5}, {48, -3}, {33, -9}
};
AZ_STATIC_ASSERT(AZ_ARRAY_SIZE(oth_snapdragon_triangles) % 3 == 0);

static const az_vector_t oth_razor_triangles[] = {
  {12, 0}, {1.5, 2.59808}, {1.5, -2.59808},
  {-6, 10.3923}, {-3, 0}, {1.5, 2.59808},
  {-6, -10.3923}, {1.5, -2.59808}, {-3, 0}
};
AZ_STATIC_ASSERT(AZ_ARRAY_SIZE(oth_razor_triangles) % 3 == 0);

static const az_vector_t oth_gunship_triangles[] = {
  // Main body:
  {20, 0}, {15, -4}, {5, 0},
  {15, 4}, {20, 0}, {5, 0},
  {-14, 4}, {5, 0}, {15, 4},
  {5, 0}, {-14, -4}, {15, -4},
  {-14, -4}, {5, 0}, {-14, 4},
  // Port strut:
  {-7, 7}, {-7, 4}, {1, 7},
  {1, 4}, {1, 7}, {-7, 4},
  // Starboard strut:
  {-7, -4}, {-7, -7}, {1, -7},
  {1, -7}, {1, -4}, {-7, -4},
  // Port engine:
  {6, 12}, {8, 7}, {-10, 12},
  {-10, 12}, {-11, 7}, {8, 7},
  // Starboard engine:
  {8, -7}, {6, -12}, {-10, -12},
  {-10, -12}, {8, -7}, {-11, -7}
};
AZ_STATIC_ASSERT(AZ_ARRAY_SIZE(oth_gunship_triangles) % 3 == 0);

static void draw_oth(
    const az_baddie_t *baddie, GLfloat frozen, az_clock_t clock,
    const az_vector_t *vertices, int num_vertices) {
  assert(baddie->kind != AZ_BAD_NOTHING);
  assert(num_vertices % 3 == 0);
  const int num_triangles = num_vertices / 3;
  const bool spin = (baddie->kind != AZ_BAD_OTH_GUNSHIP);

  const double hurt_ratio = 1.0 - baddie->health / baddie->data->max_health;
  const GLfloat flare = fmax(baddie->armor_flare, 0.5 * hurt_ratio);

  for (int i = 0; i < num_triangles; ++i) {
    const az_vector_t *vs = vertices + i * 3;
    const az_vector_t center =
      az_vdiv(az_vadd(az_vadd(vs[0], vs[1]), vs[2]), 3);
    glPushMatrix(); {
      glTranslated(center.x, center.y, 0);
      if (spin) glRotated(az_clock_mod(360, 1, clock) -
                          AZ_RAD2DEG(baddie->angle * 8), 0, 0, 1);
      glBegin(GL_TRIANGLES); {
        for (int j = 0; j < 3; ++j) {
          const az_clock_t clk = clock + 2 * j;
          const GLfloat r = (az_clock_mod(6, 2, clk)     < 3 ? 1.0f : 0.25f);
          const GLfloat g = (az_clock_mod(6, 2, clk + 2) < 3 ? 1.0f : 0.25f);
          const GLfloat b = (az_clock_mod(6, 2, clk + 4) < 3 ? 1.0f : 0.25f);
          glColor3f(r + flare * (1.0f - r), (1.0f - 0.5f * flare) * g,
                    (1.0f - flare) * b + frozen * (1.0f - b));
          const az_vector_t rel = az_vsub(vs[j], center);
          glVertex2d(rel.x, rel.y);
        }
      } glEnd();
    } glPopMatrix();
  }
}

/*===========================================================================*/

void az_draw_bad_oth_crab(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  draw_oth(baddie, frozen, clock, oth_crab_triangles,
           AZ_ARRAY_SIZE(oth_crab_triangles));
}

void az_draw_bad_oth_gunship(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  draw_oth(baddie, frozen, clock, oth_gunship_triangles,
           AZ_ARRAY_SIZE(oth_gunship_triangles));
}

void az_draw_bad_oth_orb(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  draw_oth(baddie, frozen, clock, oth_orb_triangles,
           AZ_ARRAY_SIZE(oth_orb_triangles));
}

void az_draw_bad_oth_razor(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  draw_oth(baddie, frozen, clock, oth_razor_triangles,
           AZ_ARRAY_SIZE(oth_razor_triangles));
}

void az_draw_bad_oth_snapdragon(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  draw_oth(baddie, frozen, clock, oth_snapdragon_triangles,
           AZ_ARRAY_SIZE(oth_snapdragon_triangles));
}

/*===========================================================================*/
