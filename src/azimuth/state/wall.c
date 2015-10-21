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

#include "azimuth/state/wall.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>

#include "azimuth/util/misc.h"
#include "azimuth/util/polygon.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

static const az_vector_t wall_vertices_0[] = {
  {50, 25}, {-50, 25}, {-50, -25}, {50, -25}
};
static const az_vector_t wall_vertices_1[] = {
  {50, -50}, {50, 50}, {-50, 50}, {-50, -50}
};
static const az_vector_t wall_vertices_2[] = {
  {60, 25}, {-10.710678, 25}, {-60.710678, -25}, {-25.355339, -60.355339},
  {10, -25}, {60, -25}
};
static const az_vector_t wall_vertices_3[] = {
  {-20, -8}, {15, -10}, {25, 0}, {15, 10}, {-20, 8}
};
static const az_vector_t wall_vertices_4[] = {
  {-30, -10}, {15, -15}, {35, 0}, {15, 15}, {-30, 10}
};
static const az_vector_t wall_vertices_5[] = {
  {31, 10}, {33, 30}, {20, 43}, {-2, 44}, {-7, 42}, {-30, 30}, {-48, 30},
  {-59, 0}, {-41, -34}, {-28, -35}, {-20, -29}, {-5, -42}, {4, -42}, {19, -27},
  {35, -28}, {40, 0}
};
static const az_vector_t wall_vertices_girder_long[] = {
  {25, 0}, {50, 25}, {-50, 25}, {-25, 0}, {-50, -25}, {50, -25}
};
static const az_vector_t wall_vertices_girder_long_capped[] = {
  {25, 0}, {50, 25}, {-50, 25}, {-50, -25}, {50, -25}
};
static const az_vector_t wall_vertices_girder_long_double_capped[] = {
  {50, -25}, {50, 25}, {-50, 25}, {-50, -25}
};
static const az_vector_t wall_vertices_girder_short[] = {
  {2, 0}, {27, 25}, {-27, 25}, {-2, 0}, {-27, -25}, {27, -25}
};
static const az_vector_t wall_vertices_girder_skinny[] = {
  {39, 0}, {59, 20}, {-59, 20}, {-39, 0}, {-59, -20}, {59, -20}
};
static const az_vector_t wall_vertices_girder_skinny_capped[] = {
  {39, 0}, {59, 20}, {-59, 20}, {-59, -20}, {59, -20}
};
static const az_vector_t wall_vertices_11[] = {
  {17, 4}, {23, 10}, {20, 21}, {8, 28}, {-2, 22}, {-20, 18}, {-20, 0},
  {-27, -10}, {-19, -21}, {-3, -18}, {7, -26}, {16, -20}
};
static const az_vector_t wall_vertices_12[] = {
  {6, 36}, {-10, 42}, {-20, 38}, {-50, 35}, {-54, 5}, {-50, -30}, {-35, -37},
  {-25, -32}, {-5, -41}, {25, -31}, {50, -34}, {53, -3}, {50, 35}, {32, 41}
};
static const az_vector_t wall_vertices_13[] = {
  {77, 20}, {81, 60}, {55, 86}, {11, 88}, {1, 84}, {-45, 60}, {-81, 60},
  {-103, 0}, {-72, -65}, {-41, -70}, {-25, -64}, {5, -84}, {23, -84},
  {53, -66}, {85, -56}, {95, -10}
};
static const az_vector_t wall_vertices_14[] = {
  {12, 36}, {-20, 42}, {-40, 38}, {-100, 35}, {-108, 5}, {-100, -30},
  {-70, -37}, {-50, -32}, {-10, -41}, {50, -31}, {100, -34}, {106, -3},
  {100, 35}, {64, 41}
};
static const az_vector_t wall_vertices_18[] = {
  {0, 30}, {-8, 28}, {0, -30}, {8, 28}
};
static const az_vector_t wall_vertices_19[] = {
  {0, 20}, {-6, 18}, {0, -20}, {6, 18}
};
static const az_vector_t wall_vertices_21[] = {
  {125, 50}, {-125, 50}, {-125, -50}, {125, -50}
};
static const az_vector_t wall_vertices_22[] = {
  {50, 33}, {-50, 33}, {-50, -33}, {50, -33}
};
static const az_vector_t wall_vertices_23[] = {
  {25, 33}, {-25, 33}, {-25, -33}, {25, -33}
};
static const az_vector_t wall_vertices_24[] = {
  {40, 0}, {20, 34.641016151377549}, {-20, 34.641016151377549},
  {-40, 0}, {-20, -34.641016151377549}, {20, -34.641016151377549}
};
static const az_vector_t wall_vertices_25[] = {
  {80, 0}, {40, 69.282032302755098}, {-40, 69.282032302755098},
  {-80, 0}, {-40, -69.282032302755098}, {40, -69.282032302755098}
};
static const az_vector_t wall_vertices_26[] = {
  {40, 30}, {39.24, 33.83}, {37.07, 37.07}, {33.83, 39.24}, {30, 40},
  {-30, 40}, {-33.83, 39.24}, {-37.07, 37.07}, {-39.24, 33.83}, {-40, 30},
  {-40, -30}, {-39.24, -33.83}, {-37.07, -37.07}, {-33.83, -39.24}, {-30, -40},
  {30, -40}, {33.83, -39.24}, {37.07, -37.07}, {39.24, -33.83}, {40, -30}
};
static const az_vector_t wall_vertices_27[] = {
  // Rounded corner for {16.57, 40}:
  {16.57, 26.93}, {14.98, 30.76}, {11.16, 32.35}, {7.33, 30.76},
  // Rounded corner for {-40, -16.57}:
  {-30.76, -7.33}, {-31.94, -9.09}, {-32.35, -11.16},
  {-31.94, -13.23}, {-30.76, -14.99}, {-29.01, -16.16},
  // Rounded corner for {16.57, -40}:
  {9.09, -31.94}, {11.16, -32.35}, {13.23, -31.94},
  {14.99, -30.76}, {16.16, -29.01}, {16.57, -26.93}
};
static const az_vector_t wall_vertices_31[] = {
  {25, 55}, {-25, 55}, {-25, -55}, {25, -55}
};
static const az_vector_t wall_vertices_32[] = {
  {28, 28}, {-28, 28}, {-28, -28}, {28, -28}
};
static const az_vector_t wall_vertices_34[] = {
  {28, 28}, {-28, 28}, {-28, 11.6}, {11.6, -28}, {28, -28}
};
static const az_vector_t wall_vertices_37[] = {
  {0, 45}, {-20, 42}, {-10, 0}, {-7, -3}, {0, -45}, {22, 42}
};
static const az_vector_t wall_vertices_41[] = {
  {0, -80}, {24, -94}, {47, -93}, {98, -98}, {120, -50}, {110, -15},
  {118, 20}, {119, 52}, {102, 95}, {75, 90}, {30, 88}, {0, 80}, {-30, 97},
  {-80, 90}, {-105, 60}, {-103, 37}, {-120, 20}, {-110, -40}, {-115, -70},
  {-100, -90}, {-40, -95}
};
static const az_vector_t wall_vertices_42[] = {
  {100, 20}, {50, 15}, {0, 23}, {-50, 10}, {-100, 20},
  {-100, -40}, {-50, -80}, {0, -46}, {50, -50}, {100, -40}
};
static const az_vector_t wall_vertices_43[] = {
  {35, 34}, {9, 32}, {-20, 20}, {-31, -8}, {-33, -35},
  {30, -35}, {30, -33}, {31, -31}, {33, -30}, {35, -30}
};
static const az_vector_t wall_vertices_47[] = {
  {33, -30}, {33, 30}, {-32, 30}, {-32, -30}
};
static const az_vector_t wall_vertices_50[] = {
  {20, 0}, {40, 20}, {-40, 20}, {-20, 0}, {-40, -20}, {40, -20}
};
static const az_vector_t wall_vertices_51[] = {
  {47.5, 0}, {72.5, 25}, {-72.5, 25}, {-47.5, 0}, {-72.5, -25}, {72.5, -25}
};
static const az_vector_t wall_vertices_52[] = {
  {90, 5}, {42, 60}, {-30, 60}, {-87, 0}, {-50, -62}, {40, -60}
};
static const az_vector_t wall_vertices_53[] = {
  {45, 2}, {21, 30}, {-15, 33}, {-44, 0}, {-25, -36}, {20, -30}
};
static const az_vector_t wall_vertices_54[] = {
  {20, 15}, {19.62, 16.915}, {18.535, 18.535}, {16.915, 19.62}, {15, 20},
  {-15, 20}, {-16.915, 19.62}, {-18.535, 18.535}, {-19.62, 16.915},
  {-20, 15}, {-20, -15}, {-19.62, -16.915}, {-18.535, -18.535},
  {-16.915, -19.62}, {-15, -20}, {15, -20}, {16.915, -19.62},
  {18.535, -18.535}, {19.62, -16.915}, {20, -15}
};
static const az_vector_t wall_vertices_55[] = {
  {80, 15}, {79.62, 16.915}, {78.535, 18.535}, {76.915, 19.62}, {75, 20},
  {-75, 20}, {-76.915, 19.62}, {-78.535, 18.535}, {-79.62, 16.915},
  {-80, 15}, {-80, -15}, {-79.62, -16.915}, {-78.535, -18.535},
  {-76.915, -19.62}, {-75, -20}, {75, -20}, {76.915, -19.62},
  {78.535, -18.535}, {79.62, -16.915}, {80, -15}
};
static const az_vector_t wall_vertices_56[] = {
  {80, 30}, {79.24, 33.83}, {77.07, 37.07}, {73.83, 39.24}, {70, 40},
  {-70, 40}, {-73.83, 39.24}, {-77.07, 37.07}, {-79.24, 33.83}, {-80, 30},
  {-80, -30}, {-79.24, -33.83}, {-77.07, -37.07}, {-73.83, -39.24}, {-70, -40},
  {70, -40}, {73.83, -39.24}, {77.07, -37.07}, {79.24, -33.83}, {80, -30}
};
static const az_vector_t wall_vertices_61[] = {
  {50, 20}, {27, 18}, {2, 21}, {-24, 15}, {-50, 20},
  {-50, -40}, {-25, -60}, {0, -46}, {25, -50}, {50, -40}
};
static const az_vector_t wall_vertices_63[] = {
  {20, 25}, {-10.710678, 25}, {-31.8198, 3.5355339}, {3.5355339, -31.8198},
  {10, -25}, {20, -25}
};
static const az_vector_t wall_vertices_65[] = {
  {34.641016151377549, 0}, {-34.641016151377549, 0}, {0, -20}
};
static const az_vector_t wall_vertices_66[] = {
  {23.094010767585, 0}, {-11.547005383792511, 20}, {-11.547005383792511, -20}
};
static const az_vector_t wall_vertices_67[] = {
  {0, 30}, {-8, 24}, {0, -30}, {8, 24}
};
static const az_vector_t wall_vertices_68[] = {
  {8, -20}, {8, 1}, {9, 5}, {9, 20}, {-7, 20}, {-7, 3}, {-8, 2}, {-8, -20}
};
static const az_vector_t wall_vertices_69[] = {
  {0, 30}, {-12, 14}, {-8, -24}, {0, -30}, {8, -24}, {12, 14}
};
static const az_vector_t wall_vertices_70[] = {
  {16, -30}, {16, -2}, {17, 14}, {17, 30},
  {-15, 30}, {-15, 5}, {-16, 0}, {-16, -30}
};
static const az_vector_t wall_vertices_71[] = {
  {0, 60}, {-24, 28}, {-16, -48}, {0, -60}, {16, -48}, {24, 28}
};
static const az_vector_t wall_vertices_72[] = {
  {32, -60}, {31, -35}, {32, -4}, {34, 28}, {33, 44}, {34, 60},
  {-30, 60}, {-29, 27}, {-30, 10}, {-32, 0}, {-33, -40}, {-32, -60}
};
static const az_vector_t wall_vertices_73[] = {
  {50, -8}, {30, 30}, {10, 50}, {-10, 53}, {-25, 30},
  {-50, 0}, {-40, -45}, {-10, -65}, {10, -60}, {40, -35}
};
static const az_vector_t wall_vertices_74[] = {
  {100, -10}, {60, 40}, {-20, 70}, {-50, 40},
  {-100, 0}, {-80, -60}, {-20, -90}, {20, -90}, {80, -46}
};
static const az_vector_t wall_vertices_75[] = {
  {40, 6}, {67, 40}, {70, 60}, {58, 80}, {15, 90}, {-30, 56},
  {-50, 18}, {-46, -16}, {-10, -56}, {20, -52}, {50, -35}, {52, -20}
};
static const az_vector_t wall_vertices_79[] = {
  {54, 14}, {42, 18}, {-42, 18}, {-54, 14},
  {-54, -14}, {-42, -18}, {42, -18}, {54, -14}
};
static const az_vector_t wall_vertices_80[] = {
  {-100, 40}, {-92, 2}, {-80, -40}, {20, -38}, {80, -40}, {88, -6},
  {100, 40}, {5, 38}
};
static const az_vector_t wall_vertices_81[] = {
  {-140, 30}, {-164, 2}, {-180, -30}, {-20, -28}, {180, -30}, {156, -6},
  {140, 30}, {0, 28}
};
static const az_vector_t wall_vertices_82[] = {
  {-115, 20}, {-101, 2}, {-77, -20}, {-7, -18}, {113, -20}, {119, -4},
  {103, 20}, {13, 18}
};
static const az_vector_t wall_vertices_83[] = {
  {-60, 40}, {-55, 2}, {-44, -40}, {20, -38}, {58, -40}, {59, -6},
  {58, 40}, {9, 38}
};
static az_vector_t wall_vertices_88[] = {
  // for deg in xrange(0, 360, 15):
  //   (c, s) = (cos(radians(deg)), sin(radians(deg)))
  //   print('{%.04g, %.04g},' % (60*c*exp(0.2*s), 45*s*exp(0.2*c)))
  {60, 0}, {61.03, 14.13}, {57.43, 26.75}, {48.87, 36.65}, {35.67, 43.07},
  {18.84, 45.78}, {4.487e-15, 45}, {-18.84, 41.27}, {-35.67, 35.26},
  {-48.87, 27.62}, {-57.43, 18.92}, {-61.03, 9.601}, {-60, 0},
  {-55.03, -9.601}, {-47.02, -18.92}, {-36.83, -27.62}, {-25.23, -35.26},
  {-12.8, -41.27}, {0, -45}, {12.8, -45.78}, {25.23, -43.07}, {36.83, -36.65},
  {47.02, -26.75}, {55.03, -14.13}
};
static az_vector_t wall_vertices_89[] = {
  // for deg in xrange(0, 360, 10):
  //   (c, s) = (cos(radians(deg)), sin(radians(deg)))
  //   print('{%.04g, %.04g},' % (100*c*exp(0.2*s), 80*s*exp(-0.5*c)))
  {100, 0}, {102, 8.49}, {100.6, 17.1}, {95.71, 25.94}, {87.11, 35.06},
  {74.92, 44.44}, {59.46, 53.96}, {41.27, 63.36}, {21.15, 72.23}, {0, 80},
  {-21.15, 85.93}, {-41.27, 89.2}, {-59.46, 88.96}, {-74.92, 84.51},
  {-87.11, 75.42}, {-95.71, 61.68}, {-100.6, 43.77}, {-102, 22.73}, {-100, 0},
  {-95.12, -22.73}, {-87.76, -43.77}, {-78.36, -61.68}, {-67.36, -75.42},
  {-55.15, -84.51}, {-42.05, -88.96}, {-28.34, -89.2}, {-14.26, -85.93},
  {0, -80}, {14.26, -72.23}, {28.34, -63.36}, {42.05, -53.96}, {55.15, -44.44},
  {67.36, -35.06}, {78.36, -25.94}, {87.76, -17.1}, {95.12, -8.49}
};
static az_vector_t wall_vertices_99[] = {
  {30, 5}, {10, 5}, {-10, 5}, {-30, 5},
  {-30, -6}, {-10, -4}, {12, -8}, {30, -6}
};
static const az_vector_t wall_vertices_101[] = {
  {65, 25}, {-15, 25}, {-65, -25}, {15, -25}
};
static const az_vector_t wall_vertices_105[] = {
  {25, 25}, {-15, 25}, {-65, -25}, {25, -25}
};
static const az_vector_t wall_vertices_114[] = {
  {50, 30}, {35, 28}, {25, 33}, {10, 25}, {-10, 32}, {-30, 24}, {-50, 30},
  {-50, -30}, {-30, -25}, {-10, -32}, {10, -38}, {25, -30}, {35, -23},
  {50, -30}
};
static const az_vector_t wall_vertices_115[] = {
  {100, 30}, {70, 28}, {50, 33}, {20, 25}, {-20, 32}, {-60, 24}, {-100, 30},
  {-100, -30}, {-60, -25}, {-20, -32}, {20, -38}, {50, -30}, {70, -23},
  {100, -30}
};
static const az_vector_t wall_vertices_128[] = {
  {50, 1}, {38, 28}, {30, 39}, {18, 47}, {10, 54}, {-10, 55}, {-25, 39},
  {-35, 22}, {-50, 9}, {-43, -11}, {-40, -36}, {-23, -44}, {-10, -56},
  {10, -51}, {23, -36}, {40, -26}, {48, -10}
};
static const az_vector_t wall_vertices_129[] = {
  {38, 56}, {31, 78}, {19, 85}, {1, 91}, {-14, 84}, {-37, 64}, {-47, 36},
  {-68, 15}, {-58, -18}, {-54, -59}, {-31, -73}, {-14, -92}, {14, -84},
  {31, -59}, {54, -43}, {65, -17}, {68, 2}, {51, 46}
};
static const az_vector_t wall_vertices_130[] = {
  {56, 68}, {46, 94}, {28, 108}, {2, 102}, {-20, 110}, {-55, 78}, {-70, 44},
  {-100, 18}, {-86, -22}, {-80, -72}, {-46, -88}, {-20, -112}, {20, -102},
  {46, -72}, {80, -52}, {96, -20}, {100, 2}, {76, 56}
};
static const az_vector_t wall_vertices_142[] = {
  {44.35, -18.37}, {42, -15}, {42, 15}, {44.35, 18.37},
  {-44.35, 18.37}, {-42, 15}, {-42, -15}, {-44.35, -18.37}
};
static const az_vector_t wall_vertices_143[] = {
  {44.35, -18.37}, {42, -15}, {42, 15},
  {44.35, 18.37}, {40.21, 19}, {19, 40.21},
  {18.37, 44.35}, {15, 42}, {-15, 42},
  {-18.37, 44.35}, {-18.37, -18.37}
};
static const az_vector_t wall_vertices_144[] = {
  {44.35, -18.37}, {42, -15}, {42, 15},
  {44.35, 18.37}, {40.21, 19}, {19, 40.21},
  {18.37, 44.35}, {15, 42}, {-15, 42},
  {-18.37, 44.35}, {-19, 40.21}, {-40.21, 19},
  {-44.35, 18.37}, {-7.61, -18.37}
};
static const az_vector_t wall_vertices_145[] = {
  {44.35, -18.37}, {42, -15}, {42, 15},
  {44.35, 18.37}, {40.21, 19}, {19, 40.21},
  {18.37, 44.35}, {15, 42}, {-15, 42},
  {-18.37, 44.35}, {-19, 40.21}, {-40.21, 19},
  {-44.35, 18.37}, {-42, 15}, {-42, -15}, {-44.35, -18.37}
};
static const az_vector_t wall_vertices_146[] = {
  {44.35, -18.37}, {42, -15}, {42, 15},
  {44.35, 18.37}, {40.21, 19}, {19, 40.21},
  {18.37, 44.35}, {15, 42}, {-15, 42},
  {-18.37, 44.35}, {-19, 40.21}, {-40.21, 19},
  {-44.35, 18.37}, {-42, 15}, {-42, -15},
  {-44.35, -18.37}, {-40.21, -19}, {-19, -40.21},
  {-18.37, -44.35}, {-15, -42}, {15, -42},
  {18.37, -44.35}, {19, -40.21}, {40.21, -19}
};
static const az_vector_t wall_vertices_147[] = {
  {94, 14}, {82, 18}, {70, 14}, {-70, 14}, {-82, 18}, {-94, 14},
  {-94, -14}, {-82, -18}, {-70, -14}, {70, -14}, {82, -18}, {94, -14}
};
static const az_vector_t wall_vertices_148[] = {
  {26, 14}, {14, 18}, {-14, 18}, {-26, 14},
  {-26, -14}, {-14, -18}, {14, -18}, {26, -14}
};
static const az_vector_t wall_vertices_149[] = {
  {50, -50}, {50, 50}, {-50, 50}
};
static const az_vector_t wall_vertices_153[] = {
  {50, 15}, {48, 21}, {46, 23}, {44, 24.2}, {42, 24.8}, {40, 25}, {-50, 25},
  {-50, -25}, {40, -25}, {42, -24.8}, {44, -24.2}, {46, -23}, {48, -21},
  {50, -15}
};
static const az_vector_t wall_vertices_154[] = {
  {50, 15}, {-50, 15}, {-50, -15}, {50, -15}
};
static const az_vector_t wall_vertices_155[] = {
  {50, 5}, {49, 11}, {48, 13}, {47, 14.2}, {46, 14.8}, {45, 15}, {-50, 15},
  {-50, -15}, {45, -15}, {46, -14.8}, {47, -14.2}, {48, -13}, {49, -11},
  {50, -5}
};
static const az_vector_t wall_vertices_156[] = {
  {-60, 0}, {-55, -8}, {-46, -9}, {-35, 0}, {-20, 15}, {-7, 18},
  {0, 18}, {12, 12}, {28, 11}, {47, -9}, {55, -8},
  {60, 0}, {55, 9}, {50, 10}, {31, 26}, {16, 26}, {2, 34},
  {-11, 31}, {-23, 30}, {-40, 22}, {-50, 10}, {-55, 8}
};
static const az_vector_t wall_vertices_157[] = {
  {-115, 0}, {-110, -8}, {-92, -7}, {-70, 5}, {-40, 15}, {-14, 18},
  {0, 18}, {24, 12}, {56, 11}, {94, -9}, {110, -8},
  {115, 0}, {110, 9}, {100, 10}, {62, 26}, {32, 26}, {4, 34},
  {-22, 31}, {-46, 30}, {-80, 22}, {-100, 10}, {-110, 8}
};
static const az_vector_t wall_vertices_161[] = {
  {101, -168}, {157, -112}, {157, 112}, {101, 168}, {-157, 168}, {-157, -168}
};
static const az_vector_t wall_vertices_162[] = {
  {101, -168}, {157, -112}, {157, 112}, {101, 168},
  {-180, 168}, {-236, 112}, {-236, -112}, {-180, -168}
};

