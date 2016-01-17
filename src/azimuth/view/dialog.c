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
#include "azimuth/util/color.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/polygon.h"
#include "azimuth/util/vector.h"
#include "azimuth/view/util.h"

/*===========================================================================*/

static GLuint portrait_display_lists_start;

// Offsets from portrait_display_lists_start for various portraits:
#define HOPPER_PAUSED_INDEX 0
#define HOPPER_TALKING_INDEX 1
#define HQ_PAUSED_INDEX 2
#define HQ_TALKING_INDEX 3
#define CPU_A_PAUSED_INDEX 4
#define CPU_B_PAUSED_INDEX 5
#define CPU_C_PAUSED_INDEX 6
#define TRICHORD_PAUSED_INDEX 7
#define TRICHORD_TALKING_INDEX 8
#define AZIMUTH_PAUSED_INDEX 9
#define HUMAN_PAUSED_INDEX 10
#define HUMAN_TALKING_INDEX 11
#define NUM_PORTRAIT_DISPLAY_LISTS 12

/*===========================================================================*/

static void draw_computer_circle(
    double cx, double cy, az_color_t color1, az_color_t color2) {
  const double r1 = 3;
  const double r2 = 5;
  glBegin(GL_QUAD_STRIP); {
    for (int i = 0; i <= 360; i += 30) {
      const double c = cos(AZ_DEG2RAD(i));
      const double s = sin(AZ_DEG2RAD(i));
      az_gl_color(color1);
      glVertex2d(cx + r1 * c, cy + r1 * s);
      az_gl_color(color2);
      glVertex2d(cx + r2 * c, cy + r2 * s);
    }
  } glEnd();
}

static void draw_computer_blinkenlight(
    double cx, double cy, az_color_t color) {
  const double r = 2;
  glBegin(GL_TRIANGLE_FAN); {
    az_gl_color(color);
    glVertex2d(cx, cy);
    for (int i = 0; i <= 360; i += 30) {
      const double c = cos(AZ_DEG2RAD(i));
      const double s = sin(AZ_DEG2RAD(i));
      glVertex2d(cx + r * c, cy + r * s);
    }
  } glEnd();
}

static void draw_video_lines(az_color_t color, az_clock_t clock) {
  const int num_lines = 6;
  const float spacing = 100.0 / num_lines;
  const float offset = az_clock_mod(60, 1, clock) / 60.0;
  glBegin(GL_LINES); {
    az_gl_color(color);
    glVertex2f(100.0f * offset, 0);
    glVertex2f(0, spacing * offset);
    for (int i = 0; i < num_lines - 1; ++i) {
      glVertex2f(0, spacing * (offset + i + 1));
      glVertex2f(100, spacing * (offset + i));
    }
    glVertex2f(100, spacing * (offset + num_lines - 1));
    glVertex2f(100.0f * offset, 100);
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
      az_gl_color(color1);
      az_gl_vertex(polygon.vertices[i]);
      az_gl_color(color2);
      az_gl_vertex(bezel_vertices[i]);
    }
  } glEnd();
}

static const az_vector_t hopper_paused1_vertices[] = {
  // Hair:
  {0, 7.5}, {5, 9.5},
  {6.5, 11}, {5.5, 12}, {5, 13.5}, {5.5, 15.5}, {8, 15}, {10, 15.5},
  {15, 18}, {13.5, 19.5}, {13, 21}, {13.5, 23}, {15.3, 25}, {15.4, 25.05},
  {15.5, 25}, {16, 23}, {18, 22.5}, {21, 23}, {24, 25},
  // Back of the helmet:
  {30, 31.5}, {27, 32}, {25, 34}, {21, 45}, {20, 57}, {21, 64},
  {18, 65}, {18, 68}, {19.5, 72}, {21, 74},
  // Top of the helmet:
  {26, 74}, {30.5, 82.5}, {38, 87}, {42, 89}, {52, 90.5}, {62, 90}, {67, 89},
  {75, 85}, {82, 79}, {86, 74}, {90, 64}, {91, 60}, {91, 55}, {88, 52.5},
  // Face:
  {83, 51}, {83, 47}, {87, 37}, {86.8, 35}, {85, 34}, {82.5, 33.5}, {82, 30},
  // Mouth:
  {81, 29}, {80, 28.5}, {80, 26.8}, {78.2, 25.6},
  // Chin:
  {77.1, 23}, {76.8, 20}, {75.1, 18}, {72.1, 17.4}, {70.1, 17.8}, {66, 18.5},
  // Neck/front:
  {64.8, 12}, {65.5, 12}, {66, 9}, {78, 0},
  // Border:
  {100, 0}, {100, 100}, {0, 100}
};
static const az_vector_t hopper2_vertices[] = {
  {21, 0}, {23.5, 0}, {25, 7}, {22.5, 4}
};

