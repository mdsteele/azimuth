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
#define CPU_A_PAUSED_INDEX 2
#define CPU_B_PAUSED_INDEX 3
#define TRICHORD_PAUSED_INDEX 4
#define TRICHORD_TALKING_INDEX 5
#define NUM_PORTRAIT_DISPLAY_LISTS 6

/*===========================================================================*/

static void draw_computer_circle(
    double cx, double cy, az_color_t color1, az_color_t color2) {
  const double r1 = 3;
  const double r2 = 5;
  glBegin(GL_QUAD_STRIP); {
    for (int i = 0; i <= 360; i += 30) {
      const double c = cos(AZ_DEG2RAD(i));
      const double s = sin(AZ_DEG2RAD(i));
      glColor4ub(color1.r, color1.g, color1.b, color1.a);
      glVertex2d(cx + r1 * c, cy + r1 * s);
      glColor4ub(color2.r, color2.g, color2.b, color2.a);
      glVertex2d(cx + r2 * c, cy + r2 * s);
    }
  } glEnd();
}

static void draw_computer_blinkenlight(
    double cx, double cy, az_color_t color1) {
  const double r = 2;
  glBegin(GL_TRIANGLE_FAN); {
    glColor4ub(color1.r, color1.g, color1.b, color1.a);
    glVertex2d(cx, cy);
    for (int i = 0; i <= 360; i += 30) {
      const double c = cos(AZ_DEG2RAD(i));
      const double s = sin(AZ_DEG2RAD(i));
      glVertex2d(cx + r * c, cy + r * s);
    }
  } glEnd();
}

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

static const az_vector_t cpu_a1_vertices[] = {
  {14, 100}, {0, 100}, {0, 0}, {37, 0}, {37, 37}, {36, 37}, {36, 40}, {30, 40},
  {30, 46}, {27, 46}, {27, 47},
  // Bend in pipe:
  {17, 47}, {15, 48}, {14, 50}
};
static const az_vector_t cpu_a2_vertices[] = {
  {57, 0}, {57, 37}, {56, 37}, {56, 40}, {44, 40}, {44, 37}, {43, 37}, {43, 0}
};
static const az_vector_t cpu_a3_vertices[] = {
  {100, 0}, {100, 47}, {73, 47}, {73, 46}, {70, 46}, {70, 40},
  {64, 40}, {64, 37}, {63, 37}, {63, 0}
};
static const az_vector_t cpu_a4_vertices[] = {
  {100, 100}, {63, 100}, {63, 63}, {64, 63}, {64, 60},
  {70, 60}, {70, 54}, {73, 54}, {73, 53}, {100, 53}
};
static const az_vector_t cpu_a5_vertices[] = {
  {20, 100}, {20, 53}, {27, 53}, {27, 54}, {30, 54},
  {30, 60}, {56, 60}, {56, 63}, {57, 63}, {57, 100}
};

static const az_vector_t cpu_b1_vertices[] = {
  {30, 60}, {36, 60}, {36, 63}, {37, 63}, {37, 70}, {0, 70},
  {0, 20}, {37, 20}, {37, 37}, {36, 37}, {36, 40}, {30, 40}
};
static const az_vector_t cpu_b2_vertices[] = {
  {100, 0}, {100, 24}, {60, 24}, {58, 25}, {57, 27},
  {57, 37}, {56, 37}, {56, 40}, {44, 40}, {44, 37}, {43, 37},
  {43, 17}, {42, 15}, {40, 14}, {0, 14}, {0, 0}
};
static const az_vector_t cpu_b3_vertices[] = {
  {100, 47}, {83, 47}, {73, 47}, {73, 46}, {70, 46}, {70, 40}, {64, 40},
  {64, 37}, {63, 37}, {63, 30}, {100, 30}
};
static const az_vector_t cpu_b4_vertices[] = {
  {63, 100}, {63, 63}, {64, 63}, {64, 60}, {70, 60}, {70, 54}, {73, 54},
  {73, 53}, {100, 53}, {100, 100}
};
static const az_vector_t cpu_b5_vertices[] = {
  {0, 100}, {0, 76}, {40, 76}, {42, 75}, {43, 73}, {43, 63}, {44, 63},
  {44, 60}, {56, 60}, {56, 63}, {57, 63}, {57, 100}
};