/*===========================================================================*/

static az_wall_data_t wall_datas[] = {
  // Yellow rectangle block:
  [0] = {
    .style = AZ_WSTY_BEZEL_12, .bezel = 18.0,
    .color1 = {255, 255, 0, 255}, .color2 = {64, 64, 0, 255},
    .elasticity = 0.4,
    .polygon = AZ_INIT_POLYGON(wall_vertices_0)
  },
  // Large blue square block:
  [1] = {
    .style = AZ_WSTY_BEZEL_12, .bezel = 18.0,
    .color1 = {0, 255, 255, 255}, .color2 = {0, 32, 64, 255},
    .elasticity = 0.4,
    .polygon = AZ_INIT_POLYGON(wall_vertices_1)
  },
  // Yellow angle block:
  [2] = {
    .style = AZ_WSTY_BEZEL_12, .bezel = 18.0,
    .color1 = {255, 255, 0, 255}, .color2 = {64, 64, 0, 255},
    .elasticity = 0.4,
    .polygon = AZ_INIT_POLYGON(wall_vertices_2)
  },
  // Small cyan crystal:
  [3] = {
    .style = AZ_WSTY_BEZEL_21, .bezel = 1000.0,
    .color2 = {0, 32, 32, 255}, .color1 = {0, 255, 255, 255},
    .elasticity = 0.3, .impact_damage_coeff = 10.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_3)
  },
  // Big cyan crystal:
  [4] = {
    .style = AZ_WSTY_BEZEL_21, .bezel = 1000.0,
    .color2 = {0, 32, 32, 255}, .color1 = {0, 255, 255, 255},
    .elasticity = 0.3, .impact_damage_coeff = 10.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_4)
  },
  // Gray/brown boulder:
  [5] = {
    .style = AZ_WSTY_BEZEL_ALT_21, .bezel = 25.0,
    .color1 = {96, 80, 68, 255}, .color2 = {48, 32, 16, 255},
    .elasticity = 0.25, .impact_damage_coeff = 4.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_5)
  },
  // Silver long girder:
  [6] = {
    .style = AZ_WSTY_GIRDER, .bezel = 7.0,
    .color1 = {192, 192, 192, 255}, .color2 = {64, 64, 64, 255},
    .elasticity = 0.5,
    .polygon = AZ_INIT_POLYGON(wall_vertices_girder_long)
  },
  // Red long girder:
  [7] = {
    .style = AZ_WSTY_GIRDER, .bezel = 6.0,
    .color1 = {128, 32, 32, 255}, .color2 = {64, 0, 0, 255},
    .elasticity = 0.5,
    .polygon = AZ_INIT_POLYGON(wall_vertices_girder_long)
  },
  // Red short girder:
  [8] = {
    .style = AZ_WSTY_GIRDER, .bezel = 6.0,
    .color1 = {128, 32, 32, 255}, .color2 = {64, 0, 0, 255},
    .elasticity = 0.5,
    .polygon = AZ_INIT_POLYGON(wall_vertices_girder_short)
  },
  // Brown long girder:
  [9] = {
    .style = AZ_WSTY_GIRDER, .bezel = 4.7,
    .color1 = {128, 96, 32, 255}, .color2 = {64, 64, 64, 255},
    .elasticity = 0.5,
    .polygon = AZ_INIT_POLYGON(wall_vertices_girder_skinny)
  },
  // Brown long capped girder:
  [10] = {
    .style = AZ_WSTY_GIRDER_CAP, .bezel = 4.7,
    .color1 = {128, 96, 32, 255}, .color2 = {64, 64, 64, 255},
    .elasticity = 0.5,
    .polygon = AZ_INIT_POLYGON(wall_vertices_girder_skinny_capped)
  },
  // Small gray/brown boulder:
  [11] = {
    .style = AZ_WSTY_TRIFAN, .bezel = 900.0,
    .color1 = {90, 80, 68, 255}, .color2 = {48, 32, 16, 255},
    .elasticity = 0.25, .impact_damage_coeff = 4.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_11)
  },
  // Gray/brown rock wall:
  [12] = {
    .style = AZ_WSTY_BEZEL_21, .bezel = 25.0,
    .color1 = {90, 80, 68, 255}, .color2 = {48, 32, 16, 255},
    .elasticity = 0.25, .impact_damage_coeff = 4.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_12)
  },
  // Big gray/brown boulder:
  [13] = {
    .style = AZ_WSTY_BEZEL_ALT_21, .bezel = 40.0,
    .color1 = {96, 84, 68, 255}, .color2 = {48, 32, 16, 255},
    .elasticity = 0.25, .impact_damage_coeff = 4.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_13)
  },
  // Long gray/brown rock wall:
  [14] = {
    .style = AZ_WSTY_BEZEL_21, .bezel = 25.0,
    .color1 = {90, 80, 68, 255}, .color2 = {48, 32, 16, 255},
    .elasticity = 0.25, .impact_damage_coeff = 4.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_14)
  },
  // Ice wall:
  [15] = {
    .style = AZ_WSTY_TRIFAN,
    .color1 = {90, 255, 224, 160}, .color2 = {16, 64, 64, 224},
    .elasticity = 0.2, .impact_damage_coeff = 5.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_12)
  },
  // Big ice boulder:
  [16] = {
    .style = AZ_WSTY_TRIFAN,
    .color1 = {90, 255, 255, 160}, .color2 = {16, 32, 48, 224},
    .elasticity = 0.2, .impact_damage_coeff = 5.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_13)
  },
  // Small ice boulder:
  [17] = {
    .style = AZ_WSTY_TRIFAN,
    .color1 = {90, 255, 255, 160}, .color2 = {16, 32, 48, 224},
    .elasticity = 0.2, .impact_damage_coeff = 5.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_11)
  },
  // Large icicle:
  [18] = {
    .style = AZ_WSTY_BEZEL_ALT_21, .bezel = 1000.0,
    .color1 = {90, 255, 255, 120}, .color2 = {16, 32, 48, 168},
    .elasticity = 0.2, .impact_damage_coeff = 6.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_18)
  },
  // Small icicle:
  [19] = {
    .style = AZ_WSTY_BEZEL_ALT_21, .bezel = 1000.0,
    .color1 = {90, 255, 255, 120}, .color2 = {16, 32, 48, 168},
    .elasticity = 0.2, .impact_damage_coeff = 6.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_19)
  },
  // Silver long capped girder:
  [20] = {
    .style = AZ_WSTY_GIRDER_CAP, .bezel = 7.0,
    .color1 = {192, 192, 192, 255}, .color2 = {64, 64, 64, 255},
    .elasticity = 0.5,
    .polygon = AZ_INIT_POLYGON(wall_vertices_girder_long_capped)
  },
  // Huge blue rectangular block:
  [21] = {
    .style = AZ_WSTY_BEZEL_12, .bezel = 18.0,
    .color1 = {0, 255, 255, 255}, .color2 = {0, 32, 64, 255},
    .elasticity = 0.4, .polygon = AZ_INIT_POLYGON(wall_vertices_21)
  },
  // Red/gray rectangle cinderbrick:
  [22] = {
    .style = AZ_WSTY_BEZEL_12, .bezel = 15.0,
    .color1 = {192, 24, 24, 255}, .color2 = {70, 54, 54, 255},
    .elasticity = 0.25, .polygon = AZ_INIT_POLYGON(wall_vertices_22)
  },
  // Red/gray rectangle half-size cinderbrick:
  [23] = {
    .style = AZ_WSTY_BEZEL_12, .bezel = 15.0,
    .color1 = {192, 24, 24, 255}, .color2 = {70, 54, 54, 255},
    .elasticity = 0.25, .polygon = AZ_INIT_POLYGON(wall_vertices_23)
  },
  // Green/purple hexagonal block:
  [24] = {
    .style = AZ_WSTY_BEZEL_12, .bezel = 20.0,
    .color1 = {64, 160, 8, 255}, .color2 = {90, 20, 60, 255},
    .elasticity = 0.4, .polygon = AZ_INIT_POLYGON(wall_vertices_24)
  },
  // Large green/purple hexagonal block:
  [25] = {
    .style = AZ_WSTY_BEZEL_12, .bezel = 20.0,
    .color1 = {64, 160, 8, 255}, .color2 = {60, 20, 90, 255},
    .elasticity = 0.4, .polygon = AZ_INIT_POLYGON(wall_vertices_25)
  },
  // Blue/gray square block with rounded corners:
  [26] = {
    .style = AZ_WSTY_BEZEL_21, .bezel = 10.0,
    .color1 = {64, 64, 80, 255}, .color2 = {32, 32, 32, 255},
    .elasticity = 0.25, .polygon = AZ_INIT_POLYGON(wall_vertices_26)
  },
  // Blue/gray triangular block with rounded corners:
  [27] = {
    .style = AZ_WSTY_BEZEL_21, .bezel = 5.4134,
    .color1 = {64, 64, 72, 255}, .color2 = {32, 32, 32, 255},
    .elasticity = 0.25, .polygon = AZ_INIT_POLYGON(wall_vertices_27)
  },
  // Green metal rectangle:
  [28] = {
    .style = AZ_WSTY_METAL,
    .color1 = {96, 128, 64, 255}, .color2 = {24, 32, 16, 255},
    .elasticity = 0.6, .polygon = AZ_INIT_POLYGON(wall_vertices_0)
  },
  // Green metal rectangle reverse:
  [29] = {
    .style = AZ_WSTY_METAL_ALT,
    .color1 = {96, 128, 64, 255}, .color2 = {24, 32, 16, 255},
    .elasticity = 0.6, .polygon = AZ_INIT_POLYGON(wall_vertices_0)
  },
  // Green metal angle:
  [30] = {
    .style = AZ_WSTY_METAL,
    .color1 = {96, 128, 64, 255}, .color2 = {24, 32, 16, 255},
    .elasticity = 0.6, .polygon = AZ_INIT_POLYGON(wall_vertices_2)
  },
  // Red/gray long rectangle cinderbrick:
  [31] = {
    .style = AZ_WSTY_BEZEL_12, .bezel = 15.0,
    .color1 = {192, 24, 24, 255}, .color2 = {70, 54, 54, 255},
    .elasticity = 0.25, .polygon = AZ_INIT_POLYGON(wall_vertices_31)
  },
  // Yellow square block:
  [32] = {
    .style = AZ_WSTY_TRIFAN,
    .color1 = {255, 255, 64, 255}, .color2 = {64, 64, 16, 255},
    .elasticity = 0.5, .polygon = AZ_INIT_POLYGON(wall_vertices_32)
  },
  // Yellowish-gray pipe:
  [33] = {
    .style = AZ_WSTY_QUADSTRIP_213, .color1 = {192, 192, 96, 255},
    .color2 = {48, 48, 32, 255}, .color3 = {48, 48, 32, 255},
    .elasticity = 0.5, .polygon = AZ_INIT_POLYGON(wall_vertices_0)
  },
  // Yellow triangle block:
  [34] = {
    .style = AZ_WSTY_TRIFAN,
    .color1 = {255, 255, 64, 255}, .color2 = {64, 64, 16, 255},
    .elasticity = 0.5, .polygon = AZ_INIT_POLYGON(wall_vertices_34)
  },
  // Silver short girder:
  [35] = {
    .style = AZ_WSTY_GIRDER, .bezel = 7.0,
    .color1 = {192, 192, 192, 255}, .color2 = {64, 64, 64, 255},
    .elasticity = 0.5,
    .polygon = AZ_INIT_POLYGON(wall_vertices_girder_short)
  },
  // Small brown stalactite:
  [36] = {
    .style = AZ_WSTY_BEZEL_ALT_21, .bezel = 1000.0,
    .color1 = {96, 80, 68, 255}, .color2 = {48, 32, 16, 255},
    .elasticity = 0.25, .impact_damage_coeff = 6.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_18)
  },
  // Large brown stalactite:
  [37] = {
    .style = AZ_WSTY_BEZEL_ALT_21, .bezel = 20.0,
    .color1 = {96, 80, 68, 255}, .color2 = {48, 32, 16, 255},
    .elasticity = 0.25, .impact_damage_coeff = 6.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_37)
  },
  // Red/gray square block:
  [38] = {
    .style = AZ_WSTY_TRIFAN,
    .color1 = {160, 160, 160, 255}, .color2 = {32, 0, 0, 255},
    .elasticity = 0.5, .polygon = AZ_INIT_POLYGON(wall_vertices_32)
  },
  // Reddish-gray pipe:
  [39] = {
    .style = AZ_WSTY_QUADSTRIP_213, .color1 = {128, 112, 112, 255},
    .color2 = {32, 24, 24, 255}, .color3 = {32, 24, 24, 255},
    .elasticity = 0.5, .polygon = AZ_INIT_POLYGON(wall_vertices_0)
  },
  // Red/gray triangle block:
  [40] = {
    .style = AZ_WSTY_TRIFAN,
    .color1 = {160, 160, 160, 255}, .color2 = {32, 0, 0, 255},
    .elasticity = 0.5, .polygon = AZ_INIT_POLYGON(wall_vertices_34)
  },
  // Huge gray/brown boulder:
  [41] = {
    .style = AZ_WSTY_BEZEL_ALT_21, .bezel = 70.0,
    .color1 = {96, 88, 80, 255}, .color2 = {48, 32, 16, 255},
    .elasticity = 0.25, .impact_damage_coeff = 4.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_41)
  },
  // Grass/dirt straight-ish wall:
  [42] = {
    .style = AZ_WSTY_QUADSTRIP_213, .bezel = 0.5,
    .color1 = {96, 48, 0, 255}, .color2 = {64, 128, 0, 255},
    .color3 = {24, 12, 0, 255},
    .elasticity = 0.15, .impact_damage_coeff = 2.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_42)
  },
  // Grass/dirt convex wall:
  [43] = {
    .style = AZ_WSTY_QUADSTRIP_213, .bezel = 0.5,
    .color1 = {96, 48, 0, 255}, .color2 = {64, 128, 0, 255},
    .color3 = {24, 12, 0, 255},
    .elasticity = 0.15, .impact_damage_coeff = 2.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_43)
  },
  // Grass/dirt concave wall:
  [44] = {
    .style = AZ_WSTY_QUADSTRIP_213, .bezel = -0.5,
    .color1 = {96, 48, 0, 255}, .color2 = {24, 12, 0, 255},
    .color3 = {64, 128, 0, 255},
    .elasticity = 0.15, .impact_damage_coeff = 2.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_43)
  },
  // Dust/dirt straight-ish wall:
  [45] = {
    .style = AZ_WSTY_QUADSTRIP_213, .bezel = 0.5,
    .color1 = {96, 48, 0, 255}, .color2 = {128, 64, 32, 255},
    .color3 = {24, 12, 0, 255},
    .elasticity = 0.15, .impact_damage_coeff = 2.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_42)
  },
  // Silver long double-capped girder:
  [46] = {
    .style = AZ_WSTY_GIRDER_CAPS, .bezel = 5.0,
    .color1 = {192, 192, 192, 255}, .color2 = {64, 64, 64, 255},
    .elasticity = 0.5,
    .polygon = AZ_INIT_POLYGON(wall_vertices_girder_long_double_capped)
  },
  // Blue-gray girder box:
  [47] = {
    .style = AZ_WSTY_GIRDER_CAPS, .bezel = 7.0,
    .color1 = {192, 192, 255, 255}, .color2 = {64, 64, 96, 255},
    .elasticity = 0.5, .polygon = AZ_INIT_POLYGON(wall_vertices_47)
  },
  // Yellowish-gray angle pipe:
  [48] = {
    .style = AZ_WSTY_QUADSTRIP_213,
    .color1 = {192, 192, 96, 255}, .color2 = {48, 48, 32, 255},
    .color3 = {48, 48, 32, 255},
    .elasticity = 0.4, .polygon = AZ_INIT_POLYGON(wall_vertices_2)
  },
  // Reddish-gray angle pipe:
  [49] = {
    .style = AZ_WSTY_QUADSTRIP_213,
    .color1 = {128, 112, 112, 255}, .color2 = {32, 24, 24, 255},
    .color3 = {32, 24, 24, 255},
    .elasticity = 0.4, .polygon = AZ_INIT_POLYGON(wall_vertices_2)
  },
  // Brown medium girder:
  [50] = {
    .style = AZ_WSTY_GIRDER, .bezel = 4.7,
    .color1 = {128, 96, 32, 255}, .color2 = {64, 64, 64, 255},
    .elasticity = 0.5, .polygon = AZ_INIT_POLYGON(wall_vertices_50)
  },
  // Silver very long girder:
  [51] = {
    .style = AZ_WSTY_GIRDER, .bezel = 7.0,
    .color1 = {192, 192, 192, 255}, .color2 = {64, 64, 64, 255},
    .elasticity = 0.5, .polygon = AZ_INIT_POLYGON(wall_vertices_51)
  },
  // Large glowing hex crystal:
  [52] = {
    .style = AZ_WSTY_CELL_QUAD, .color1 = {128, 96, 80, 255},
    .color2 = {48, 16, 32, 128}, .color3 = {84, 68, 96, 255},
    .underglow = {50, 50, 50, 255}, .impact_damage_coeff = 7.0,
    .elasticity = 0.3, .polygon = AZ_INIT_POLYGON(wall_vertices_52)
  },
  // Medium glowing hex crystal:
  [53] = {
    .style = AZ_WSTY_CELL_QUAD, .color1 = {128, 96, 80, 255},
    .color2 = {48, 16, 32, 128}, .color3 = {84, 68, 96, 255},
    .underglow = {50, 50, 50, 255}, .impact_damage_coeff = 7.0,
    .elasticity = 0.3, .polygon = AZ_INIT_POLYGON(wall_vertices_53)
  },
  // Small blue/gray square block with rounded corners:
  [54] = {
    .style = AZ_WSTY_BEZEL_21, .bezel = 10.0,
    .color1 = {64, 64, 80, 255}, .color2 = {32, 32, 32, 255},
    .elasticity = 0.25, .polygon = AZ_INIT_POLYGON(wall_vertices_54)
  },
  // Long blue/gray rectangular block with rounded corners:
  [55] = {
    .style = AZ_WSTY_BEZEL_21, .bezel = 10.0,
    .color1 = {64, 64, 72, 255}, .color2 = {32, 32, 32, 255},
    .elasticity = 0.25, .polygon = AZ_INIT_POLYGON(wall_vertices_55)
  },
  // Big long blue/gray rectangular block with rounded corners:
  [56] = {
    .style = AZ_WSTY_BEZEL_21, .bezel = 10.0,
    .color1 = {64, 64, 80, 255}, .color2 = {32, 32, 32, 255},
    .elasticity = 0.25, .polygon = AZ_INIT_POLYGON(wall_vertices_56)
  },
  // Long ice wall:
  [57] = {
    .style = AZ_WSTY_TRIFAN,
    .color1 = {90, 255, 224, 160}, .color2 = {16, 64, 64, 224},
    .elasticity = 0.2, .impact_damage_coeff = 5.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_14)
  },
  // Ice boulder:
  [58] = {
    .style = AZ_WSTY_TRIFAN,
    .color1 = {90, 255, 255, 160}, .color2 = {16, 32, 48, 224},
    .elasticity = 0.2, .impact_damage_coeff = 5.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_5)
  },
  // Dust/dirt convex wall:
  [59] = {
    .style = AZ_WSTY_QUADSTRIP_213, .bezel = 0.5,
    .color1 = {96, 48, 0, 255}, .color2 = {128, 64, 32, 255},
    .color3 = {24, 12, 0, 255},
    .elasticity = 0.15, .impact_damage_coeff = 2.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_43)
  },
  // Dust/dirt concave wall:
  [60] = {
    .style = AZ_WSTY_QUADSTRIP_213, .bezel = -0.5,
    .color1 = {96, 48, 0, 255}, .color2 = {24, 12, 0, 255},
    .color3 = {128, 64, 32, 255},
    .elasticity = 0.15, .impact_damage_coeff = 2.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_43)
  },
  // Dust/dirt straight-ish short wall:
  [61] = {
    .style = AZ_WSTY_QUADSTRIP_213, .bezel = 0.5,
    .color1 = {96, 48, 0, 255}, .color2 = {128, 64, 32, 255},
    .color3 = {24, 12, 0, 255},
    .elasticity = 0.15, .impact_damage_coeff = 2.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_61)
  },
  // Grass/dirt straight-ish short wall:
  [62] = {
    .style = AZ_WSTY_QUADSTRIP_213, .bezel = 0.5,
    .color1 = {96, 48, 0, 255}, .color2 = {64, 128, 0, 255},
    .color3 = {24, 12, 0, 255},
    .elasticity = 0.15, .impact_damage_coeff = 2.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_61)
  },
  // Yellowish-gray short angle pipe:
  [63] = {
    .style = AZ_WSTY_QUADSTRIP_213,
    .color1 = {192, 192, 96, 255}, .color2 = {48, 48, 32, 255},
    .color3 = {48, 48, 32, 255},
    .elasticity = 0.4, .polygon = AZ_INIT_POLYGON(wall_vertices_63)
  },
  // Reddish-gray short angle pipe:
  [64] = {
    .style = AZ_WSTY_QUADSTRIP_213,
    .color1 = {128, 112, 112, 255}, .color2 = {32, 24, 24, 255},
    .color3 = {32, 24, 24, 255},
    .elasticity = 0.4, .polygon = AZ_INIT_POLYGON(wall_vertices_63)
  },
  // Green/purple wedge:
  [65] = {
    .style = AZ_WSTY_BEZEL_12, .bezel = 6.0,
    .color1 = {64, 160, 8, 255}, .color2 = {90, 20, 60, 255},
    .elasticity = 0.4, .polygon = AZ_INIT_POLYGON(wall_vertices_65)
  },
  // Green/purple triangle:
  [66] = {
    .style = AZ_WSTY_BEZEL_12, .bezel = 10.0,
    .color1 = {64, 160, 8, 255}, .color2 = {60, 20, 90, 255},
    .elasticity = 0.4, .polygon = AZ_INIT_POLYGON(wall_vertices_66)
  },
  // Brown tree twig:
  [67] = {
    .style = AZ_WSTY_TRIFAN_ALT,
    .color1 = {80, 70, 70, 255}, .color2 = {40, 32, 24, 255},
    .elasticity = 0.25, .impact_damage_coeff = 3.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_67)
  },
  // Brown tree branch:
  [68] = {
    .style = AZ_WSTY_QUADSTRIP_213, .color1 = {80, 70, 70, 255},
    .color2 = {40, 32, 24, 255}, .color3 = {40, 32, 24, 255},
    .elasticity = 0.25, .impact_damage_coeff = 3.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_68)
  },
  // Brown tree branch base:
  [69] = {
    .style = AZ_WSTY_QUADSTRIP_ALT_213, .color1 = {80, 70, 70, 255},
    .color2 = {40, 32, 24, 255}, .color3 = {40, 32, 24, 255},
    .elasticity = 0.25, .impact_damage_coeff = 3.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_69)
  },
  // Brown tree bough:
  [70] = {
    .style = AZ_WSTY_QUADSTRIP_213, .color1 = {80, 70, 70, 255},
    .color2 = {40, 32, 24, 255}, .color3 = {40, 32, 24, 255},
    .elasticity = 0.25, .impact_damage_coeff = 3.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_70)
  },
  // Brown tree bough base:
  [71] = {
    .style = AZ_WSTY_QUADSTRIP_ALT_213, .color1 = {80, 70, 70, 255},
    .color2 = {40, 32, 24, 255}, .color3 = {40, 32, 24, 255},
    .elasticity = 0.25, .impact_damage_coeff = 3.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_71)
  },
  // Brown tree trunk:
  [72] = {
    .style = AZ_WSTY_QUADSTRIP_213, .color1 = {80, 70, 70, 255},
    .color2 = {40, 32, 24, 255}, .color3 = {40, 32, 24, 255},
    .elasticity = 0.25, .impact_damage_coeff = 3.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_72)
  },
  // Yellow/blue seashell:
  [73] = {
    .style = AZ_WSTY_CELL_TRI, .color1 = {128, 128, 32, 255},
    .color2 = {64, 64, 0, 255}, .color3 = {0, 64, 64, 255},
    .elasticity = 0.3, .impact_damage_coeff = 6.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_73)
  },
  // Yellow/purple seashell:
  [74] = {
    .style = AZ_WSTY_CELL_TRI, .color1 = {128, 128, 32, 255},
    .color2 = {96, 64, 0, 255}, .color3 = {64, 0, 96, 255},
    .elasticity = 0.3, .impact_damage_coeff = 6.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_74)
  },
  // Yellow/green seashell:
  [75] = {
    .style = AZ_WSTY_CELL_TRI, .color1 = {160, 128, 32, 255},
    .color2 = {96, 64, 32, 255}, .color3 = {0, 96, 0, 255},
    .elasticity = 0.3, .impact_damage_coeff = 6.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_75)
  },
  // Yellow/green seashell:
  [76] = {
    .style = AZ_WSTY_CELL_QUAD, .color1 = {128, 128, 32, 255},
    .color2 = {64, 64, 0, 255}, .color3 = {0, 64, 0, 255},
    .elasticity = 0.3, .impact_damage_coeff = 6.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_73)
  },
  // Yellow/purple seashell:
  [77] = {
    .style = AZ_WSTY_CELL_QUAD, .color1 = {128, 160, 32, 255},
    .color2 = {96, 64, 32, 255}, .color3 = {64, 0, 96, 255},
    .elasticity = 0.3, .impact_damage_coeff = 6.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_75)
  },
  // Yellow/blue seashell:
  [78] = {
    .style = AZ_WSTY_CELL_TRI, .color1 = {128, 128, 32, 255},
    .color2 = {64, 96, 0, 255}, .color3 = {0, 64, 96, 255},
    .elasticity = 0.3, .impact_damage_coeff = 6.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_74)
  },
  // Silver bezel pipe:
  [79] = {
    .style = AZ_WSTY_QUADSTRIP_213, .color1 = {179, 179, 179, 255},
    .color2 = {64, 64, 64, 255}, .color3 = {64, 64, 64, 255},
    .elasticity = 0.5, .polygon = AZ_INIT_POLYGON(wall_vertices_79)
  },
  // Blue shale rock:
  [80] = {
    .style = AZ_WSTY_CELL_QUAD, .bezel = 5.0,
    .color1 = {80, 80, 82, 255}, .color2 = {38, 40, 42, 255},
    .color3 = {70, 70, 72, 255},
    .elasticity = 0.25, .impact_damage_coeff = 4.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_80)
  },
  [81] = {
    .style = AZ_WSTY_CELL_QUAD, .bezel = 5.0,
    .color1 = {80, 80, 82, 255}, .color2 = {38, 40, 42, 255},
    .color3 = {70, 70, 72, 255},
    .elasticity = 0.25, .impact_damage_coeff = 4.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_81)
  },
  [82] = {
    .style = AZ_WSTY_CELL_QUAD, .bezel = 5.0,
    .color1 = {80, 80, 82, 255}, .color2 = {38, 40, 43, 255},
    .color3 = {70, 70, 72, 255},
    .elasticity = 0.25, .impact_damage_coeff = 4.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_82)
  },
  [83] = {
    .style = AZ_WSTY_CELL_QUAD, .bezel = 5.0,
    .color1 = {80, 80, 82, 255}, .color2 = {38, 40, 42, 255},
    .color3 = {70, 70, 72, 255},
    .elasticity = 0.25, .impact_damage_coeff = 4.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_83)
  },
  // Brown shale rock:
  [84] = {
    .style = AZ_WSTY_CELL_QUAD, .bezel = 5.0,
    .color1 = {81, 80, 80, 255}, .color2 = {42, 40, 38, 255},
    .color3 = {71, 70, 70, 255},
    .elasticity = 0.25, .impact_damage_coeff = 4.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_80)
  },
  [85] = {
    .style = AZ_WSTY_CELL_QUAD, .bezel = 5.0,
    .color1 = {81, 80, 80, 255}, .color2 = {42, 40, 38, 255},
    .color3 = {71, 70, 70, 255},
    .elasticity = 0.25, .impact_damage_coeff = 4.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_81)
  },
  [86] = {
    .style = AZ_WSTY_CELL_QUAD, .bezel = 5.0,
    .color1 = {81, 80, 80, 255}, .color2 = {42, 40, 38, 255},
    .color3 = {71, 70, 70, 255},
    .elasticity = 0.25, .impact_damage_coeff = 4.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_82)
  },
  [87] = {
    .style = AZ_WSTY_CELL_QUAD, .bezel = 5.0,
    .color1 = {81, 80, 80, 255}, .color2 = {42, 40, 38, 255},
    .color3 = {71, 70, 70, 255},
    .elasticity = 0.25, .impact_damage_coeff = 4.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_83)
  },
  // Water-smoothed boulder:
  [88] = {
    .style = AZ_WSTY_TFQS_123, .bezel = 0.5, .color1 = {90, 85, 100, 255},
    .color2 = {56, 49, 57, 255}, .color3 = {36, 30, 37, 255},
    .elasticity = 0.25, .impact_damage_coeff = 2.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_88)
  },
  // Large water-smoothed boulder:
  [89] = {
    .style = AZ_WSTY_TFQS_123, .bezel = 0.4, .color1 = {85, 80, 90, 255},
    .color2 = {57, 52, 59, 255}, .color3 = {31, 27, 32, 255},
    .elasticity = 0.25, .impact_damage_coeff = 2.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_89)
  },
  // Sand straight-ish long wall:
  [90] = {
    .style = AZ_WSTY_QUADSTRIP_213, .bezel = 0.0,
    .color1 = {175, 150, 132, 255}, .color2 = {117, 100, 90, 255},
    .color3 = {237, 201, 175, 255},
    .elasticity = 0.05, .impact_damage_coeff = 1.5,
    .polygon = AZ_INIT_POLYGON(wall_vertices_42)
  },
  // Sand straight-ish short wall:
  [91] = {
    .style = AZ_WSTY_QUADSTRIP_213, .bezel = 0.0,
    .color1 = {175, 150, 132, 255}, .color2 = {117, 100, 90, 255},
    .color3 = {237, 201, 175, 255},
    .elasticity = 0.05, .impact_damage_coeff = 1.5,
    .polygon = AZ_INIT_POLYGON(wall_vertices_61)
  },
  // Solid-colored sand rectangle:
  [92] = {
    .style = AZ_WSTY_TRIFAN_ALT,
    .color1 = {237, 201, 175, 255}, .color2 = {237, 201, 175, 255},
    .elasticity = 0.05, .impact_damage_coeff = 1.5,
    .polygon = AZ_INIT_POLYGON(wall_vertices_21)
  },
  // Snow/ice straight-ish wall:
  [93] = {
    .style = AZ_WSTY_QUADSTRIP_213, .bezel = 0.6,
    .color1 = {96, 32, 192, 255}, .color2 = {192, 176, 192, 255},
    .color3 = {12, 4, 24, 255},
    .elasticity = 0.15, .impact_damage_coeff = 2.5,
    .polygon = AZ_INIT_POLYGON(wall_vertices_42)
  },
  // Snow/ice straight-ish short wall:
  [94] = {
    .style = AZ_WSTY_QUADSTRIP_213, .bezel = 0.6,
    .color1 = {96, 32, 192, 255}, .color2 = {192, 176, 192, 255},
    .color3 = {12, 4, 24, 255},
    .elasticity = 0.15, .impact_damage_coeff = 2.5,
    .polygon = AZ_INIT_POLYGON(wall_vertices_61)
  },
  // Snow/ice convex wall:
  [95] = {
    .style = AZ_WSTY_QUADSTRIP_213, .bezel = 0.6,
    .color1 = {96, 32, 192, 255}, .color2 = {192, 176, 192, 255},
    .color3 = {12, 4, 24, 255},
    .elasticity = 0.15, .impact_damage_coeff = 2.5,
    .polygon = AZ_INIT_POLYGON(wall_vertices_43)
  },
  // Snow/ice concave wall:
  [96] = {
    .style = AZ_WSTY_QUADSTRIP_213, .bezel = -0.6,
    .color1 = {96, 32, 192, 255}, .color3 = {192, 176, 192, 255},
    .color2 = {12, 4, 24, 255},
    .elasticity = 0.15, .impact_damage_coeff = 2.5,
    .polygon = AZ_INIT_POLYGON(wall_vertices_43)
  },
  // Small green/gray boulder:
  [97] = {
    .style = AZ_WSTY_BEZEL_ALT_21, .bezel = 30.0,
    .color1 = {68, 96, 84, 255}, .color2 = {16, 48, 32, 255},
    .elasticity = 0.25, .impact_damage_coeff = 4.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_73)
  },
  // Large green/gray boulder:
  [98] = {
    .style = AZ_WSTY_BEZEL_ALT_21, .bezel = 50.0,
    .color1 = {68, 96, 84, 255}, .color2 = {16, 48, 32, 255},
    .elasticity = 0.25, .impact_damage_coeff = 4.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_74)
  },
  // Frozen layer of water:
  [99] = {
    .style = AZ_WSTY_QUADSTRIP_123, .color1 = {160, 255, 224, 160},
    .color2 = {224, 224, 255, 80}, .color3 = {225, 225, 225, 0},
    .elasticity = 0.2, .impact_damage_coeff = 5.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_99)
  },
  // Reddish heavy girder:
  [100] = {
    .style = AZ_WSTY_HEAVY_GIRDER, .bezel = 7.0,
    .color1 = {128, 128, 128, 255}, .color2 = {80, 48, 48, 255},
    .elasticity = 0.5,
    .polygon = AZ_INIT_POLYGON(wall_vertices_girder_long)
  },
  // Blue-gray square block:
  [101] = {
    .style = AZ_WSTY_TFQS_213, .bezel = 1/3.0,
    .color1 = {160, 160, 192, 255}, .color2 = {64, 64, 80, 255},
    .color3 = {32, 32, 48, 255},
    .elasticity = 0.5,
    .polygon = AZ_INIT_POLYGON(wall_vertices_32)
  },
  // Blue-gray triangle block:
  [102] = {
    .style = AZ_WSTY_TFQS_213, .bezel = 1/3.0,
    .color1 = {160, 160, 192, 255}, .color2 = {64, 64, 80, 255},
    .color3 = {32, 32, 48, 255},
    .elasticity = 0.5,
    .polygon = AZ_INIT_POLYGON(wall_vertices_34)
  },
  // Orange parallelagram block:
  [103] = {
    .style = AZ_WSTY_BEZEL_21, .bezel = 8.0,
    .color1 = {128, 48, 0, 255}, .color2 = {48, 36, 0, 255},
    .elasticity = 0.4,
    .polygon = AZ_INIT_POLYGON(wall_vertices_101)
  },
  // Black parallelagram block:
  [104] = {
    .style = AZ_WSTY_BEZEL_21, .bezel = 8.0,
    .color1 = {80, 80, 80, 255}, .color2 = {48, 48, 48, 255},
    .elasticity = 0.4,
    .polygon = AZ_INIT_POLYGON(wall_vertices_101)
  },
  // Yellow parallelagram block:
  [105] = {
    .style = AZ_WSTY_BEZEL_21, .bezel = 8.0,
    .color1 = {128, 128, 0, 255}, .color2 = {48, 48, 0, 255},
    .elasticity = 0.4,
    .polygon = AZ_INIT_POLYGON(wall_vertices_101)
  },
  // Orange trapezoid block:
  [106] = {
    .style = AZ_WSTY_BEZEL_21, .bezel = 8.0,
    .color1 = {128, 48, 0, 255}, .color2 = {48, 36, 0, 255},
    .elasticity = 0.4,
    .polygon = AZ_INIT_POLYGON(wall_vertices_105)
  },
  // Black trapezoid block:
  [107] = {
    .style = AZ_WSTY_BEZEL_21, .bezel = 8.0,
    .color1 = {80, 80, 80, 255}, .color2 = {48, 48, 48, 255},
    .elasticity = 0.4,
    .polygon = AZ_INIT_POLYGON(wall_vertices_105)
  },
  // Yellow trapezoid block:
  [108] = {
    .style = AZ_WSTY_BEZEL_21, .bezel = 8.0,
    .color1 = {128, 128, 0, 255}, .color2 = {48, 48, 0, 255},
    .elasticity = 0.4,
    .polygon = AZ_INIT_POLYGON(wall_vertices_105)
  },
  // Small blue/gray boulder:
  [109] = {
    .style = AZ_WSTY_BEZEL_ALT_21, .bezel = 30.0,
    .color1 = {84, 68, 96, 255}, .color2 = {32, 16, 48, 255},
    .elasticity = 0.25, .impact_damage_coeff = 4.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_73)
  },
  // Large blue/gray boulder:
  [110] = {
    .style = AZ_WSTY_BEZEL_ALT_21, .bezel = 50.0,
    .color1 = {84, 68, 96, 255}, .color2 = {32, 16, 48, 255},
    .elasticity = 0.25, .impact_damage_coeff = 4.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_74)
  },
  // Small blue/gray stalactite:
  [111] = {
    .style = AZ_WSTY_BEZEL_ALT_21, .bezel = 1000.0,
    .color1 = {84, 68, 96, 255}, .color2 = {32, 16, 48, 255},
    .elasticity = 0.25, .impact_damage_coeff = 6.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_18)
  },
  // Large blue/gray stalactite:
  [112] = {
    .style = AZ_WSTY_BEZEL_ALT_21, .bezel = 20.0,
    .color1 = {84, 68, 96, 255}, .color2 = {32, 16, 48, 255},
    .elasticity = 0.25, .impact_damage_coeff = 6.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_37)
  },
  // Solid-colored dark green rectangle:
  [113] = {
    .style = AZ_WSTY_TRIFAN_ALT,
    .color1 = {0, 64, 24, 255}, .color2 = {0, 64, 24, 255},
    .elasticity = 0.1, .impact_damage_coeff = 2.5,
    .polygon = AZ_INIT_POLYGON(wall_vertices_21)
  },
  // Short jungle grass wall:
  [114] = {
    .style = AZ_WSTY_QUADSTRIP_213, .bezel = 0.75,
    .color2 = {40, 64, 16, 255}, .color1 = {64, 96, 16, 255},
    .color3 = {0, 64, 24, 255},
    .elasticity = 0.1, .impact_damage_coeff = 2.5,
    .polygon = AZ_INIT_POLYGON(wall_vertices_114)
  },
  // Long jungle grass wall:
  [115] = {
    .style = AZ_WSTY_QUADSTRIP_213, .bezel = 0.75,
    .color2 = {40, 64, 16, 255}, .color1 = {64, 96, 16, 255},
    .color3 = {0, 64, 24, 255},
    .elasticity = 0.1, .impact_damage_coeff = 2.5,
    .polygon = AZ_INIT_POLYGON(wall_vertices_115)
  },
  // Jungle grass convex wall:
  [116] = {
    .style = AZ_WSTY_QUADSTRIP_213, .bezel = 0.75,
    .color2 = {40, 64, 16, 255}, .color1 = {64, 96, 16, 255},
    .color3 = {0, 64, 24, 255},
    .elasticity = 0.1, .impact_damage_coeff = 2.5,
    .polygon = AZ_INIT_POLYGON(wall_vertices_43)
  },
  // Jungle grass concave wall:
  [117] = {
    .style = AZ_WSTY_QUADSTRIP_213, .bezel = -0.75,
    .color3 = {40, 64, 16, 255}, .color1 = {64, 96, 16, 255},
    .color2 = {0, 64, 24, 255},
    .elasticity = 0.1, .impact_damage_coeff = 2.5,
    .polygon = AZ_INIT_POLYGON(wall_vertices_43)
  },
  // Solid-colored dark blue rectangle:
  [118] = {
    .style = AZ_WSTY_TRIFAN_ALT,
    .color1 = {80, 24, 144, 255}, .color2 = {80, 24, 144, 255},
    .elasticity = 0.25, .impact_damage_coeff = 2.5,
    .polygon = AZ_INIT_POLYGON(wall_vertices_21)
  },
  // Dark blue straight-ish long wall:
  [119] = {
    .style = AZ_WSTY_QUADSTRIP_213, .bezel = 0.6,
    .color1 = {64, 16, 128, 255}, .color2 = {80, 24, 144, 255},
    .color3 = {12, 4, 24, 255},
    .elasticity = 0.25, .impact_damage_coeff = 2.5,
    .polygon = AZ_INIT_POLYGON(wall_vertices_42)
  },
  // Dark blue straight-ish short wall:
  [120] = {
    .style = AZ_WSTY_QUADSTRIP_213, .bezel = 0.6,
    .color1 = {64, 16, 128, 255}, .color2 = {80, 24, 144, 255},
    .color3 = {12, 4, 24, 255},
    .elasticity = 0.25, .impact_damage_coeff = 2.5,
    .polygon = AZ_INIT_POLYGON(wall_vertices_61)
  },
  // Dark blue curved wall:
  [121] = {
    .style = AZ_WSTY_QUADSTRIP_213, .bezel = 0.6,
    .color1 = {64, 16, 128, 255}, .color2 = {80, 24, 144, 255},
    .color3 = {12, 4, 24, 255},
    .elasticity = 0.25, .impact_damage_coeff = 2.5,
    .polygon = AZ_INIT_POLYGON(wall_vertices_43)
  },
  // Dark blue curved wall:
  [122] = {
    .style = AZ_WSTY_QUADSTRIP_213, .bezel = -0.6,
    .color1 = {64, 16, 128, 255}, .color3 = {80, 24, 144, 255},
    .color2 = {12, 4, 24, 255},
    .elasticity = 0.25, .impact_damage_coeff = 2.5,
    .polygon = AZ_INIT_POLYGON(wall_vertices_43)
  },
  // Small red-glowing ash boulder:
  [123] = {
    .style = AZ_WSTY_TRIFAN, .bezel = 30.0,
    .color1 = {100, 94, 90, 128}, .color2 = {50, 30, 40, 128},
    .underglow = {32, 0, 0, 255}, .impact_damage_coeff = 10.0,
    .elasticity = 0.2, .polygon = AZ_INIT_POLYGON(wall_vertices_128)
  },
  // Large red-glowing ash boulder:
  [124] = {
    .style = AZ_WSTY_TRIFAN, .bezel = 50.0,
    .color1 = {104, 94, 100, 128}, .color2 = {50, 30, 50, 128},
    .underglow = {48, 0, 0, 255}, .impact_damage_coeff = 10.0,
    .elasticity = 0.2, .polygon = AZ_INIT_POLYGON(wall_vertices_129)
  },
  // Green/gray metal block:
  [125] = {
    .style = AZ_WSTY_TRIFAN,
    .color1 = {130, 130, 115, 255}, .color2 = {24, 48, 24, 255},
    .elasticity = 0.5, .polygon = AZ_INIT_POLYGON(wall_vertices_22)
  },
  // Green/gray metal half-size block:
  [126] = {
    .style = AZ_WSTY_TRIFAN,
    .color1 = {140, 140, 128, 255}, .color2 = {24, 48, 24, 255},
    .elasticity = 0.5, .polygon = AZ_INIT_POLYGON(wall_vertices_23)
  },
  // Green/gray long rectangle metal block:
  [127] = {
    .style = AZ_WSTY_TRIFAN,
    .color1 = {140, 140, 128, 255}, .color2 = {24, 48, 24, 255},
    .elasticity = 0.5, .polygon = AZ_INIT_POLYGON(wall_vertices_31)
  },
  // Small dark volcanic rock:
  [128] = {
    .style = AZ_WSTY_VOLCANIC, .bezel = 25.0,
    .color1 = {50, 47, 45, 255}, .color2 = {25, 15, 20, 255},
    .color3 = {30, 30, 30, 192}, .impact_damage_coeff = 5.0,
    .elasticity = 0.2, .polygon = AZ_INIT_POLYGON(wall_vertices_128)
  },
  // Medium dark volcanic rock:
  [129] = {
    .style = AZ_WSTY_VOLCANIC, .bezel = 30.0,
    .color1 = {52, 47, 45, 255}, .color2 = {25, 15, 20, 255},
    .color3 = {30, 30, 30, 192}, .impact_damage_coeff = 5.0,
    .elasticity = 0.2, .polygon = AZ_INIT_POLYGON(wall_vertices_129)
  },
  // Large dark volcanic rock:
  [130] = {
    .style = AZ_WSTY_VOLCANIC, .bezel = 45.0,
    .color1 = {52, 47, 50, 255}, .color2 = {25, 15, 25, 255},
    .color3 = {30, 30, 30, 192}, .impact_damage_coeff = 5.0,
    .elasticity = 0.2, .polygon = AZ_INIT_POLYGON(wall_vertices_130)
  },
  // Small light volcanic rock:
  [131] = {
    .style = AZ_WSTY_VOLCANIC, .bezel = 30.0,
    .color1 = {100, 95, 90, 255}, .color2 = {25, 15, 20, 255},
    .color3 = {30, 20, 20, 192}, .impact_damage_coeff = 5.0,
    .elasticity = 0.2, .polygon = AZ_INIT_POLYGON(wall_vertices_128)
  },
  // Medium light volcanic rock:
  [132] = {
    .style = AZ_WSTY_VOLCANIC, .bezel = 35.0,
    .color1 = {105, 95, 90, 255}, .color2 = {25, 15, 20, 255},
    .color3 = {30, 25, 25, 192}, .impact_damage_coeff = 5.0,
    .elasticity = 0.2, .polygon = AZ_INIT_POLYGON(wall_vertices_129)
  },
  // Large light volcanic rock:
  [133] = {
    .style = AZ_WSTY_VOLCANIC, .bezel = 50.0,
    .color1 = {105, 95, 100, 255}, .color2 = {25, 15, 25, 255},
    .color3 = {40, 20, 0, 192}, .impact_damage_coeff = 5.0,
    .elasticity = 0.2, .polygon = AZ_INIT_POLYGON(wall_vertices_130)
  },
  // Solid-colored dark green rectangle:
  [134] = {
    .style = AZ_WSTY_TRIFAN_ALT,
    .color1 = {64, 0, 24, 255}, .color2 = {64, 0, 24, 255},
    .elasticity = 0.1, .impact_damage_coeff = 2.5,
    .polygon = AZ_INIT_POLYGON(wall_vertices_21)
  },
  // Short jungle grass wall:
  [135] = {
    .style = AZ_WSTY_QUADSTRIP_213, .bezel = 0.75,
    .color2 = {64, 40, 16, 255}, .color1 = {96, 64, 16, 255},
    .color3 = {64, 0, 24, 255},
    .elasticity = 0.1, .impact_damage_coeff = 2.5,
    .polygon = AZ_INIT_POLYGON(wall_vertices_114)
  },
  // Long jungle grass wall:
  [136] = {
    .style = AZ_WSTY_QUADSTRIP_213, .bezel = 0.75,
    .color2 = {64, 40, 16, 255}, .color1 = {96, 64, 16, 255},
    .color3 = {64, 0, 24, 255},
    .elasticity = 0.1, .impact_damage_coeff = 2.5,
    .polygon = AZ_INIT_POLYGON(wall_vertices_115)
  },
  // Jungle grass convex wall:
  [137] = {
    .style = AZ_WSTY_QUADSTRIP_213, .bezel = 0.75,
    .color2 = {64, 40, 16, 255}, .color1 = {96, 64, 16, 255},
    .color3 = {64, 0, 24, 255},
    .elasticity = 0.1, .impact_damage_coeff = 2.5,
    .polygon = AZ_INIT_POLYGON(wall_vertices_43)
  },
  // Jungle grass concave wall:
  [138] = {
    .style = AZ_WSTY_QUADSTRIP_213, .bezel = -0.75,
    .color3 = {64, 40, 16, 255}, .color1 = {96, 64, 16, 255},
    .color2 = {64, 0, 24, 255},
    .elasticity = 0.1, .impact_damage_coeff = 2.5,
    .polygon = AZ_INIT_POLYGON(wall_vertices_43)
  },
  // Blue-gray pipe:
  [139] = {
    .style = AZ_WSTY_QUADSTRIP_213, .color1 = {128, 128, 192, 255},
    .color2 = {40, 40, 48, 255}, .color3 = {40, 40, 48, 255},
    .elasticity = 0.5, .polygon = AZ_INIT_POLYGON(wall_vertices_0)
  },
  // Blue-gray angle pipe:
  [140] = {
    .style = AZ_WSTY_QUADSTRIP_213, .color1 = {128, 128, 192, 255},
    .color2 = {40, 40, 48, 255}, .color3 = {40, 40, 48, 255},
    .elasticity = 0.4, .polygon = AZ_INIT_POLYGON(wall_vertices_2)
  },
  // Blue-gray short angle pipe:
  [141] = {
    .style = AZ_WSTY_QUADSTRIP_213, .color1 = {128, 128, 192, 255},
    .color2 = {40, 40, 48, 255}, .color3 = {40, 40, 48, 255},
    .elasticity = 0.4, .polygon = AZ_INIT_POLYGON(wall_vertices_63)
  },
  // 2-way steel connector:
  [142] = {
    .style = AZ_WSTY_TFQS_213, .bezel = 0.3, .color1 = {200, 179, 179, 255},
    .color2 = {128, 112, 112, 255}, .color3 = {64, 64, 64, 255},
    .elasticity = 0.5, .polygon = AZ_INIT_POLYGON(wall_vertices_142)
  },
  // 3-way steel connector:
  [143] = {
    .style = AZ_WSTY_TFQS_213, .bezel = 0.3, .color1 = {200, 200, 179, 255},
    .color2 = {128, 128, 112, 255}, .color3 = {64, 64, 64, 255},
    .elasticity = 0.5, .polygon = AZ_INIT_POLYGON(wall_vertices_143)
  },
  // 4-way steel connector:
  [144] = {
    .style = AZ_WSTY_TFQS_213, .bezel = 0.3, .color1 = {179, 200, 179, 255},
    .color2 = {112, 128, 112, 255}, .color3 = {64, 64, 64, 255},
    .elasticity = 0.5, .polygon = AZ_INIT_POLYGON(wall_vertices_144)
  },
  // 5-way steel connector:
  [145] = {
    .style = AZ_WSTY_TFQS_213, .bezel = 0.3, .color1 = {179, 200, 200, 255},
    .color2 = {112, 128, 128, 255}, .color3 = {64, 64, 64, 255},
    .elasticity = 0.5, .polygon = AZ_INIT_POLYGON(wall_vertices_145)
  },
  // 8-way steel connector:
  [146] = {
    .style = AZ_WSTY_TFQS_213, .bezel = 0.3, .color1 = {179, 179, 200, 255},
    .color2 = {112, 112, 128, 255}, .color3 = {64, 64, 64, 255},
    .elasticity = 0.5, .polygon = AZ_INIT_POLYGON(wall_vertices_146)
  },
  // Long silver bezel pipe:
  [147] = {
    .style = AZ_WSTY_QUADSTRIP_213, .color1 = {179, 179, 179, 255},
    .color2 = {64, 64, 64, 255}, .color3 = {64, 64, 64, 255},
    .elasticity = 0.5, .polygon = AZ_INIT_POLYGON(wall_vertices_147)
  },
  // Short silver bezel pipe:
  [148] = {
    .style = AZ_WSTY_QUADSTRIP_213, .color1 = {179, 179, 179, 255},
    .color2 = {64, 64, 64, 255}, .color3 = {64, 64, 64, 255},
    .elasticity = 0.5, .polygon = AZ_INIT_POLYGON(wall_vertices_148)
  },
  // Green/gray triangular full metal panel:
  [149] = {
    .style = AZ_WSTY_BEZEL_21, .bezel = 10.0,
    .color1 = {111, 120, 111, 255}, .color2 = {36, 48, 36, 255},
    .elasticity = 0.5, .polygon = AZ_INIT_POLYGON(wall_vertices_149)
  },
  // Green/gray square full metal panel:
  [150] = {
    .style = AZ_WSTY_BEZEL_21, .bezel = 10.0,
    .color1 = {111, 120, 111, 255}, .color2 = {36, 48, 36, 255},
    .elasticity = 0.5, .polygon = AZ_INIT_POLYGON(wall_vertices_1)
  },
  // Green/gray square empty metal panel:
  [151] = {
    .style = AZ_WSTY_GIRDER_CAPS, .bezel = 10.0,
    .color1 = {111, 130, 111, 255}, .color2 = {36, 48, 36, 255},
    .color3 = {36, 48, 36, 255},
    .elasticity = 0.5, .polygon = AZ_INIT_POLYGON(wall_vertices_1)
  },
  // Glowing jewel column thick:
  [152] = {
    .style = AZ_WSTY_QUADSTRIP_213, .color1 = {96, 64, 128, 40},
    .color2 = {96, 64, 128, 196}, .color3 = {96, 64, 128, 196},
    .underglow = {20, 32, 45, 255}, .impact_damage_coeff = 3.0,
    .elasticity = 0.4, .polygon = AZ_INIT_POLYGON(wall_vertices_0)
  },
  // Glowing jewel column thick end cap:
  [153] = {
    .style = AZ_WSTY_QUADSTRIP_213, .color1 = {96, 64, 128, 40},
    .color2 = {96, 64, 128, 196}, .color3 = {96, 64, 128, 196},
    .underglow = {20, 32, 45, 255}, .impact_damage_coeff = 3.0,
    .elasticity = 0.4, .polygon = AZ_INIT_POLYGON(wall_vertices_153)
  },
  // Glowing jewel column thin:
  [154] = {
    .style = AZ_WSTY_QUADSTRIP_213, .color1 = {96, 64, 128, 40},
    .color2 = {96, 64, 128, 196}, .color3 = {96, 64, 128, 196},
    .underglow = {20, 32, 45, 255}, .impact_damage_coeff = 3.0,
    .elasticity = 0.4, .polygon = AZ_INIT_POLYGON(wall_vertices_154)
  },
  // Glowing jewel column thin end cap:
  [155] = {
    .style = AZ_WSTY_QUADSTRIP_213, .color1 = {96, 64, 128, 40},
    .color2 = {96, 64, 128, 196}, .color3 = {96, 64, 128, 196},
    .underglow = {20, 32, 45, 255}, .impact_damage_coeff = 3.0,
    .elasticity = 0.4, .polygon = AZ_INIT_POLYGON(wall_vertices_155)
  },
  // Short green twisted vine:
  [156] = {
    .style = AZ_WSTY_QUADSTRIP_ALT_213, .color1 = {70, 80, 40, 255},
    .color2 = {24, 40, 32, 255}, .color3 = {24, 40, 32, 255},
    .elasticity = 0.3, .impact_damage_coeff = 4.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_156)
  },
  // Long green twisted vine:
  [157] = {
    .style = AZ_WSTY_QUADSTRIP_ALT_213, .color1 = {70, 80, 40, 255},
    .color2 = {24, 40, 32, 255}, .color3 = {24, 40, 32, 255},
    .elasticity = 0.3, .impact_damage_coeff = 4.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_157)
  },
  // Solid-colored dark purple rectangle (for snow/ice walls):
  [158] = {
    .style = AZ_WSTY_TRIFAN_ALT,
    .color1 = {12, 4, 24, 255}, .color2 = {12, 4, 24, 255},
    .elasticity = 0.25, .impact_damage_coeff = 2.5,
    .polygon = AZ_INIT_POLYGON(wall_vertices_21)
  },
  // Solid-colored dark brown rectangle (for dirt walls):
  [159] = {
    .style = AZ_WSTY_TRIFAN_ALT,
    .color1 = {24, 12, 0, 255}, .color2 = {24, 12, 0, 255},
    .elasticity = 0.25, .impact_damage_coeff = 2.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_21)
  },
  // Solid-colored dark brown small rectangle (for dirt walls):
  [160] = {
    .style = AZ_WSTY_TRIFAN_ALT,
    .color1 = {24, 12, 0, 255}, .color2 = {24, 12, 0, 255},
    .elasticity = 0.25, .impact_damage_coeff = 2.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_0)
  },
  // High Energy Lab windows:
  [161] = {
    .style = AZ_WSTY_BEZEL_21, .bezel = 18.0,
    .color1 = {255, 255, 255, 64}, .color2 = {64, 64, 16, 255},
    .elasticity = 0.4,
    .polygon = AZ_INIT_POLYGON(wall_vertices_161)
  },
  [162] = {
    .style = AZ_WSTY_BEZEL_21, .bezel = 18.0,
    .color1 = {255, 255, 255, 64}, .color2 = {64, 64, 16, 255},
    .elasticity = 0.4,
    .polygon = AZ_INIT_POLYGON(wall_vertices_162)
  },
  // Yellow rectangle block:
  [163] = {
    .style = AZ_WSTY_BEZEL_21, .bezel = 10.0,
    .color1 = {176, 176, 112, 255}, .color2 = {60, 48, 32, 255},
    .elasticity = 0.4,
    .polygon = AZ_INIT_POLYGON(wall_vertices_0)
  },
  // Yellow angle block:
  [164] = {
    .style = AZ_WSTY_BEZEL_21, .bezel = 10.0,
    .color1 = {176, 176, 112, 255}, .color2 = {60, 48, 32, 255},
    .elasticity = 0.4,
    .polygon = AZ_INIT_POLYGON(wall_vertices_2)
  },
  // Large blue square block:
  [165] = {
    .style = AZ_WSTY_BEZEL_21, .bezel = 10.0,
    .color1 = {112, 128, 128, 255}, .color2 = {32, 48, 64, 255},
    .elasticity = 0.4,
    .polygon = AZ_INIT_POLYGON(wall_vertices_1)
  },
  // Huge blue rectangular block:
  [166] = {
    .style = AZ_WSTY_BEZEL_21, .bezel = 10.0,
    .color1 = {112, 128, 128, 255}, .color2 = {32, 48, 64, 255},
    .elasticity = 0.4,
    .polygon = AZ_INIT_POLYGON(wall_vertices_21)
  },
};