static const az_vector_t hopper_talking1_vertices[] = {
  // Hair:
  {0, 7.5}, {5, 9.5},
  {6.5, 11}, {5.5, 12}, {5, 13.5}, {5.5, 15.5}, {8, 15}, {10, 15.5},
  {15, 18}, {13.5, 19.5}, {13, 21}, {13.5, 23}, {15.3, 25}, {15.4, 25.05},
  {15.5, 25}, {16, 23}, {18, 22.5}, {21, 23}, {24, 25},
  // Back of the helmet:
  {30, 31.5}, {27, 32}, {25, 34}, {21, 45}, {20, 57}, {21, 64},
  {18, 65}, {18, 68}, {19.5, 72}, {21, 74},
  // Top of the helmet:
  {26, 74}, {30.5, 82.5}, {38, 87}, {42, 89}, {52, 90.5}, {62, 90}, {67, 89},
  {75, 85}, {82, 79}, {86, 74}, {90, 64}, {91, 60}, {91, 55}, {88, 52.5},
  // Face:
  {83, 51}, {83, 47.1}, {87, 37.2}, {86.8, 35.3}, {85, 34.3}, {82.5, 33.8},
  {82, 30.3},
  // Mouth:
  {82, 30}, {81, 29.5}, {79, 28.5}, {79, 27.5}, {80, 25.8}, {78.2, 24.6},
  // Chin:
  {77.1, 22}, {76.8, 19}, {75.1, 17}, {72.1, 16.7}, {70.1, 17.3}, {66, 18.5},
  // Neck/front:
  {64.9, 12}, {65.6, 12}, {66.1, 9}, {78, 0},
  // Border:
  {100, 0}, {100, 100}, {0, 100}
};

static const az_vector_t hq_paused1_vertices[] = {
  // Left neck/shoulder:
  {10, 6}, {20, 10}, {30, 18}, {31, 25},
  // Left earpiece:
  {31, 28}, {27, 29}, {25, 30}, {24, 32}, {23, 38},
  {22, 38}, {19.5, 42}, {19, 51}, {21, 55},
  // Top of head:
  {24, 57}, {27, 73}, {30, 82}, {36, 88}, {41, 89}, {50, 90},
  {59, 89}, {64, 88}, {70, 82}, {73, 73}, {76, 57},
  // Right earpiece:
  {79, 55}, {81, 51}, {80.5, 42}, {78, 39}, {74, 37}, {70, 37},
  // Right neck/shoulder:
  {69, 25}, {70, 18}, {80, 10}, {90, 6},
  // Border:
  {100, 0}, {100, 100}, {0, 100}, {0, 0}
};
static const az_vector_t hq_paused2_vertices[] = {
  {30, 37}, {25, 37.5}, {25.5, 34}, {27, 31}, {31, 30}
};
static const az_vector_t hq_paused3_vertices[] = {
  {72, 59}, {74, 58}, {72, 68}
};
static const az_vector_t hq_paused4_vertices[] = {
  {26, 58}, {28, 59}, {28, 68}
};