static const az_vector_t trichord_paused1_vertices[] = {
  {100, 100},
  // Right side of top arm:
  {81, 100}, {78, 91}, {73, 85}, {68, 82}, {62, 78}, {59, 70},
  // Hub top-right:
  {58, 62}, {64, 59}, {68, 55}, {70, 47}, {69, 40},
  // Top of right arm:
  {75, 38}, {80, 34}, {88, 20}, {94, 13}, {100, 10}
};
static const az_vector_t trichord_paused2_vertices[] = {
  {0, 100},
  // Top of left arm:
  {0, 28}, {8, 27}, {11, 29}, {15, 34}, {21, 41}, {26, 45},
  // Hub top-left:
  {31, 47}, {32, 55}, {35, 59}, {40, 62}, {45, 64},
  // Left side of top arm:
  {46, 70}, {50, 80}, {54, 85}, {63, 90}, {66, 94}, {66, 100}
};
static const az_vector_t trichord_paused3_vertices[] = {
  {0, 0},
  // Bottom of right arm:
  {85, 0}, {81, 10}, {75, 20}, {70, 26},
  // Hub bottom:
  {60, 30}, {52, 28}, {47, 28}, {41, 30}, {35, 35},
  // Bottom of left arm:
  {29, 33}, {24, 28}, {20, 23}, {15, 19}, {7, 16}, {0, 10}
};
static const az_vector_t trichord4_vertices[] = {
  {69, 100}, {70, 92}, {74, 94}, {77, 100}
};
static const az_vector_t trichord5_vertices[] = {
  {0, 15}, {8, 22}, {0, 24}
};
static const az_vector_t trichord6_vertices[] = {
  {100, 0}, {100, 5}, {89, 8}, {90, 0}
};
static const az_vector_t trichord_talking1_vertices[] = {
  {100, 100},
  // Right side of top arm:
  {81, 100}, {78, 91}, {73, 85}, {68, 82}, {62, 78}, {59, 70},
  // Hub top-right:
  {59, 63}, {65, 60}, {69, 56}, {71, 48}, {70, 41},
  // Top of right arm:
  {75, 38}, {80, 34}, {88, 20}, {94, 13}, {100, 10}
};
static const az_vector_t trichord_talking2_vertices[] = {
  {0, 100},
  // Top of left arm:
  {0, 28}, {8, 27}, {11, 29}, {15, 34}, {21, 41}, {26, 45},
  // Hub top-left:
  {30, 48}, {31, 56}, {34, 60}, {39, 63}, {44, 65},
  // Left side of top arm:
  {46, 70}, {50, 80}, {54, 85}, {63, 90}, {66, 94}, {66, 100}
};
static const az_vector_t trichord_talking3_vertices[] = {
  {0, 0},
  // Bottom of right arm:
  {85, 0}, {81, 10}, {75, 20}, {70, 26},
  // Hub bottom:
  {61, 29}, {52, 27}, {47, 27}, {41, 29}, {34, 34},
  // Bottom of left arm:
  {29, 33}, {24, 28}, {20, 23}, {15, 19}, {7, 16}, {0, 10}
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
    draw_portrait_polygon(5, (az_color_t){0, 255, 255, 255},
                          (az_color_t){255, 32, 0, 0}, polygon);
  } glEndList();
  glNewList(portrait_display_lists_start + HOPPER_TALKING_INDEX, GL_COMPILE); {
    const az_polygon_t polygon = AZ_INIT_POLYGON(hopper_talking_vertices);
    draw_portrait_polygon(5, (az_color_t){0, 255, 255, 255},
                          (az_color_t){255, 32, 0, 0}, polygon);
  } glEndList();
  // Compile CPU A portrait:
  glNewList(portrait_display_lists_start + CPU_A_PAUSED_INDEX, GL_COMPILE); {
    const az_color_t color1 = {255, 128, 255, 255};
    const az_color_t color2 = {255, 0, 255, 0};
    const az_polygon_t polygon1 = AZ_INIT_POLYGON(cpu_a1_vertices);
    draw_portrait_polygon(6, color1, color2, polygon1);
    const az_polygon_t polygon2 = AZ_INIT_POLYGON(cpu_a2_vertices);
    draw_portrait_polygon(6, color1, color2, polygon2);
    const az_polygon_t polygon3 = AZ_INIT_POLYGON(cpu_a3_vertices);
    draw_portrait_polygon(6, color1, color2, polygon3);
    const az_polygon_t polygon4 = AZ_INIT_POLYGON(cpu_a4_vertices);
    draw_portrait_polygon(6, color1, color2, polygon4);
    const az_polygon_t polygon5 = AZ_INIT_POLYGON(cpu_a5_vertices);
    draw_portrait_polygon(6, color1, color2, polygon5);
    draw_computer_circle(40, 50, color1, (az_color_t){255, 0, 255, 128});
  } glEndList();
  // Compile CPU B portrait:
  glNewList(portrait_display_lists_start + CPU_B_PAUSED_INDEX, GL_COMPILE); {
    const az_color_t color1 = {128, 255, 128, 255};
    const az_color_t color2 = {0, 255, 128, 0};
    const az_polygon_t polygon1 = AZ_INIT_POLYGON(cpu_b1_vertices);
    draw_portrait_polygon(6, color1, color2, polygon1);
    const az_polygon_t polygon2 = AZ_INIT_POLYGON(cpu_b2_vertices);
    draw_portrait_polygon(6, color1, color2, polygon2);
    const az_polygon_t polygon3 = AZ_INIT_POLYGON(cpu_b3_vertices);
    draw_portrait_polygon(6, color1, color2, polygon3);
    const az_polygon_t polygon4 = AZ_INIT_POLYGON(cpu_b4_vertices);
    draw_portrait_polygon(6, color1, color2, polygon4);
    const az_polygon_t polygon5 = AZ_INIT_POLYGON(cpu_b5_vertices);
    draw_portrait_polygon(6, color1, color2, polygon5);
    draw_computer_circle(60, 50, color1, (az_color_t){0, 255, 0, 128});
  } glEndList();
  // Compile Trichord portrait:
  glNewList(portrait_display_lists_start + TRICHORD_PAUSED_INDEX,
            GL_COMPILE); {
    const az_color_t color1 = {255, 0, 0, 255};
    const az_color_t color2 = {255, 0, 255, 0};
    const az_polygon_t polygon1 = AZ_INIT_POLYGON(trichord_paused1_vertices);
    draw_portrait_polygon(4, color1, color2, polygon1);
    const az_polygon_t polygon2 = AZ_INIT_POLYGON(trichord_paused2_vertices);
    draw_portrait_polygon(4, color1, color2, polygon2);
    const az_polygon_t polygon3 = AZ_INIT_POLYGON(trichord_paused3_vertices);
    draw_portrait_polygon(4, color1, color2, polygon3);
    const az_polygon_t polygon4 = AZ_INIT_POLYGON(trichord4_vertices);
    draw_portrait_polygon(3, color1, color2, polygon4);
    const az_polygon_t polygon5 = AZ_INIT_POLYGON(trichord5_vertices);
    draw_portrait_polygon(3, color1, color2, polygon5);
    const az_polygon_t polygon6 = AZ_INIT_POLYGON(trichord6_vertices);
    draw_portrait_polygon(3, color1, color2, polygon6);
  } glEndList();
  glNewList(portrait_display_lists_start + TRICHORD_TALKING_INDEX,
            GL_COMPILE); {
    const az_color_t color1 = {255, 0, 0, 255};
    const az_color_t color2 = {255, 0, 255, 0};
    const az_polygon_t polygon1 = AZ_INIT_POLYGON(trichord_talking1_vertices);
    draw_portrait_polygon(4, color1, color2, polygon1);
    const az_polygon_t polygon2 = AZ_INIT_POLYGON(trichord_talking2_vertices);
    draw_portrait_polygon(4, color1, color2, polygon2);
    const az_polygon_t polygon3 = AZ_INIT_POLYGON(trichord_talking3_vertices);
    draw_portrait_polygon(4, color1, color2, polygon3);
    const az_polygon_t polygon4 = AZ_INIT_POLYGON(trichord4_vertices);
    draw_portrait_polygon(3, color1, color2, polygon4);
    const az_polygon_t polygon5 = AZ_INIT_POLYGON(trichord5_vertices);
    draw_portrait_polygon(3, color1, color2, polygon5);
    const az_polygon_t polygon6 = AZ_INIT_POLYGON(trichord6_vertices);
    draw_portrait_polygon(3, color1, color2, polygon6);
  } glEndList();
}

