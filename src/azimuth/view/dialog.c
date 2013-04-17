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

#include "azimuth/view/dialog.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>

#include <GL/gl.h>

#include "azimuth/state/dialog.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/polygon.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

static GLuint portrait_display_lists_start;

// Offsets from portrait_display_lists_start for various portraits:
#define HOPPER_PAUSED_INDEX 0
#define HOPPER_TALKING_INDEX 1
#define NUM_PORTRAIT_DISPLAY_LISTS 2

/*===========================================================================*/

static void draw_portrait_polygon(
    double bezel, az_color_t color1, az_color_t color2, az_polygon_t polygon) {
  const int n = polygon.num_vertices;
  assert(n >= 3);
  // Calculate bezel vertices:
  az_vector_t bezel_vertices[n];
  for (int i = 0; i < n; ++i) {
    const az_vector_t a = polygon.vertices[az_modulo(i - 1, n)];
    const az_vector_t b = polygon.vertices[i];
    const az_vector_t c = polygon.vertices[az_modulo(i + 1, n)];
    const double ta = az_vtheta(az_vsub(a, b));
    const double tc = az_vtheta(az_vsub(c, b));
    const double tt = 0.5 * az_mod2pi_nonneg(ta - tc);
    assert(0.0 < tt && tt < AZ_TWO_PI);
    bezel_vertices[i] = az_vadd(b, az_vpolar(bezel / sin(tt), tc + tt));
  }
  // Draw the quad strip:
  glBegin(GL_QUAD_STRIP); {
    for (int i = n - 1, i2 = 0; i < n; i = i2++) {
      glColor4ub(color1.r, color1.g, color1.b, color1.a);
      glVertex2d(polygon.vertices[i].x, polygon.vertices[i].y);
      glColor4ub(color2.r, color2.g, color2.b, color2.a);
      glVertex2d(bezel_vertices[i].x, bezel_vertices[i].y);
    }
  } glEnd();
}

static const az_vector_t hopper_paused_vertices[] = {
  // Back/neck:
  {20, 0}, {32, 15}, {33, 21},
  // Hair rear:
  {28, 30}, {26, 43}, {27, 55},
  // Ponytail bottom edge:
  {32, 70}, {30, 71}, {26, 69}, {23, 62}, {22, 45}, {18, 25},
  // Ponytail tip:
  {20, 20}, {16, 16}, {13, 17}, {10, 25}, {10, 32}, {7, 37},
  // Ponytail top edge:
  {9, 50}, {8, 65}, {10, 75}, {15, 82}, {25, 84}, {31, 82}, {33, 79}, {35, 78},
  // Hair top:
  {42, 85}, {50, 90}, {60, 92}, {70, 90}, {75, 88},
  // Hair front:
  {84, 82}, {89, 76}, {92, 69}, {91, 67},
  // Forehead:
  {89, 65}, {88.5, 55}, {87, 51},
  // Nose:
  {86, 46}, {90, 33}, {89, 31}, {85, 31},
  // Mouth:
  {84, 29}, {84, 25}, {84.5, 23}, {82, 22}, {83, 20}, {82, 18}, {81, 17},
  // Chin:
  {81, 13}, {80, 11}, {78, 10}, {77, 10}, {73, 11},
  // Front/neck:
  {68, 14}, {65, 10}, {72, 5}, {77, 0},
  // Border:
  {100, 0}, {100, 100}, {0, 100}, {0, 0}
};

static const az_vector_t hopper_talking_vertices[] = {
  // Back/neck:
  {20, 0}, {32, 15}, {33, 21},
  // Hair rear:
  {28, 30}, {26, 43}, {27, 55},
  // Ponytail bottom edge:
  {32, 70}, {30, 71}, {26, 69}, {23, 62}, {22, 45}, {18, 25},
  // Ponytail tip:
  {20, 20}, {16, 16}, {13, 17}, {10, 25}, {10, 32}, {7, 37},
  // Ponytail top edge:
  {9, 50}, {8, 65}, {10, 75}, {15, 82}, {25, 84}, {31, 82}, {33, 79}, {35, 78},
  // Hair top:
  {42, 85}, {50, 90}, {60, 92}, {70, 90}, {75, 88},
  // Hair front:
  {84, 82}, {89, 76}, {92, 69}, {91, 67},
  // Forehead:
  {89, 65}, {88.5, 55}, {87, 51},
  // Nose:
  {86, 46}, {90, 33}, {89, 31}, {85, 31},
  // Mouth:
  {84, 29}, {84, 27}, {84.5, 25},
  {82, 24}, {79, 22}, {79, 21}, {82, 19},
  {83, 17}, {82, 15}, {81, 14},
  // Chin:
  {81, 12}, {80, 10}, {78, 9}, {77, 9}, {73, 10},
  // Front/neck:
  {68, 14}, {65, 10}, {72, 5}, {77, 0},
  // Border:
  {100, 0}, {100, 100}, {0, 100}, {0, 0}
};

/*===========================================================================*/

void az_init_portrait_drawing(void) {
  portrait_display_lists_start = glGenLists(NUM_PORTRAIT_DISPLAY_LISTS);
  if (portrait_display_lists_start == 0u) {
    AZ_FATAL("glGenLists failed.\n");
  }
  // Compile Hopper portrait:
  glNewList(portrait_display_lists_start + HOPPER_PAUSED_INDEX, GL_COMPILE); {
    const az_polygon_t polygon = AZ_INIT_POLYGON(hopper_paused_vertices);
    draw_portrait_polygon(4.5, (az_color_t){0, 255, 255, 255},
                          (az_color_t){0, 255, 255, 0}, polygon);
  } glEndList();
  glNewList(portrait_display_lists_start + HOPPER_TALKING_INDEX, GL_COMPILE); {
    const az_polygon_t polygon = AZ_INIT_POLYGON(hopper_talking_vertices);
    draw_portrait_polygon(4.5, (az_color_t){0, 255, 255, 255},
                          (az_color_t){0, 255, 255, 0}, polygon);
  } glEndList();
  // TODO: Compile other portraits.
}

/*===========================================================================*/

void az_draw_portrait(az_portrait_t portrait, bool talking, az_clock_t clock) {
  assert(portrait != AZ_POR_NOTHING);
  GLuint display_list = portrait_display_lists_start;
  switch (portrait) {
    case AZ_POR_NOTHING: AZ_ASSERT_UNREACHABLE();
    case AZ_POR_HOPPER:
      display_list += (talking && az_clock_mod(3, 3, clock) > 0 ?
                       HOPPER_TALKING_INDEX : HOPPER_PAUSED_INDEX);
      break;
    case AZ_POR_HQ: return; // TODO
    case AZ_POR_CPU_A: return; // TODO
    case AZ_POR_CPU_B: return; // TODO
    case AZ_POR_CPU_C: return; // TODO
    case AZ_POR_TRICHORD: return; // TODO
  }
  assert(glIsList(display_list));
  glCallList(display_list);
}

/*===========================================================================*/