static const az_vector_t hq_talking1_vertices[] = {
  // Left neck/shoulder:
  {10, 6}, {20, 10}, {30, 18}, {31, 25},
  // Left earpiece:
  {31, 27}, {27, 28}, {25, 29}, {24, 31}, {23, 38},
  {22, 38}, {19.5, 42}, {19, 51}, {21, 55},
  // Top of head:
  {24, 57}, {27, 73}, {30, 82}, {36, 88}, {41, 89}, {50, 90},
  {59, 89}, {64, 88}, {70, 82}, {73, 73}, {76, 57},
  // Right earpiece:
  {79, 55}, {81, 51}, {80.5, 42}, {78, 39}, {74, 37}, {70, 37},
  // Right neck/shoulder:
  {69, 25}, {70, 18}, {80, 10}, {90, 6},
  // Border:
  {100, 0}, {100, 100}, {0, 100}, {0, 0}
};
static const az_vector_t hq_talking2_vertices[] = {
  {30, 37}, {25, 37.5}, {25.5, 33}, {27, 30}, {31, 29}
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

static const az_vector_t cpu_c1_vertices[] = {
  {0, 100}, {0, 53}, {27, 53}, {27, 54}, {30, 54},
  {30, 60}, {36, 60}, {36, 63}, {37, 63}, {37, 100}
};
static const az_vector_t cpu_c2_vertices[] = {
  {100, 100}, {43, 100}, {43, 63}, {44, 63}, {44, 60}, {56, 60},
  {56, 63}, {57, 63}, {57, 73}, {58, 75}, {60, 76}, {100, 76}
};
static const az_vector_t cpu_c3_vertices[] = {
  {100, 70}, {63, 70}, {63, 63}, {64, 63}, {64, 60}, {70, 60},
  {70, 54}, {73, 54}, {73, 53}, {83, 53}, {100, 53}
};
static const az_vector_t cpu_c4_vertices[] = {
  {100, 0}, {100, 47}, {73, 47}, {73, 46}, {70, 46}, {70, 40}, {54, 40},
  {54, 37}, {53, 37}, {53, 20}, {52, 18}, {50, 17}, {33, 17}, {33, 0}
};
static const az_vector_t cpu_c5_vertices[] = {
  {0, 0}, {27, 0}, {27, 20}, {28, 22}, {30, 23}, {47, 23}, {47, 37},
  {46, 37}, {46, 40}, {30, 40}, {30, 46}, {27, 46}, {27, 47}, {0, 47}
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

static const az_vector_t azimuth1_vertices[] = {
  {100, 100}, {73.96, 100}, {62.76, 72.96}, {63.68, 72.58}, {62.53, 69.81},
  {67.68, 67.68}, {69.81, 62.53}, {72.58, 63.68}, {72.96, 62.76}, {100, 73.96}
};
static const az_vector_t azimuth2_vertices[] = {
  {32.54, 100}, {42.78, 75.26}, {43.71, 75.64}, {44.86, 72.87},
  {50, 75}, {55.14, 72.87}, {56.29, 75.64}, {57.22, 75.26}, {67.46, 100}
};
static const az_vector_t azimuth3_vertices[] = {
  {0, 100}, {0, 73.96}, {27.04, 62.76}, {27.42, 63.68}, {30.19, 62.53},
  {32.32, 67.68}, {37.47, 69.81}, {36.32, 72.58}, {37.24, 72.96}, {26.04, 100}
};
static const az_vector_t azimuth4_vertices[] = {
  {0, 32.54}, {24.74, 42.78}, {24.36, 43.71}, {27.13, 44.86},
  {25, 50}, {27.13, 55.14}, {24.36, 56.29}, {24.74, 57.22}, {0, 67.46}
};
static const az_vector_t azimuth5_vertices[] = {
  {0, 0}, {26.04, 0}, {37.24, 27.04}, {36.32, 27.42}, {37.47, 30.19},
  {32.32, 32.32}, {30.19, 37.47}, {27.42, 36.32}, {27.04, 37.24}, {0, 26.04}
};
static const az_vector_t azimuth6_vertices[] = {
  {67.46, 0}, {57.22, 24.74}, {56.29, 24.36}, {55.14, 27.13},
  {50, 25}, {44.86, 27.13}, {43.71, 24.36}, {42.78, 24.74}, {32.54, 0}
};
static const az_vector_t azimuth7_vertices[] = {
  {100, 0}, {100, 26.04}, {72.96, 37.24}, {72.58, 36.32}, {69.81, 37.47},
  {67.68, 32.32}, {62.53, 30.19}, {63.68, 27.42}, {62.76, 27.04}, {73.96, 0}
};
static const az_vector_t azimuth8_vertices[] = {
  {100, 67.46}, {75.26, 57.22}, {75.64, 56.29}, {72.87, 55.14},
  {75, 50}, {72.87, 44.86}, {75.64, 43.71}, {75.26, 42.78}, {100, 32.54}
};

static const az_vector_t human_paused_vertices[] = {
  {100, 100}, {0, 100},
  // Left shoulder:
  {0, 0}, {5, 5.5}, {8, 8}, {14, 11}, {20, 13}, {31.5, 17.5},
  // Left neck wisp of hair:
  {32, 19.7}, {27, 18.5}, {20.5, 19}, {20.4, 19.1}, {20.5, 19.2}, {25, 19.5},
  {28.4, 21}, {30.7, 23.3},
  // Left side of head:
  {28.5, 29.3}, {25.5, 35.6}, {24.7, 38}, {24.5, 42.2},
  // Left side wisp of hair:
  {19, 40.5}, {18.9, 40.55}, {19, 40.7}, {23.1, 44.2},
  // Top half of head:
  {24.6, 46.8}, {29, 57}, {33.7, 72.2}, {36.8, 77.4}, {40.3, 80.4},
  {44.3, 82.3}, {49.1, 83}, {51.2, 84.1}, {54.5, 84.1}, {60, 83},
  {63.5, 80.7}, {69, 75.4}, {72.5, 70}, {74.5, 67}, {78.3, 59}, {80, 52},
  // Right side wisp of hair:
  {80.6, 46}, {80.1, 40.6}, {79, 36.1}, {75.2, 29.5}, {75.1, 29.4},
  {75, 29.5}, {76.7, 35},
  // Right side of head:
  {76.7, 39}, {72, 30.3}, {71, 28},
  // Right side of neck/collar:
  {71, 26.5}, {72, 24.5}, {74.5, 22.5}, {77.5, 21.5}, {79, 20.5}, {79.5, 18.5},
  // Right shoulder:
  {78.5, 13}, {83, 10}, {92.5, 6}, {100, 0}
};

static const az_vector_t human_talking_vertices[] = {
  {100, 100}, {0, 100},
  // Left shoulder:
  {0, 0}, {5, 5.5}, {8, 8}, {14, 11}, {20, 13}, {31.5, 17.5},
  // Left neck wisp of hair:
  {32, 20.7}, {27, 19.5}, {20.5, 20}, {20.4, 20.1}, {20.5, 20.2}, {25, 20.5},
  {28.4, 22}, {30.7, 24.3},
  // Left side of head:
  {28.5, 30.3}, {25.5, 36.6}, {24.7, 39}, {24.5, 43.2},
  // Left side wisp of hair:
  {19, 41.5}, {18.9, 41.55}, {19, 41.7}, {23.1, 45.2},
  // Top half of head:
  {24.6, 47.8}, {29, 58}, {33.7, 73.2}, {36.8, 78.4}, {40.3, 81.4},
  {44.3, 83.3}, {49.1, 84}, {51.2, 85.1}, {54.5, 85.1}, {60, 84},
  {63.5, 81.7}, {69, 76.4}, {72.5, 71}, {74.5, 68}, {78.3, 60}, {80, 53},
  // Right side wisp of hair:
  {80.6, 47}, {80.1, 41.6}, {79, 37.1}, {75.2, 30.5}, {75.1, 30.4},
  {75, 30.5}, {76.7, 36},
  // Right side of head:
  {76.7, 40}, {72, 31.3}, {71, 29},
  // Right side of neck/collar:
  {71, 26.5}, {72, 24.5}, {74.5, 22.5}, {77.5, 21.5}, {79, 20.5}, {79.5, 18.5},
  // Right shoulder:
  {78.5, 13}, {83, 10}, {92.5, 6}, {100, 0}
};

/*===========================================================================*/

void az_init_portrait_drawing(void) {
  portrait_display_lists_start = glGenLists(NUM_PORTRAIT_DISPLAY_LISTS);
  if (portrait_display_lists_start == 0u) {
    AZ_FATAL("glGenLists failed.\n");
  }
  // Compile Hopper portrait:
  glNewList(portrait_display_lists_start + HOPPER_PAUSED_INDEX, GL_COMPILE); {
    const az_color_t color1 = {0, 255, 255, 255};
    const az_color_t color2 = {255, 32, 0, 0};
    const az_polygon_t polygon1 = AZ_INIT_POLYGON(hopper_paused1_vertices);
    draw_portrait_polygon(5, color1, color2, polygon1);
    const az_polygon_t polygon2 = AZ_INIT_POLYGON(hopper2_vertices);
    draw_portrait_polygon(1.5, color1, color2, polygon2);
  } glEndList();
  glNewList(portrait_display_lists_start + HOPPER_TALKING_INDEX, GL_COMPILE); {
    const az_color_t color1 = {0, 255, 255, 255};
    const az_color_t color2 = {255, 32, 0, 0};
    const az_polygon_t polygon1 = AZ_INIT_POLYGON(hopper_talking1_vertices);
    draw_portrait_polygon(5, color1, color2, polygon1);
    const az_polygon_t polygon2 = AZ_INIT_POLYGON(hopper2_vertices);
    draw_portrait_polygon(1.5, color1, color2, polygon2);
  } glEndList();
  // Compile HQ portrait:
  glNewList(portrait_display_lists_start + HQ_PAUSED_INDEX, GL_COMPILE); {
    const az_color_t color1 = {255, 255, 0, 255};
    const az_color_t color2 = {0, 128, 128, 0};
    const az_polygon_t polygon1 = AZ_INIT_POLYGON(hq_paused1_vertices);
    draw_portrait_polygon(6, color1, color2, polygon1);
    const az_polygon_t polygon2 = AZ_INIT_POLYGON(hq_paused2_vertices);
    draw_portrait_polygon(3, color1, color2, polygon2);
    const az_polygon_t polygon3 = AZ_INIT_POLYGON(hq_paused3_vertices);
    draw_portrait_polygon(1, color1, color2, polygon3);
    const az_polygon_t polygon4 = AZ_INIT_POLYGON(hq_paused4_vertices);
    draw_portrait_polygon(1, color1, color2, polygon4);
  } glEndList();
  glNewList(portrait_display_lists_start + HQ_TALKING_INDEX, GL_COMPILE); {
    const az_color_t color1 = {255, 255, 0, 255};
    const az_color_t color2 = {0, 128, 128, 0};
    const az_polygon_t polygon1 = AZ_INIT_POLYGON(hq_talking1_vertices);
    draw_portrait_polygon(6, color1, color2, polygon1);
    const az_polygon_t polygon2 = AZ_INIT_POLYGON(hq_talking2_vertices);
    draw_portrait_polygon(3, color1, color2, polygon2);
    const az_polygon_t polygon3 = AZ_INIT_POLYGON(hq_paused3_vertices);
    draw_portrait_polygon(1, color1, color2, polygon3);
    const az_polygon_t polygon4 = AZ_INIT_POLYGON(hq_paused4_vertices);
    draw_portrait_polygon(1, color1, color2, polygon4);
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
  // Compile CPU C portrait:
  glNewList(portrait_display_lists_start + CPU_C_PAUSED_INDEX, GL_COMPILE); {
    const az_color_t color1 = {192, 192, 255, 255};
    const az_color_t color2 = {64, 64, 255, 0};
    const az_polygon_t polygon1 = AZ_INIT_POLYGON(cpu_c1_vertices);
    draw_portrait_polygon(6, color1, color2, polygon1);
    const az_polygon_t polygon2 = AZ_INIT_POLYGON(cpu_c2_vertices);
    draw_portrait_polygon(6, color1, color2, polygon2);
    const az_polygon_t polygon3 = AZ_INIT_POLYGON(cpu_c3_vertices);
    draw_portrait_polygon(6, color1, color2, polygon3);
    const az_polygon_t polygon4 = AZ_INIT_POLYGON(cpu_c4_vertices);
    draw_portrait_polygon(6, color1, color2, polygon4);
    const az_polygon_t polygon5 = AZ_INIT_POLYGON(cpu_c5_vertices);
    draw_portrait_polygon(6, color1, color2, polygon5);
    draw_computer_circle(50, 50, color1, (az_color_t){64, 64, 255, 128});
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
  // Compile Azimuth portrait:
  glNewList(portrait_display_lists_start + AZIMUTH_PAUSED_INDEX, GL_COMPILE); {
    const az_color_t color1 = {255, 255, 255, 255};
    const az_color_t color2 = {0, 128, 128, 0};
    const az_polygon_t polygon1 = AZ_INIT_POLYGON(azimuth1_vertices);
    draw_portrait_polygon(6, color1, color2, polygon1);
    const az_polygon_t polygon2 = AZ_INIT_POLYGON(azimuth2_vertices);
    draw_portrait_polygon(6, color1, color2, polygon2);
    const az_polygon_t polygon3 = AZ_INIT_POLYGON(azimuth3_vertices);
    draw_portrait_polygon(6, color1, color2, polygon3);
    const az_polygon_t polygon4 = AZ_INIT_POLYGON(azimuth4_vertices);
    draw_portrait_polygon(6, color1, color2, polygon4);
    const az_polygon_t polygon5 = AZ_INIT_POLYGON(azimuth5_vertices);
    draw_portrait_polygon(6, color1, color2, polygon5);
    const az_polygon_t polygon6 = AZ_INIT_POLYGON(azimuth6_vertices);
    draw_portrait_polygon(6, color1, color2, polygon6);
    const az_polygon_t polygon7 = AZ_INIT_POLYGON(azimuth7_vertices);
    draw_portrait_polygon(6, color1, color2, polygon7);
    const az_polygon_t polygon8 = AZ_INIT_POLYGON(azimuth8_vertices);
    draw_portrait_polygon(6, color1, color2, polygon8);
    draw_computer_circle(44, 50, color1, (az_color_t){128, 255, 255, 128});
    draw_computer_circle(56, 50, color1, (az_color_t){128, 255, 255, 128});
  } glEndList();
  // Compile Human portrait:
  {
    const double bezel = 3.7;
    const az_color_t color1 = {160, 92, 64, 255};
    const az_color_t color2 = {160, 64, 92, 0};
    glNewList(portrait_display_lists_start + HUMAN_PAUSED_INDEX, GL_COMPILE); {
      const az_polygon_t polygon = AZ_INIT_POLYGON(human_paused_vertices);
      draw_portrait_polygon(bezel, color1, color2, polygon);
    } glEndList();
    glNewList(portrait_display_lists_start + HUMAN_TALKING_INDEX,
              GL_COMPILE); {
      const az_polygon_t polygon = AZ_INIT_POLYGON(human_talking_vertices);
      draw_portrait_polygon(bezel, color1, color2, polygon);
    } glEndList();
  }
}

/*===========================================================================*/

void az_draw_portrait(az_portrait_t portrait, bool talking, az_clock_t clock) {
  assert(portrait != AZ_POR_NOTHING);
  GLuint display_list = portrait_display_lists_start;
  az_color_t video_lines = {.a = 0};
  switch (portrait) {
    case AZ_POR_NOTHING: AZ_ASSERT_UNREACHABLE();
    case AZ_POR_HOPPER:
      display_list += (talking && az_clock_mod(3, 4, clock) != 0 ?
                       HOPPER_TALKING_INDEX : HOPPER_PAUSED_INDEX);
      break;
    case AZ_POR_HQ:
      display_list += (talking && az_clock_mod(3, 4, clock) != 0 ?
                       HQ_TALKING_INDEX : HQ_PAUSED_INDEX);
      break;
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
    case AZ_POR_CPU_C:
      display_list += CPU_C_PAUSED_INDEX;
      if (az_clock_mod(2, (talking ? 3 : 25), clock)) {
        draw_computer_blinkenlight(50, 50, (az_color_t){192, 192, 255, 255});
      }
      break;
    case AZ_POR_TRICHORD:
      display_list += (talking && az_clock_mod(2, 5, clock) != 0 ?
                       TRICHORD_TALKING_INDEX : TRICHORD_PAUSED_INDEX);
      break;
    case AZ_POR_AZIMUTH:
      display_list += AZIMUTH_PAUSED_INDEX;
      if (az_clock_mod(2, (talking ? 3 : 25), clock)) {
        draw_computer_blinkenlight(44, 50, (az_color_t){192, 192, 192, 255});
        draw_computer_blinkenlight(56, 50, (az_color_t){192, 192, 192, 255});
      }
      break;
    case AZ_POR_TRICHORD_VIDEO:
      display_list += (talking && az_clock_mod(2, 5, clock) != 0 ?
                       TRICHORD_TALKING_INDEX : TRICHORD_PAUSED_INDEX);
      video_lines = (az_color_t){255, 0, 0, 128};
      break;
    case AZ_POR_HUMAN:
      display_list += (talking && az_clock_mod(2, 6, clock) != 0 ?
                       HUMAN_TALKING_INDEX : HUMAN_PAUSED_INDEX);
      break;
    case AZ_POR_HUMAN_VIDEO:
      display_list += (talking && az_clock_mod(2, 6, clock) != 0 ?
                       HUMAN_TALKING_INDEX : HUMAN_PAUSED_INDEX);
      video_lines = (az_color_t){160, 92, 64, 255};
      break;
  }
  assert(glIsList(display_list));
  if (video_lines.a == 0u) glCallList(display_list);
  else if (az_clock_mod(3, 1, clock) != 0) {
    glCallList(display_list);
    draw_video_lines(video_lines, clock);
  }
}

/*===========================================================================*/