/*===========================================================================*/

void az_draw_portrait(az_portrait_t portrait, bool talking, az_clock_t clock) {
  assert(portrait != AZ_POR_NOTHING);
  GLuint display_list = portrait_display_lists_start;
  switch (portrait) {
    case AZ_POR_NOTHING: AZ_ASSERT_UNREACHABLE();
    case AZ_POR_HOPPER:
      display_list += (talking && az_clock_mod(3, 3, clock) != 0 ?
                       HOPPER_TALKING_INDEX : HOPPER_PAUSED_INDEX);
      break;
    case AZ_POR_HQ: return; // TODO
    case AZ_POR_CPU_A:
      display_list += CPU_A_PAUSED_INDEX;
      if (az_clock_mod(2, (talking ? 3 : 25), clock)) {
        draw_computer_blinkenlight(40, 50, (az_color_t){255, 128, 255, 255});
      }
      break;
    case AZ_POR_CPU_B:
      display_list += CPU_B_PAUSED_INDEX;
      if (az_clock_mod(2, (talking ? 3 : 25), clock)) {
        draw_computer_blinkenlight(60, 50, (az_color_t){128, 255, 128, 255});
      }
      break;
    case AZ_POR_CPU_C: return; // TODO
    case AZ_POR_TRICHORD:
      display_list += (talking && az_clock_mod(2, 5, clock) != 0 ?
                       TRICHORD_TALKING_INDEX : TRICHORD_PAUSED_INDEX);
      break;
  }
  assert(glIsList(display_list));
  glCallList(display_list);
}

/*===========================================================================*/