/*===========================================================================*/

const int AZ_NUM_WALL_DATAS = AZ_ARRAY_SIZE(wall_datas);

static bool wall_data_initialized = false;

void az_init_wall_datas(void) {
  assert(!wall_data_initialized);
  AZ_ARRAY_LOOP(data, wall_datas) {
    const az_polygon_t polygon = data->polygon;
    assert(polygon.num_vertices >= 3);
    double radius = 0.0;
    for (int i = polygon.num_vertices - 1, j = 0; i >= 0; j = i--) {
      assert(!az_vapprox(polygon.vertices[i], polygon.vertices[j]));
      radius = fmax(radius, az_vnorm(polygon.vertices[i]));
    }
    data->bounding_radius = radius + 0.01; // small safety margin
  }
  wall_data_initialized = true;
}

const az_wall_data_t *az_get_wall_data(int index) {
  assert(wall_data_initialized);
  assert(index >= 0);
  assert(index < AZ_NUM_WALL_DATAS);
  return &wall_datas[index];
}

int az_wall_data_index(const az_wall_data_t *data) {
  assert(data >= wall_datas);
  assert(data < wall_datas + AZ_NUM_WALL_DATAS);
  return data - wall_datas;
}

/*===========================================================================*/

bool az_point_touches_wall(const az_wall_t *wall, az_vector_t point) {
  assert(wall->kind != AZ_WALL_NOTHING);
  return (az_vwithin(point, wall->position, wall->data->bounding_radius) &&
          az_polygon_contains(wall->data->polygon,
                              az_vrotate(az_vsub(point, wall->position),
                                         -wall->angle)));
}

