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
#include <math.h> // for fmax
#include <stdbool.h>

#include "azimuth/util/misc.h"
#include "azimuth/util/polygon.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

static az_vector_t wall_vertices_0[] = {
  {50, 25}, {-50, 25}, {-50, -25}, {50, -25}
};
static az_vector_t wall_vertices_1[] = {
  {50, -50}, {50, 50}, {-50, 50}, {-50, -50}
};
static az_vector_t wall_vertices_2[] = {
  {60, 25}, {-10.710678, 25}, {-60.710678, -25}, {-25.355339, -60.355339},
  {10, -25}, {60, -25}
};
static az_vector_t wall_vertices_3[] = {
  {-20, -8}, {15, -10}, {25, 0}, {15, 10}, {-20, 8}
};
static az_vector_t wall_vertices_4[] = {
  {-30, -10}, {15, -15}, {35, 0}, {15, 15}, {-30, 10}
};
static az_vector_t wall_vertices_5[] = {
  {31, 10}, {33, 30}, {20, 43}, {-2, 44}, {-7, 42}, {-30, 30}, {-48, 30},
  {-59, 0}, {-41, -34}, {-28, -35}, {-20, -29}, {-5, -42}, {4, -42}, {19, -27},
  {35, -28}, {40, 0}
};
static az_vector_t wall_vertices_girder_long[] = {
  {25, 0}, {50, 25}, {-50, 25}, {-25, 0}, {-50, -25}, {50, -25}
};
static az_vector_t wall_vertices_girder_long_capped[] = {
  {25, 0}, {50, 25}, {-50, 25}, {-50, -25}, {50, -25}
};
static az_vector_t wall_vertices_girder_long_double_capped[] = {
  {50, -25}, {50, 25}, {-50, 25}, {-50, -25}
};
static az_vector_t wall_vertices_girder_short[] = {
  {2, 0}, {27, 25}, {-27, 25}, {-2, 0}, {-27, -25}, {27, -25}
};
static az_vector_t wall_vertices_girder_skinny[] = {
  {39, 0}, {59, 20}, {-59, 20}, {-39, 0}, {-59, -20}, {59, -20}
};
static az_vector_t wall_vertices_girder_skinny_capped[] = {
  {39, 0}, {59, 20}, {-59, 20}, {-59, -20}, {59, -20}
};
static az_vector_t wall_vertices_11[] = {
  {17, 4}, {23, 10}, {20, 21}, {8, 28}, {-2, 22}, {-20, 18}, {-20, 0},
  {-27, -10}, {-19, -21}, {-3, -18}, {7, -26}, {16, -20}
};
static az_vector_t wall_vertices_12[] = {
  {6, 36}, {-10, 42}, {-20, 38}, {-50, 35}, {-54, 5}, {-50, -30}, {-35, -37},
  {-25, -32}, {-5, -41}, {25, -31}, {50, -34}, {53, -3}, {50, 35}, {32, 41}
};
static az_vector_t wall_vertices_13[] = {
  {77, 20}, {81, 60}, {55, 86}, {11, 88}, {1, 84}, {-45, 60}, {-81, 60},
  {-103, 0}, {-72, -65}, {-41, -70}, {-25, -64}, {5, -84}, {23, -84},
  {53, -66}, {85, -56}, {95, -10}
};
static az_vector_t wall_vertices_14[] = {
  {12, 36}, {-20, 42}, {-40, 38}, {-100, 35}, {-108, 5}, {-100, -30},
  {-70, -37}, {-50, -32}, {-10, -41}, {50, -31}, {100, -34}, {106, -3},
  {100, 35}, {64, 41}
};
static az_vector_t wall_vertices_18[] = {
  {0, 30}, {-8, 28}, {0, -30}, {8, 28}
};
static az_vector_t wall_vertices_19[] = {
  {0, 20}, {-6, 18}, {0, -20}, {6, 18}
};
static az_vector_t wall_vertices_21[] = {
  {125, 50}, {-125, 50}, {-125, -50}, {125, -50}
};
static az_vector_t wall_vertices_22[] = {
  {50, 33}, {-50, 33}, {-50, -33}, {50, -33}
};
static az_vector_t wall_vertices_23[] = {
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
static az_vector_t wall_vertices_26[] = {
  {40, 30}, {39.24, 33.83}, {37.07, 37.07}, {33.83, 39.24}, {30, 40},
  {-30, 40}, {-33.83, 39.24}, {-37.07, 37.07}, {-39.24, 33.83}, {-40, 30},
  {-40, -30}, {-39.24, -33.83}, {-37.07, -37.07}, {-33.83, -39.24}, {-30, -40},
  {30, -40}, {33.83, -39.24}, {37.07, -37.07}, {39.24, -33.83}, {40, -30}
};
static az_vector_t wall_vertices_27[] = {
  // Rounded corner for {16.57, 40}:
  {16.57, 26.93}, {14.98, 30.76}, {11.16, 32.35}, {7.33, 30.76},
  // Rounded corner for {-40, -16.57}:
  {-30.76, -7.33}, {-31.94, -9.09}, {-32.35, -11.16},
  {-31.94, -13.23}, {-30.76, -14.99}, {-29.01, -16.16},
  // Rounded corner for {16.57, -40}:
  {9.09, -31.94}, {11.16, -32.35}, {13.23, -31.94},
  {14.99, -30.76}, {16.16, -29.01}, {16.57, -26.93}
};
static az_vector_t wall_vertices_31[] = {
  {25, 55}, {-25, 55}, {-25, -55}, {25, -55}
};
static az_vector_t wall_vertices_32[] = {
  {28, 28}, {-28, 28}, {-28, -28}, {28, -28}
};
static az_vector_t wall_vertices_34[] = {
  {28, 28}, {-28, 28}, {-28, 11.6}, {11.6, -28}, {28, -28}
};
static az_vector_t wall_vertices_37[] = {
  {0, 45}, {-20, 42}, {-10, 0}, {-7, -3}, {0, -45}, {22, 42}
};
static az_vector_t wall_vertices_41[] = {
  {0, -80}, {24, -94}, {47, -93}, {98, -98}, {120, -50}, {110, -15},
  {118, 20}, {119, 52}, {102, 95}, {75, 90}, {30, 88}, {0, 80}, {-30, 97},
  {-80, 90}, {-105, 60}, {-103, 37}, {-120, 20}, {-110, -40}, {-115, -70},
  {-100, -90}, {-40, -95}
};
static az_vector_t wall_vertices_42[] = {
  {100, 20}, {50, 15}, {0, 23}, {-50, 10}, {-100, 20},
  {-100, -40}, {-50, -80}, {0, -46}, {50, -50}, {100, -40}
};
static az_vector_t wall_vertices_43[] = {
  {35, 34}, {9, 32}, {-20, 20}, {-31, -8}, {-33, -35},
  {30, -35}, {30, -33}, {31, -31}, {33, -30}, {35, -30}
};
static az_vector_t wall_vertices_47[] = {
  {33, -30}, {33, 30}, {-32, 30}, {-32, -30}
};
static az_vector_t wall_vertices_50[] = {
  {20, 0}, {40, 20}, {-40, 20}, {-20, 0}, {-40, -20}, {40, -20}
};
static az_vector_t wall_vertices_51[] = {
  {47.5, 0}, {72.5, 25}, {-72.5, 25}, {-47.5, 0}, {-72.5, -25}, {72.5, -25}
};

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
    .color1 = {96, 80, 64, 255}, .color2 = {48, 32, 16, 255},
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
    .color1 = {90, 80, 70, 255}, .color2 = {48, 32, 16, 255},
    .elasticity = 0.25, .impact_damage_coeff = 4.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_11)
  },
  // Gray/brown rock wall:
  [12] = {
    .style = AZ_WSTY_BEZEL_21, .bezel = 25.0,
    .color1 = {90, 80, 80, 255}, .color2 = {48, 32, 16, 255},
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
    .color1 = {90, 80, 80, 255}, .color2 = {48, 32, 16, 255},
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
    .color1 = {192, 24, 24, 255}, .color2 = {80, 64, 64, 255},
    .elasticity = 0.25, .polygon = AZ_INIT_POLYGON(wall_vertices_22)
  },
  // Red/gray rectangle half-size cinderbrick:
  [23] = {
    .style = AZ_WSTY_BEZEL_12, .bezel = 15.0,
    .color1 = {192, 24, 24, 255}, .color2 = {80, 64, 64, 255},
    .elasticity = 0.25, .polygon = AZ_INIT_POLYGON(wall_vertices_23)
  },
  // Green/purple hexagonal block:
  [24] = {
    .style = AZ_WSTY_BEZEL_12, .bezel = 20.0,
    .color1 = {64, 160, 8, 255}, .color2 = {60, 20, 90, 255},
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
    .color1 = {192, 24, 24, 255}, .color2 = {80, 64, 64, 255},
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
    .style = AZ_WSTY_QUADSTRIP_213,
    .color1 = {192, 192, 96, 255}, .color2 = {48, 48, 32, 255},
    .color3 = {48, 48, 32, 255},
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
  // Small stalactite:
  [36] = {
    .style = AZ_WSTY_BEZEL_ALT_21, .bezel = 1000.0,
    .color1 = {90, 80, 80, 255}, .color2 = {40, 32, 24, 255},
    .elasticity = 0.25, .impact_damage_coeff = 6.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_18)
  },
  // Large stalactite:
  [37] = {
    .style = AZ_WSTY_BEZEL_ALT_21, .bezel = 20.0,
    .color1 = {90, 80, 80, 255}, .color2 = {40, 32, 24, 255},
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
    .style = AZ_WSTY_QUADSTRIP_213,
    .color1 = {128, 112, 112, 255}, .color2 = {32, 24, 24, 255},
    .color3 = {32, 24, 24, 255},
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
    .color3 = {0, 0, 0, 255},
    .elasticity = 0.15, .impact_damage_coeff = 2.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_42)
  },
  // Grass/dirt convex wall:
  [43] = {
    .style = AZ_WSTY_QUADSTRIP_213, .bezel = 0.5,
    .color1 = {96, 48, 0, 255}, .color2 = {64, 128, 0, 255},
    .color3 = {0, 0, 0, 255},
    .elasticity = 0.15, .impact_damage_coeff = 2.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_43)
  },
  // Grass/dirt concave wall:
  [44] = {
    .style = AZ_WSTY_QUADSTRIP_213, .bezel = -0.5,
    .color1 = {96, 48, 0, 255}, .color2 = {0, 0, 0, 255},
    .color3 = {64, 128, 0, 255},
    .elasticity = 0.15, .impact_damage_coeff = 2.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_43)
  },
  // Dust/dirt straight-ish wall:
  [45] = {
    .style = AZ_WSTY_QUADSTRIP_213, .bezel = 0.5,
    .color1 = {96, 48, 0, 255}, .color2 = {128, 64, 32, 255},
    .color3 = {0, 0, 0, 255},
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
  }
};

/*===========================================================================*/

const int AZ_NUM_WALL_DATAS = AZ_ARRAY_SIZE(wall_datas);

static bool wall_data_initialized = false;

void az_init_wall_datas(void) {
  assert(!wall_data_initialized);
  AZ_ARRAY_LOOP(data, wall_datas) {
    assert(data->polygon.num_vertices >= 3);
    double radius = 0.0;
    for (int i = 0; i < data->polygon.num_vertices; ++i) {
      radius = fmax(radius, az_vnorm(data->polygon.vertices[i]));
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
    az_vector_t *pos_out, az_vector_t *impact_out) {
  assert(wall->kind != AZ_WALL_NOTHING);
  return (az_ray_hits_bounding_circle(start, delta, wall->position,
                                      wall->data->bounding_radius + radius) &&
          az_circle_hits_polygon_trans(wall->data->polygon, wall->position,
                                       wall->angle, radius, start, delta,
                                       pos_out, impact_out));
}

bool az_arc_circle_hits_wall(
    const az_wall_t *wall, double circle_radius,
    az_vector_t start, az_vector_t spin_center, double spin_angle,
    double *angle_out, az_vector_t *pos_out, az_vector_t *impact_out) {
  assert(wall->kind != AZ_WALL_NOTHING);
  return (az_arc_ray_might_hit_bounding_circle(
              start, spin_center, spin_angle, wall->position,
              wall->data->bounding_radius + circle_radius) &&
          az_arc_circle_hits_polygon_trans(
              wall->data->polygon, wall->position, wall->angle,
              circle_radius, start, spin_center, spin_angle,
              angle_out, pos_out, impact_out));
}

/*===========================================================================*/