bool az_circle_touches_wall(
    const az_wall_t *wall, double radius, az_vector_t center) {
  assert(wall->kind != AZ_WALL_NOTHING);
  return (az_vwithin(center, wall->position,
                     radius + wall->data->bounding_radius) &&
          az_circle_touches_polygon_trans(wall->data->polygon, wall->position,
                                          wall->angle, radius, center));
}

bool az_ray_hits_wall(const az_wall_t *wall, az_vector_t start,
                      az_vector_t delta, az_vector_t *point_out,
                      az_vector_t *normal_out) {
  assert(wall->kind != AZ_WALL_NOTHING);
  return (az_ray_hits_bounding_circle(start, delta, wall->position,
                                      wall->data->bounding_radius) &&
          az_ray_hits_polygon_trans(wall->data->polygon, wall->position,
                                    wall->angle, start, delta,
                                    point_out, normal_out));
}

bool az_circle_hits_wall(
    const az_wall_t *wall, double radius, az_vector_t start, az_vector_t delta,
    az_vector_t *pos_out, az_vector_t *normal_out) {
  assert(wall->kind != AZ_WALL_NOTHING);
  return (az_ray_hits_bounding_circle(start, delta, wall->position,
                                      wall->data->bounding_radius + radius) &&
          az_circle_hits_polygon_trans(wall->data->polygon, wall->position,
                                       wall->angle, radius, start, delta,
                                       pos_out, normal_out));
}

bool az_arc_circle_hits_wall(
    const az_wall_t *wall, double circle_radius,
    az_vector_t start, az_vector_t spin_center, double spin_angle,
    double *angle_out, az_vector_t *pos_out, az_vector_t *normal_out) {
  assert(wall->kind != AZ_WALL_NOTHING);
  return (az_arc_ray_might_hit_bounding_circle(
              start, spin_center, spin_angle, wall->position,
              wall->data->bounding_radius + circle_radius) &&
          az_arc_circle_hits_polygon_trans(
              wall->data->polygon, wall->position, wall->angle,
              circle_radius, start, spin_center, spin_angle,
              angle_out, pos_out, normal_out));
}

/*===========================================================================*/
