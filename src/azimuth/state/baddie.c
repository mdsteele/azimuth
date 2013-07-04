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

#include "azimuth/state/baddie.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h> // for NULL
#include <string.h> // for memset

#include "azimuth/state/pickup.h" // for AZ_PUPF_* macros
#include "azimuth/util/misc.h"
#include "azimuth/util/polygon.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

#define DECL_COMPONENTS(c) .num_components=AZ_ARRAY_SIZE(c), .components=(c)

static const az_vector_t turret_vertices[] = {
  {20, 0}, {10, 17.320508075688775}, {-10, 17.320508075688775},
  {-20, 0}, {-10, -17.320508075688775}, {10, -17.320508075688775}
};
static const az_vector_t turret_cannon_vertices[] = {
  {30, 5}, {0, 5}, {0, -5}, {30, -5}
};
static az_component_data_t turret_components[] = {
  { .polygon = AZ_INIT_POLYGON(turret_cannon_vertices), .impact_damage = 10.0 }
};
static az_component_data_t broken_turret_components[] = {
  { .polygon = AZ_INIT_POLYGON(turret_cannon_vertices), .impact_damage = 5.0 }
};
static az_component_data_t armored_turret_components[] = {
  { .polygon = AZ_INIT_POLYGON(turret_cannon_vertices),
    .immunities = AZ_DMGF_NORMAL, .impact_damage = 10.0 }
};
static az_component_data_t crawling_turret_components[] = {
  { .polygon = AZ_INIT_POLYGON(turret_cannon_vertices), .impact_damage = 10.0 }
};
static az_component_data_t beam_turret_components[] = {
  { .polygon = AZ_INIT_POLYGON(turret_cannon_vertices),
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_BEAM), .impact_damage = 10.0 }
};
static az_component_data_t rocket_turret_components[] = {
  { .polygon = AZ_INIT_POLYGON(turret_cannon_vertices),
    .immunities = AZ_DMGF_NORMAL, .impact_damage = 10.0 }
};

static const az_vector_t zipper_vertices[] = {
  {20, 3}, {15, 6}, {10, 7}, {-10, 4}, {-15, 2},
  {-15, -2}, {-10, -4}, {10, -7}, {15, -6}, {20, -3}
};

static az_component_data_t atom_components[] = {
  { .init_position = {20, 0},
    .bounding_radius = 6.3, .immunities = ~(AZ_DMGF_PIERCE | AZ_DMGF_CPLUS),
    .impact_damage = 15.0 },
  { .init_position = {-1.675, -4.977}, .init_angle = AZ_DEG2RAD(100),
    .bounding_radius = 6.5, .immunities = ~(AZ_DMGF_PIERCE | AZ_DMGF_CPLUS),
    .impact_damage = 15.0 },
  { .init_position = {8.212, 16.96}, .init_angle = AZ_DEG2RAD(200),
    .bounding_radius = 6.7, .immunities = ~(AZ_DMGF_PIERCE | AZ_DMGF_CPLUS),
    .impact_damage = 15.0 }
};

static const az_vector_t box_vertices[] = {
  {16, 8}, {11, 13}, {-11, 13}, {-16, 8},
  {-16, -8}, {-11, -13}, {11, -13}, {16, -8}
};

static const az_vector_t clam_shell1_vertices[] = {
  {24, 0}, {27,  2}, {24,  8}, {20,  11}, {9,  14}, {-5, 10}, {-8,  6}, {-9, 0}
};
static const az_vector_t clam_shell2_vertices[] = {
  {24, 0}, {27, -2}, {24, -7}, {19, -10}, {7, -13}, {-6, -9}, {-8, -5}, {-9, 0}
};
static az_component_data_t clam_components[] = {
  { .init_position = {-4, 0}, .polygon = AZ_INIT_POLYGON(clam_shell1_vertices),
    .immunities = ~(AZ_DMGF_FREEZE | AZ_DMGF_PIERCE | AZ_DMGF_CPLUS) },
  { .init_position = {-4, 0}, .polygon = AZ_INIT_POLYGON(clam_shell2_vertices),
    .immunities = ~(AZ_DMGF_FREEZE | AZ_DMGF_PIERCE | AZ_DMGF_CPLUS) }
};

static const az_vector_t nightbug_vertices[] = {
  {17, 0}, {9, 10}, {-11, 5}, {-11, -5}, {9, -10}
};

static const az_vector_t zenith_core_vertices[] = {
  { 120, 0}, { 84.852813742385706,  84.852813742385706},
  {0,  120}, {-84.852813742385706,  84.852813742385706},
  {-120, 0}, {-84.852813742385706, -84.852813742385706},
  {0, -120}, { 84.852813742385706, -84.852813742385706}
};

static const az_vector_t dragonfly_vertices[] = {
  {20, 2}, {10, 5}, {4, 10}, {0, 5}, {-15, 1},
  {-15, -1}, {0, -5}, {4, -10}, {10, -5}, {20, -2}
};

static const az_vector_t cave_crawler_vertices[] = {
  {10, -8}, {10, 8}, {-2, 18}, {-15, 10}, {-15, -10}, {-2, -18}
};

static const az_vector_t beam_sensor_casing_vertices[] = {
  {4, 0}, {10, 12}, {10, 20}, {-20, 20}, {-20, -20}, {10, -20}, {10, -12}
};
static az_component_data_t beam_sensor_components[] = {
  { .polygon = AZ_INIT_POLYGON(beam_sensor_casing_vertices), .immunities = ~0 }
};

static const az_vector_t rockwyrm_pincer1_vertices[] = {
  {40, -10}, {20, -7}, {0, -12}, {-15, -12}, {-30, -6},
  {-20,  18}, {0,  24}, {18,  20}, {30,  10}
};
static const az_vector_t rockwyrm_pincer2_vertices[] = {
  {40,  10}, {20,  7}, {0,  12}, {-15,  12}, {-30,  6},
  {-20, -18}, {0, -24}, {18, -20}, {30, -10}
};
static az_component_data_t rockwyrm_components[] = {
  { .polygon = AZ_INIT_POLYGON(rockwyrm_pincer1_vertices),
    .init_position = {-5,  10}, .immunities = ~0, .impact_damage = 8.0 },
  { .polygon = AZ_INIT_POLYGON(rockwyrm_pincer2_vertices),
    .init_position = {-5, -10}, .immunities = ~0, .impact_damage = 8.0 },
  { .init_position = { -40, 0}, .bounding_radius = 29.0,
    .immunities = ~0, .impact_damage = 3.0 },
  { .init_position = { -80, 0}, .bounding_radius = 30.0,
    .immunities = ~0, .impact_damage = 3.0 },
  { .init_position = {-120, 0}, .bounding_radius = 30.0,
    .immunities = ~0, .impact_damage = 3.0 },
  { .init_position = {-160, 0}, .bounding_radius = 30.0,
    .immunities = ~0, .impact_damage = 3.0 },
  { .init_position = {-200, 0}, .bounding_radius = 29.0,
    .immunities = ~0, .impact_damage = 3.0 },
  { .init_position = {-240, 0}, .bounding_radius = 28.0,
    .immunities = ~0, .impact_damage = 3.0 },
  { .init_position = {-280, 0}, .bounding_radius = 28.0,
    .immunities = ~0, .impact_damage = 3.0 },
  { .init_position = {-320, 0}, .bounding_radius = 27.0,
    .immunities = ~0, .impact_damage = 3.0 },
  { .init_position = {-360, 0}, .bounding_radius = 26.0,
    .immunities = ~0, .impact_damage = 3.0 },
  { .init_position = {-400, 0}, .bounding_radius = 25.0,
    .immunities = ~0, .impact_damage = 3.0 }
};

static az_component_data_t wyrmling_components[] = {
  { .init_position = {-5, 0}, .init_angle = AZ_DEG2RAD(0),
    .bounding_radius = 4.5, .impact_damage = 2.0 },
  { .init_position = {-5, 5}, .init_angle = AZ_DEG2RAD(-90),
    .bounding_radius = 4.5, .impact_damage = 2.0 },
  { .init_position = { 0, 5}, .init_angle = AZ_DEG2RAD(180),
    .bounding_radius = 4.0, .impact_damage = 2.0 },
  { .init_position = { 5, 5}, .init_angle = AZ_DEG2RAD(180),
    .bounding_radius = 3.5, .impact_damage = 2.0 },
  { .init_position = { 5, 0}, .init_angle = AZ_DEG2RAD(90),
    .bounding_radius = 3.0, .impact_damage = 2.0 }
};

static const az_vector_t trapdoor_hinge_vertices[] = {
  {-10, 8}, {-10, -8}, {0, -8}, {5, -5}, {7, 0}, {5, 5}, {0, 8}
};
static const az_vector_t trapdoor_door_vertices[] = {
  {10, -95}, {10, 10}, {-2, -2}, {4, -89}
};
static az_component_data_t trapdoor_components[] = {
  { .polygon = AZ_INIT_POLYGON(trapdoor_door_vertices),
    .immunities = ~AZ_DMGF_HYPER_ROCKET }
};

static const az_vector_t swooper_vertices[] = {
  {8, 0}, {7, 3}, {0, 12}, {-10, 8}, {-16, 6}, {-14, 0}, {-16, -6},
  {-10, -8}, {0, -12}, {7, -3}, {8, 0}
};

static const az_vector_t ice_crawler_body_vertices[] = {
  {8, 0}, {-2, 15}, {-16, 10}, {-18, 0}, {-16, -10}, {-2, -15}
};
static const az_vector_t ice_crawler_shell_vertices[] = {
  {17, -4}, {11, 2}, {14, 12}, {6, 14}, {5, 17}, {-5, 18},
  {-14, 12}, {-16, 0}, {-13, -13}, {-2, -18}, {9, -16}, {10, -9}
};
static az_component_data_t ice_crawler_components[] = {
  { .polygon = AZ_INIT_POLYGON(ice_crawler_shell_vertices),
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED | AZ_DMGF_FREEZE),
    .impact_damage = 15.0 }
};

static const az_vector_t oth_crab_vertices[] = {
  {0, 20}, {-5, 16}, {-25, 22}, {-10, 11}, {-15, 7},
  {-15, -7}, {-10, -11}, {-25, -22}, {-5, -16}, {0, -20},
  {5, -16.66}, {22, -25}, {41, -12}, {24, -17}, {15, -10},
  {15, 10}, {24, 17}, {41, 12}, {22, 25}, {5, 16.66}
};

static const az_vector_t gun_sensor_main_vertices[] = {
  {7, 15}, {-7, 15}, {-15, 7}, {-15, -7},
  {-7, -15}, {7, -15}, {15, -7}, {15, 7}
};
static const az_vector_t gun_sensor_casing_vertices[] = {
  {2, 0}, {10, 15}, {10, 20}, {-20, 20}, {-20, -20}, {10, -20}, {10, -15}
};
static az_component_data_t gun_sensor_components[] = {
  { .polygon = AZ_INIT_POLYGON(gun_sensor_casing_vertices), .immunities = ~0 }
};

static const az_vector_t security_drone_cannon_vertices[] = {
  {30, 10}, {-10.66, 10}, {-16, 15}, {-24, 0}, {-16, -15}, {-10.66, -10},
  {30, -10}, {30, -2}, {0, -2}, {0, 2}, {30, 2}
};
static az_component_data_t security_drone_components[] = {
  { .polygon = AZ_INIT_POLYGON(security_drone_cannon_vertices),
    .impact_damage = 10.0 }
};

static const az_vector_t small_truck_vertices[] = {
  {32, 12}, {10, 20}, {10, 14}, {-30, 14},
  {-30, -14}, {10, -14}, {10, -20}, {32, -12}
};

static const az_vector_t heat_ray_vertices[] = {
  {0, -13.6}, {1, -13.9}, {3.4, -13.9}, {5.5, -13}, {7.4, -11.5}, {8.5, -9.5},
  {9, -7}, {15, -7}, {15, 7}, {0, 7}, {0, 15}, {-10, 20}, {-10, -20}, {0, -15}
};

static const az_vector_t nuclear_mine_vertices[] = {
  {11, -1.5}, {18, -1.5}, {18, 1.5}, {11, 1.5}, {6, 10.3923}, {-4.2, 10.2763},
  {-7.7, 16.3385}, {-10.299, 14.8385}, {-6.79904, 8.77628}, {-12, 0},
  {-6.79904, -8.77628}, {-10.299, -14.8385}, {-7.7, -16.3385},
  {-4.2, -10.2763}, {6, -10.3923}
};

static const az_vector_t beam_wall_vertices[] = {
  {52, 15}, {-52, 15}, {-52, 10}, {-50, 10}, {-50, -10}, {-52, -10},
  {-52, -15}, {52, -15}, {52, -10}, {50, -10}, {50, 10}, {52, 10}
};

static const az_vector_t flyer_vertices[] = {
  {10, 2}, {7.5, 4}, {5, 4.66}, {-5, 2.66}, {-7.5, 1.33},
  {-7.5, -1.33}, {-5, -2.66}, {5, -4.66}, {7.5, -4}, {10, -2}
};

// TODO: Update the various Forcefiend component polygons once we have real
//   graphics for the Forcefiend.
static const az_vector_t forcefiend_left_pincer_vertices[] = {
  {0, 0}, {30, 0}, {15, 15}
};
static const az_vector_t forcefiend_right_pincer_vertices[] = {
  {0, 0}, {15, -15}, {30, 0}
};
static const az_vector_t forcefiend_upper_arm_vertices[] = {
  {0, 0}, {20, -5}, {40, 0}, {20, 5}
};
static const az_vector_t forcefiend_lower_arm_vertices[] = {
  {0, 0}, {17, -5}, {34, 0}, {17, 5}
};
static const az_vector_t forcefiend_left_claw_vertices[] = {
  {0, 0}, {20, 5}, {40, 0}, {20, 15}
};
static const az_vector_t forcefiend_right_claw_vertices[] = {
  {0, 0}, {20, -15}, {40, 0}, {20, -5}
};
static const az_vector_t forcefiend_segment0_vertices[] = {
  {20, 0}, {5, 15}, {-5, 15}, {-20, 0}, {-5, -15}, {5, -15}
};
static const az_vector_t forcefiend_segment1_vertices[] = {
  {20, 0}, {6, 14}, {-6, 14}, {-20, 0}, {-6, -14}, {6, -14}
};
static const az_vector_t forcefiend_segment2_vertices[] = {
  {20, 0}, {7, 13}, {-8, 12}, {-20, 0}, {-8, -12}, {7, -13}
};
static const az_vector_t forcefiend_segment3_vertices[] = {
  {20, 0}, {12, 8}, {-15, 5}, {-20, 0}, {-15, -5}, {12, -8}
};
static const az_vector_t forcefiend_stinger_vertices[] = {
  {14, 5}, {-20, 0}, {14, -5}
};
static az_component_data_t forcefiend_components[] = {
  // Pincers:
  { .polygon = AZ_INIT_POLYGON(forcefiend_left_pincer_vertices),
    .init_position = {15, 0}, .immunities = ~0, .impact_damage = 10.0 },
  { .polygon = AZ_INIT_POLYGON(forcefiend_right_pincer_vertices),
    .init_position = {15, 0}, .immunities = ~0, .impact_damage = 10.0 },
  // Left arm:
  { .polygon = AZ_INIT_POLYGON(forcefiend_upper_arm_vertices),
    .immunities = (AZ_DMGF_FREEZE | AZ_DMGF_CPLUS | AZ_DMGF_REACTIVE),
    .init_angle = AZ_DEG2RAD(120), .impact_damage = 4.0 },
  { .polygon = AZ_INIT_POLYGON(forcefiend_lower_arm_vertices),
    .init_position = {-20, 34.64}, .init_angle = AZ_DEG2RAD(180),
    .immunities = (AZ_DMGF_FREEZE | AZ_DMGF_CPLUS | AZ_DMGF_REACTIVE),
    .impact_damage = 4.0 },
  { .polygon = AZ_INIT_POLYGON(forcefiend_left_claw_vertices),
    .init_position = {-54, 34.64}, .init_angle = AZ_DEG2RAD(180),
    .immunities = ~0, .impact_damage = 10.0 },
  // Right arm:
  { .polygon = AZ_INIT_POLYGON(forcefiend_upper_arm_vertices),
    .immunities = (AZ_DMGF_FREEZE | AZ_DMGF_CPLUS | AZ_DMGF_REACTIVE),
    .init_angle = AZ_DEG2RAD(-120), .impact_damage = 4.0 },
  { .polygon = AZ_INIT_POLYGON(forcefiend_lower_arm_vertices),
    .init_position = {-20, -34.64}, .init_angle = AZ_DEG2RAD(180),
    .immunities = (AZ_DMGF_FREEZE | AZ_DMGF_CPLUS | AZ_DMGF_REACTIVE),
    .impact_damage = 4.0 },
  { .polygon = AZ_INIT_POLYGON(forcefiend_right_claw_vertices),
    .init_position = {-54, -34.64}, .init_angle = AZ_DEG2RAD(180),
    .immunities = ~0, .impact_damage = 10.0 },
  // Tail:
  { .polygon = AZ_INIT_POLYGON(forcefiend_segment1_vertices),
    .immunities = (AZ_DMGF_FREEZE | AZ_DMGF_CPLUS | AZ_DMGF_REACTIVE),
    .init_position = {-30, 0}, .impact_damage = 4.0 },
  { .polygon = AZ_INIT_POLYGON(forcefiend_segment2_vertices),
    .immunities = (AZ_DMGF_FREEZE | AZ_DMGF_CPLUS | AZ_DMGF_REACTIVE),
    .init_position = {-60, 0}, .impact_damage = 4.0 },
  { .polygon = AZ_INIT_POLYGON(forcefiend_segment3_vertices),
    .immunities = (AZ_DMGF_FREEZE | AZ_DMGF_CPLUS | AZ_DMGF_REACTIVE),
    .init_position = {-90, 0}, .impact_damage = 4.0 },
  { .polygon = AZ_INIT_POLYGON(forcefiend_stinger_vertices),
    .init_position = {-120, 0}, .immunities = ~0, .impact_damage = 10.0 }
};

static az_vector_t stalk_plant_base_vertices[] = {
  {14, 4}, {8, 11}, {0, 14}, {0, -14}, {8, -11}, {14, -4}
};
static const az_vector_t stalk_plant_left_pincer_vertices[] = {
  {23, 0}, {26,  2}, {23,  8}, {20,  11}, {9,  14}, {-5, 10}, {-8,  6}, {-9, 0}
};
static const az_vector_t stalk_plant_right_pincer_vertices[] = {
  {23, 0}, {26, -2}, {23, -7}, {19, -10}, {7, -13}, {-6, -9}, {-8, -5}, {-9, 0}
};
static az_vector_t stalk_segment_vertices[] = {
  {15, 0}, {12, 4}, {-12, 4}, {-15, 0}, {-12, -4}, {12, -4}
};
static az_component_data_t stalk_plant_components[] = {
  // Base:
  { .polygon = AZ_INIT_POLYGON(stalk_plant_base_vertices),
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED | AZ_DMGF_FREEZE | \
                   AZ_DMGF_PIERCE | AZ_DMGF_ROCKET),
    .init_position = {-190, 0}, .impact_damage = 4.0 },
  // Pincers:
  { .polygon = AZ_INIT_POLYGON(stalk_plant_left_pincer_vertices),
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED), .impact_damage = 12.0 },
  { .polygon = AZ_INIT_POLYGON(stalk_plant_right_pincer_vertices),
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED), .impact_damage = 12.0 },
  // Stalk:
#define STALK_SEGMENT(x) \
  { .polygon = AZ_INIT_POLYGON(stalk_segment_vertices), \
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED | AZ_DMGF_FREEZE | \
                   AZ_DMGF_PIERCE | AZ_DMGF_ROCKET), \
    .init_position = {x, 0}, .impact_damage = 4.0 }

  STALK_SEGMENT(-10), STALK_SEGMENT(-30), STALK_SEGMENT(-50),
  STALK_SEGMENT(-70), STALK_SEGMENT(-90), STALK_SEGMENT(-110),
  STALK_SEGMENT(-130), STALK_SEGMENT(-150), STALK_SEGMENT(-170)
#undef STALK_SEGMENT
};

static az_vector_t copter_vertices[] = {
  {14, 21}, {-14, 21}, {-14, -21}, {14, -21}
};

static az_vector_t boss_door_body_vertices[] = {
  {-20, -50}, {-3, -41}, {7, -24}, {7, 24}, {-3, 41}, {-20, 50}
};
static az_vector_t boss_door_eyelid_vertices[] = {
  {0, 0}, {22, 0}, {21.6658, 3.82026}, {20.6732, 7.52444}, {19.0526, 11},
  {16.853, 14.1413}, {14.1413, 16.853}, {11, 19.0526}, {7.52444, 20.6732},
  {3.82026, 21.6658}, {0, 22}
};
static az_component_data_t boss_door_components[] = {
  // Eye:
  { .bounding_radius = 20.0, .impact_damage = 12.0,
    .immunities = (AZ_DMGF_FREEZE | AZ_DMGF_CPLUS) },
  // Eyelids:
  { .polygon = AZ_INIT_POLYGON(boss_door_eyelid_vertices),
    .immunities = ~0, .impact_damage = 10.0, .init_angle = AZ_DEG2RAD(0) },
  { .polygon = AZ_INIT_POLYGON(boss_door_eyelid_vertices),
    .immunities = ~0, .impact_damage = 10.0, .init_angle = AZ_DEG2RAD(-90) }
};

static const az_vector_t mini_zipper_vertices[] = {
  {14, 2.1}, {10.5, 4.2}, {7, 4.9}, {-7, 2.8}, {-10.5, 1.4}, {-10.5, -1.4},
  {-7, -2.8}, {7, -4.9}, {10.5, -4.2}, {14, -2.1}
};

static const az_vector_t spined_crawler_vertices[] = {
  {-14.5, -11}, {-8, -16}, {-1.5, -13.8564}, {3.25833, -8}, {5, 0},
  {3.25833, 8}, {-1.5, 13.8564}, {-8, 16}, {-14.5, 11}, {-18, 0}
};

static az_baddie_data_t baddie_datas[] = {
  [AZ_BAD_MARKER] = {
    .max_health = 1000000.0, .death_sound = AZ_SND_KILL_TURRET,
    .properties = (AZ_BADF_INCORPOREAL | AZ_BADF_NO_HOMING_BEAM |
                   AZ_BADF_NO_HOMING_PROJ),
    .main_body = { .bounding_radius = 20.0 }
  },
  [AZ_BAD_TURRET] = {
    .max_health = 12.0, .overall_bounding_radius = 30.5,
    .color = {160, 160, 160, 255}, .death_sound = AZ_SND_KILL_TURRET,
    .potential_pickups = (AZ_PUPF_NOTHING | AZ_PUPF_SMALL_SHIELDS |
                          AZ_PUPF_ROCKETS),
    .main_body = { .polygon = AZ_INIT_POLYGON(turret_vertices),
                   .impact_damage = 10.0 },
    DECL_COMPONENTS(turret_components)
  },
  [AZ_BAD_ZIPPER] = {
    .max_health = 20.0, .color = {0, 255, 0, 255},
    .death_sound = AZ_SND_KILL_DRAGONFLY, .death_style = AZ_DEATH_EMBERS,
    .potential_pickups = AZ_PUPF_ALL, .properties = AZ_BADF_BOUNCE_PERP,
    .main_body = { .polygon = AZ_INIT_POLYGON(zipper_vertices),
                   .impact_damage = 10.0 }
  },
  [AZ_BAD_BOUNCER] = {
    .max_health = 5.0, .color = {255, 0, 0, 255},
    .death_sound = AZ_SND_KILL_TURRET, .death_style = AZ_DEATH_EMBERS,
    .potential_pickups = AZ_PUPF_ALL,
    .main_body = { .bounding_radius = 15.0, .impact_damage = 10.0 }
  },
  [AZ_BAD_ATOM] = {
    .max_health = 15.0, .overall_bounding_radius = 40.0,
    .potential_pickups = AZ_PUPF_ALL, .color = {64, 192, 64, 255},
    .death_sound = AZ_SND_KILL_ATOM,
    .main_body = { .bounding_radius = 10.0, .impact_damage = 10.0 },
    DECL_COMPONENTS(atom_components)
  },
  [AZ_BAD_SPINER] = {
    .max_health = 10.0, .color = {128, 255, 0, 255},
    .death_sound = AZ_SND_KILL_TURRET, .death_style = AZ_DEATH_EMBERS,
    .potential_pickups = AZ_PUPF_ALL,
    .main_body = { .bounding_radius = 18.0, .impact_damage = 20.0 }
  },
  [AZ_BAD_BOX] = {
    .max_health = 8.0,
    .potential_pickups = ~(AZ_PUPF_NOTHING | AZ_PUPF_LARGE_SHIELDS),
    .color = {160, 160, 160, 255}, .death_sound = AZ_SND_KILL_TURRET,
    .main_body = { .polygon = AZ_INIT_POLYGON(box_vertices),
                   .immunities = AZ_DMGF_FREEZE, .impact_damage = 0.0 }
  },
  [AZ_BAD_ARMORED_BOX] = {
    .max_health = 10.0,
    .potential_pickups = ~(AZ_PUPF_NOTHING | AZ_PUPF_SMALL_SHIELDS),
    .color = {80, 80, 160, 255}, .death_sound = AZ_SND_KILL_TURRET,
    .main_body = { .polygon = AZ_INIT_POLYGON(box_vertices),
                   .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED |
                                  AZ_DMGF_FREEZE | AZ_DMGF_BEAM),
                   .impact_damage = 0.0 }
  },
  [AZ_BAD_CLAM] = {
    .max_health = 6.0, .overall_bounding_radius = 30.0,
    .potential_pickups = AZ_PUPF_ALL, .color = {128, 0, 255, 255},
    .death_sound = AZ_SND_KILL_TURRET,
    .main_body = { .bounding_radius = 8.0, .impact_damage = 10.0 },
    DECL_COMPONENTS(clam_components)
  },
  [AZ_BAD_NIGHTBUG] = {
    .max_health = 8.0, .color = {192, 96, 0, 255},
    .death_sound = AZ_SND_KILL_DRAGONFLY, .death_style = AZ_DEATH_EMBERS,
    .potential_pickups = ~AZ_PUPF_LARGE_SHIELDS,
    .main_body = { .polygon = AZ_INIT_POLYGON(nightbug_vertices),
                   .impact_damage = 10.0 }
  },
  [AZ_BAD_SPINE_MINE] = {
    .max_health = 5.0, .potential_pickups = ~AZ_PUPF_LARGE_SHIELDS,
    .color = {128, 255, 0, 255}, .death_sound = AZ_SND_KILL_TURRET,
    .main_body = { .bounding_radius = 9.0, .impact_damage = 20.0 }
  },
  [AZ_BAD_BROKEN_TURRET] = {
    .max_health = 5.0, .overall_bounding_radius = 30.5,
    .potential_pickups = (AZ_PUPF_NOTHING | AZ_PUPF_SMALL_SHIELDS |
                          AZ_PUPF_ROCKETS),
    .color = {160, 160, 160, 255}, .death_sound = AZ_SND_KILL_TURRET,
    .main_body = { .polygon = AZ_INIT_POLYGON(turret_vertices),
                   .impact_damage = 5.0 },
    DECL_COMPONENTS(broken_turret_components)
  },
  [AZ_BAD_ZENITH_CORE] = {
    .max_health = 1000.0, .color = {255, 0, 255, 255},
    .death_sound = AZ_SND_KILL_TURRET, .death_style = AZ_DEATH_SHARDS,
    .main_body = { .polygon = AZ_INIT_POLYGON(zenith_core_vertices),
                   .immunities = (AZ_DMGF_FREEZE | AZ_DMGF_CPLUS |
                                  AZ_DMGF_REACTIVE) }
  },
  [AZ_BAD_ARMORED_TURRET] = {
    .max_health = 12.0, .overall_bounding_radius = 30.5,
    .potential_pickups = ~AZ_PUPF_LARGE_SHIELDS, .color = {80, 80, 160, 255},
    .death_sound = AZ_SND_KILL_TURRET,
    .main_body = { .polygon = AZ_INIT_POLYGON(turret_vertices),
                   .immunities = AZ_DMGF_NORMAL, .impact_damage = 10.0 },
    DECL_COMPONENTS(armored_turret_components)
  },
  [AZ_BAD_DRAGONFLY] = {
    .max_health = 8.0, .color = {255, 64, 0, 255},
    .potential_pickups = (AZ_PUPF_NOTHING | AZ_PUPF_SMALL_SHIELDS |
                          AZ_PUPF_MEDIUM_SHIELDS),
    .death_sound = AZ_SND_KILL_DRAGONFLY, .death_style = AZ_DEATH_EMBERS,
    .main_body = { .polygon = AZ_INIT_POLYGON(dragonfly_vertices),
                   .impact_damage = 8.0 }
  },
  [AZ_BAD_CAVE_CRAWLER] = {
    .max_health = 7.0,
    .potential_pickups = ~AZ_PUPF_LARGE_SHIELDS,
    .color = {128, 0, 128, 255}, .death_sound = AZ_SND_KILL_TURRET,
    .main_body = { .polygon = AZ_INIT_POLYGON(cave_crawler_vertices),
                   .impact_damage = 10.0 }
  },
  [AZ_BAD_CRAWLING_TURRET] = {
    .max_health = 12.0, .overall_bounding_radius = 30.5,
    .potential_pickups = ~AZ_PUPF_LARGE_SHIELDS,
    .color = {160, 160, 160, 255}, .death_sound = AZ_SND_KILL_TURRET,
    .main_body = { .polygon = AZ_INIT_POLYGON(turret_vertices),
                   .impact_damage = 10.0 },
    DECL_COMPONENTS(crawling_turret_components)
  },
  [AZ_BAD_STINGER] = {
    .max_health = 8.0, .color = {192, 192, 0, 255},
    .death_sound = AZ_SND_KILL_DRAGONFLY, .death_style = AZ_DEATH_EMBERS,
    .potential_pickups = AZ_PUPF_ALL,
    .main_body = { .polygon = AZ_INIT_POLYGON(dragonfly_vertices),
                   .impact_damage = 5.0 }
  },
  [AZ_BAD_BEAM_SENSOR] = {
    .max_health = 1000000.0, .overall_bounding_radius = 30.0,
    .color = {160, 160, 160, 255}, .death_sound = AZ_SND_KILL_TURRET,
    .properties = AZ_BADF_NO_HOMING_PROJ,
    .main_body = { .bounding_radius = 15.0, .immunities = ~AZ_DMGF_BEAM },
    DECL_COMPONENTS(beam_sensor_components)
  },
  [AZ_BAD_ROCKWYRM] = {
    .max_health = 150.0, .overall_bounding_radius = 500.0,
    .color = {192, 255, 128, 255},
    .death_sound = AZ_SND_KILL_TURRET, .death_style = AZ_DEATH_EMBERS,
    .main_body = { .bounding_radius = 24.0, .impact_damage = 5.0,
                   .immunities = (AZ_DMGF_FREEZE | AZ_DMGF_CPLUS |
                                  AZ_DMGF_REACTIVE) },
    DECL_COMPONENTS(rockwyrm_components)
  },
  [AZ_BAD_WYRM_EGG] = {
    .max_health = 2.0,
    .potential_pickups = (AZ_PUPF_NOTHING | AZ_PUPF_SMALL_SHIELDS),
    .color = {128, 255, 255, 255}, .death_sound = AZ_SND_KILL_TURRET,
    .main_body = { .bounding_radius = 10.0 }
  },
  [AZ_BAD_WYRMLING] = {
    .max_health = 0.1, .overall_bounding_radius = 30.0,
    .potential_pickups = ~AZ_PUPF_LARGE_SHIELDS, .color = {192, 255, 128, 255},
    .death_sound = AZ_SND_KILL_TURRET, .death_style = AZ_DEATH_EMBERS,
    .main_body = { .bounding_radius = 5.0, .impact_damage = 3.0,
                   .immunities = AZ_DMGF_FREEZE },
    DECL_COMPONENTS(wyrmling_components)
  },
  [AZ_BAD_TRAPDOOR] = {
    .max_health = 3.0, .overall_bounding_radius = 95.6,
    .potential_pickups = (AZ_PUPF_LARGE_SHIELDS | AZ_PUPF_ROCKETS |
                          AZ_PUPF_BOMBS),
    .color = {160, 160, 160, 255}, .death_sound = AZ_SND_KILL_TURRET,
    .main_body = { .polygon = AZ_INIT_POLYGON(trapdoor_hinge_vertices),
                   .immunities = ~(AZ_DMGF_FREEZE | AZ_DMGF_HYPER_ROCKET) },
    DECL_COMPONENTS(trapdoor_components)
  },
  [AZ_BAD_SWOOPER] = {
    .max_health = 5.0, .potential_pickups = ~AZ_PUPF_LARGE_SHIELDS,
    .color = {0, 128, 0, 255}, .death_sound = AZ_SND_KILL_TURRET,
    .main_body = { .polygon = AZ_INIT_POLYGON(swooper_vertices),
                   .impact_damage = 8.0 }
  },
  [AZ_BAD_ICE_CRAWLER] = {
    .max_health = 7.0, .overall_bounding_radius = 18.9,
    .potential_pickups = ~AZ_PUPF_LARGE_SHIELDS,
    .color = {0, 128, 128, 255}, .death_sound = AZ_SND_KILL_TURRET,
    .main_body = { .polygon = AZ_INIT_POLYGON(ice_crawler_body_vertices),
                   .impact_damage = 10.0, .immunities = AZ_DMGF_FREEZE },
    DECL_COMPONENTS(ice_crawler_components)
  },
  [AZ_BAD_BEAM_TURRET] = {
    .max_health = 20.0, .overall_bounding_radius = 30.5,
    .potential_pickups = AZ_PUPF_ALL, .color = {80, 160, 120, 255},
    .death_sound = AZ_SND_KILL_TURRET,
    .main_body = { .polygon = AZ_INIT_POLYGON(turret_vertices),
                   .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_BEAM),
                   .impact_damage = 10.0 },
    DECL_COMPONENTS(beam_turret_components)
  },
  [AZ_BAD_OTH_CRAB_1] = {
    .max_health = 50.0, .potential_pickups = ~AZ_PUPF_NOTHING,
    .death_sound = AZ_SND_KILL_TURRET, .death_style = AZ_DEATH_OTH,
    .main_body = { .polygon = AZ_INIT_POLYGON(oth_crab_vertices),
                   .impact_damage = 20.0 }
  },
  [AZ_BAD_OTH_ORB] = {
    .max_health = 50.0,
    .death_sound = AZ_SND_KILL_TURRET, .death_style = AZ_DEATH_OTH,
    .main_body = { .bounding_radius = 19.0, .impact_damage = 20.0 }
  },
  [AZ_BAD_OTH_SNAPDRAGON] = {
    .max_health = 60.0,
    .death_sound = AZ_SND_KILL_TURRET, .death_style = AZ_DEATH_OTH,
    .main_body = { .bounding_radius = 30.0, .impact_damage = 6.0 }
  },
  [AZ_BAD_OTH_RAZOR] = {
    .max_health = 0.1, .color = {255, 255, 255, 255},
    .death_sound = AZ_SND_KILL_TURRET, .death_style = AZ_DEATH_OTH,
    .potential_pickups = (AZ_PUPF_NOTHING | AZ_PUPF_SMALL_SHIELDS),
    .properties = AZ_BADF_KAMIKAZE,
    .main_body = { .bounding_radius = 10.0, .impact_damage = 3.0 }
  },
  [AZ_BAD_GUN_SENSOR] = {
    .max_health = 1000000.0, .overall_bounding_radius = 30.0,
    .color = {160, 160, 160, 255}, .death_sound = AZ_SND_KILL_TURRET,
    .main_body = { .polygon = AZ_INIT_POLYGON(gun_sensor_main_vertices),
                   .immunities = (AZ_DMGF_FREEZE | AZ_DMGF_CPLUS |
                                  AZ_DMGF_REACTIVE) },
    DECL_COMPONENTS(gun_sensor_components)
  },
  [AZ_BAD_SECURITY_DRONE] = {
    .max_health = 20.0, .overall_bounding_radius = 31.7,
    .potential_pickups = ~AZ_PUPF_SMALL_SHIELDS,
    .color = {160, 160, 160, 255}, .death_sound = AZ_SND_KILL_TURRET,
    .main_body = { .polygon = AZ_INIT_POLYGON(turret_vertices),
                   .impact_damage = 10.0 },
    DECL_COMPONENTS(security_drone_components)
  },
  [AZ_BAD_SMALL_TRUCK] = {
    .max_health = 40.0,
    .potential_pickups = ~AZ_PUPF_SMALL_SHIELDS,
    .color = {160, 160, 160, 255}, .death_sound = AZ_SND_KILL_TURRET,
    .main_body = { .polygon = AZ_INIT_POLYGON(small_truck_vertices),
                   .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED |
                                  AZ_DMGF_PIERCE | AZ_DMGF_BEAM |
                                  AZ_DMGF_REACTIVE), .impact_damage = 8.0 }
  },
  [AZ_BAD_HEAT_RAY] = {
    .max_health = 40.0,
    .potential_pickups = ~AZ_PUPF_LARGE_SHIELDS,
    .color = {160, 160, 160, 255}, .death_sound = AZ_SND_KILL_TURRET,
    .main_body = { .polygon = AZ_INIT_POLYGON(heat_ray_vertices),
                   .immunities = ~(AZ_DMGF_CPLUS | AZ_DMGF_HYPER_ROCKET) }
  },
  [AZ_BAD_NUCLEAR_MINE] = {
    .max_health = 7.0, .potential_pickups = AZ_PUPF_ALL,
    .color = {160, 160, 128, 255}, .death_sound = AZ_SND_KILL_TURRET,
    .main_body = { .polygon = AZ_INIT_POLYGON(nuclear_mine_vertices),
                   .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_BOMB) }
  },
  [AZ_BAD_BEAM_WALL] = {
    .max_health = 50.0, .potential_pickups = AZ_PUPF_NOTHING,
    .color = {160, 160, 160, 255}, .death_sound = AZ_SND_KILL_TURRET,
    .properties = (AZ_BADF_NO_HOMING_BEAM | AZ_BADF_NO_HOMING_PROJ),
    .main_body = { .polygon = AZ_INIT_POLYGON(beam_wall_vertices),
                   .immunities = ~AZ_DMGF_BEAM }
  },
  [AZ_BAD_SPARK] = {
    .max_health = 1000000.0, .death_sound = AZ_SND_KILL_TURRET,
    .properties = (AZ_BADF_INCORPOREAL | AZ_BADF_NO_HOMING_BEAM |
                   AZ_BADF_NO_HOMING_PROJ),
    .main_body = { .bounding_radius = 10.0 }
  },
  [AZ_BAD_FLYER] = {
    .max_health = 1.0, .color = {255, 64, 0, 255},
    .death_sound = AZ_SND_KILL_DRAGONFLY, .death_style = AZ_DEATH_EMBERS,
    .potential_pickups = ~AZ_PUPF_LARGE_SHIELDS,
    .properties = AZ_BADF_KAMIKAZE,
    .main_body = { .polygon = AZ_INIT_POLYGON(flyer_vertices),
                   .impact_damage = 6.0 }
  },
  [AZ_BAD_ARMORED_ZIPPER] = {
    .max_health = 24.0, .color = {128, 160, 128, 255},
    .death_sound = AZ_SND_KILL_DRAGONFLY, .death_style = AZ_DEATH_SHARDS,
    .potential_pickups = AZ_PUPF_ALL, .properties = AZ_BADF_BOUNCE_PERP,
    .main_body = { .polygon = AZ_INIT_POLYGON(zipper_vertices),
                   .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED),
                   .impact_damage = 10.0 }
  },
  [AZ_BAD_FORCEFIEND] = {
    .max_health = 450.0, .color = {192, 128, 255, 255},
    .overall_bounding_radius = 150.0,
    .death_sound = AZ_SND_KILL_TURRET, .death_style = AZ_DEATH_EMBERS,
    .main_body = { .polygon = AZ_INIT_POLYGON(forcefiend_segment0_vertices),
                   .immunities = (AZ_DMGF_FREEZE | AZ_DMGF_CPLUS |
                                  AZ_DMGF_REACTIVE),
                   .impact_damage = 4.0 },
    DECL_COMPONENTS(forcefiend_components)
  },
  [AZ_BAD_STALK_PLANT] = {
    .max_health = 14.0, .overall_bounding_radius = 250.0,
    .potential_pickups = AZ_PUPF_ALL, .color = {32, 128, 0, 255},
    .death_sound = AZ_SND_KILL_TURRET,
    .main_body = { .bounding_radius = 8.0, .impact_damage = 10.0 },
    DECL_COMPONENTS(stalk_plant_components)
  },
  [AZ_BAD_COPTER] = {
    .max_health = 40.0,
    .potential_pickups = ~AZ_PUPF_SMALL_SHIELDS,
    .color = {160, 160, 160, 255}, .death_sound = AZ_SND_KILL_TURRET,
    .main_body = { .polygon = AZ_INIT_POLYGON(copter_vertices),
                   .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED |
                                  AZ_DMGF_PIERCE | AZ_DMGF_BEAM |
                                  AZ_DMGF_REACTIVE), .impact_damage = 8.0 }
  },
  [AZ_BAD_URCHIN] = {
    .max_health = 5.0, .potential_pickups = ~AZ_PUPF_LARGE_SHIELDS,
    .color = {128, 255, 0, 255}, .death_sound = AZ_SND_KILL_TURRET,
    .main_body = { .bounding_radius = 9.0, .impact_damage = 20.0 }
  },
  [AZ_BAD_BOSS_DOOR] = {
    .max_health = 24.0, .overall_bounding_radius = 55.0,
    .potential_pickups = AZ_PUPF_LARGE_SHIELDS,
    .color = {160, 160, 160, 255}, .death_sound = AZ_SND_KILL_TURRET,
    .main_body = { .polygon = AZ_INIT_POLYGON(boss_door_body_vertices),
                   .impact_damage = 10.0, .immunities = ~0 },
    DECL_COMPONENTS(boss_door_components)
  },
  [AZ_BAD_ROCKET_TURRET] = {
    .max_health = 20.0, .overall_bounding_radius = 30.5,
    .potential_pickups = AZ_PUPF_ALL, .color = {160, 80, 120, 255},
    .death_sound = AZ_SND_KILL_TURRET,
    .main_body = { .polygon = AZ_INIT_POLYGON(turret_vertices),
                   .immunities = AZ_DMGF_NORMAL, .impact_damage = 10.0 },
    DECL_COMPONENTS(rocket_turret_components)
  },
  [AZ_BAD_MINI_ARMORED_ZIPPER] = {
    .max_health = 4.0, .color = {128, 160, 128, 255},
    .death_sound = AZ_SND_KILL_DRAGONFLY, .death_style = AZ_DEATH_SHARDS,
    .potential_pickups = AZ_PUPF_ALL, .properties = AZ_BADF_BOUNCE_PERP,
    .main_body = { .polygon = AZ_INIT_POLYGON(mini_zipper_vertices),
                   .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED),
                   .impact_damage = 10.0 }
  },
  [AZ_BAD_OTH_CRAB_2] = {
    .max_health = 60.0, .potential_pickups = AZ_PUPF_LARGE_SHIELDS,
    .death_sound = AZ_SND_KILL_TURRET, .death_style = AZ_DEATH_OTH,
    .main_body = { .polygon = AZ_INIT_POLYGON(oth_crab_vertices),
                   .impact_damage = 20.0 }
  },
  [AZ_BAD_SPINED_CRAWLER] = {
    .max_health = 9.0, .potential_pickups = AZ_PUPF_ALL,
    .color = {32, 128, 64, 255}, .death_sound = AZ_SND_KILL_TURRET,
    .main_body = { .polygon = AZ_INIT_POLYGON(spined_crawler_vertices),
                   .impact_damage = 12.0 }
  },
  [AZ_BAD_DEATH_RAY] = {
    .max_health = 100.0,
    .potential_pickups = AZ_PUPF_ALL,
    .color = {160, 160, 160, 255}, .death_sound = AZ_SND_KILL_TURRET,
    .main_body = { .polygon = AZ_INIT_POLYGON(heat_ray_vertices),
                   .immunities = ~AZ_DMGF_CPLUS }
  }
};

AZ_STATIC_ASSERT(AZ_ARRAY_SIZE(baddie_datas) == AZ_NUM_BADDIE_KINDS + 1);

/*===========================================================================*/

static double polygon_bounding_radius(az_polygon_t polygon) {
  double radius = 0.0;
  for (int i = 0; i < polygon.num_vertices; ++i) {
    radius = fmax(radius, az_vnorm(polygon.vertices[i]));
  }
  return radius + 0.01; // small safety margin
}

static void init_component_data(az_component_data_t *component) {
  if (component->polygon.num_vertices > 0) {
    assert(component->bounding_radius == 0.0);
    component->bounding_radius =
      polygon_bounding_radius(component->polygon);
  } else assert(component->bounding_radius > 0.0);
}

static bool baddie_data_initialized = false;

void az_init_baddie_datas(void) {
  assert(!baddie_data_initialized);
  bool skipped = false;
  AZ_ARRAY_LOOP(data, baddie_datas) {
    // Skip the first entry in the array (which is for AZ_BAD_NOTHING)
    if (!skipped) {
      skipped = true;
      continue;
    }
    assert(data->num_components <= AZ_MAX_BADDIE_COMPONENTS);
    // Set bounding radius for all components.
    for (int i = 0; i < data->num_components; ++i) {
      // N.B. We need to cast away the const-ness of the data->components
      // pointer here, so that we can initialize its bounding_radius.  We could
      // avoid needing a const-cast by accessing the component data arrays
      // directly by name (as they are each declared non-const above), but it's
      // far more convenient to go through the data->components pointer.
      az_component_data_t *component =
        (az_component_data_t *)(&data->components[i]);
      init_component_data(component);
    }
    // Set main body bounding radius.
    init_component_data(&data->main_body);
    // Set overall bounding radius.
    if (data->num_components == 0) {
      assert(data->overall_bounding_radius == 0.0);
      data->overall_bounding_radius = data->main_body.bounding_radius;
    }
    // Sanity-check the overall bounding radius.
    assert(data->overall_bounding_radius >= data->main_body.bounding_radius);
    for (int i = 0; i < data->num_components; ++i) {
      assert(data->overall_bounding_radius >=
             data->components[i].bounding_radius);
    }
    // Sanity-check other fields.
    assert(data->max_health > 0.0);
  }
  baddie_data_initialized = true;
}

const az_baddie_data_t *az_get_baddie_data(az_baddie_kind_t kind) {
  assert(baddie_data_initialized);
  assert(kind != AZ_BAD_NOTHING);
  const int data_index = (int)kind;
  assert(0 <= data_index && data_index < AZ_ARRAY_SIZE(baddie_datas));
  return &baddie_datas[data_index];
}

void az_init_baddie(az_baddie_t *baddie, az_baddie_kind_t kind,
                    az_vector_t position, double angle) {
  assert(kind != AZ_BAD_NOTHING);
  const az_uid_t uid = baddie->uid;
  memset(baddie, 0, sizeof(*baddie));
  baddie->kind = kind;
  baddie->data = az_get_baddie_data(kind);
  baddie->uid = uid;
  baddie->position = position;
  baddie->angle = angle;
  baddie->health = baddie->data->max_health;
  for (int i = 0; i < baddie->data->num_components; ++i) {
    assert(i < AZ_ARRAY_SIZE(baddie->components));
    baddie->components[i].position = baddie->data->components[i].init_position;
    baddie->components[i].angle = baddie->data->components[i].init_angle;
  }
}

/*===========================================================================*/

static bool point_touches_component(
    const az_component_data_t *component, az_vector_t point) {
  if (component->polygon.num_vertices > 0) {
    return (az_vwithin(point, AZ_VZERO, component->bounding_radius) &&
            az_polygon_contains(component->polygon, point));
  } else {
    return az_vwithin(point, AZ_VZERO, component->bounding_radius);
  }
}

bool az_point_touches_baddie(const az_baddie_t *baddie, az_vector_t point) {
  assert(baddie->kind != AZ_BAD_NOTHING);
  const az_baddie_data_t *data = baddie->data;

  // Common case: if point definitely doesn't touch baddie, return early.
  if (!az_vwithin(point, baddie->position, data->overall_bounding_radius)) {
    return false;
  }

  // Calculate point relative to the positioning of the baddie.
  const az_vector_t rel_point =
    az_vrotate(az_vsub(point, baddie->position), -baddie->angle);

  // Check if we hit the main body of the baddie.
  if (point_touches_component(&data->main_body, rel_point)) {
    return true;
  }

  // Now check if we hit any of the baddie's components.
  for (int i = 0; i < data->num_components; ++i) {
    assert(i < AZ_ARRAY_SIZE(baddie->components));
    const az_component_data_t *component = &data->components[i];
    const az_vector_t rel_rel_point =
      az_vrotate(az_vsub(rel_point, baddie->components[i].position),
                 -baddie->components[i].angle);
    if (point_touches_component(component, rel_rel_point)) {
      return true;
    }
  }

  return false;
}

/*===========================================================================*/

static bool circle_touches_component(
    const az_component_data_t *component, az_vector_t position, double angle,
    double circle_radius, az_vector_t circle_center) {
  if (!az_vwithin(position, circle_center,
                  component->bounding_radius + circle_radius)) return false;
  if (component->polygon.num_vertices == 0) return true;
  return az_circle_touches_polygon_trans(component->polygon, position, angle,
                                         circle_radius, circle_center);
}

bool az_circle_touches_baddie(
    const az_baddie_t *baddie, double radius, az_vector_t center,
    const az_component_data_t **component_out) {
  assert(baddie->kind != AZ_BAD_NOTHING);
  // Common case: if circle definitely doesn't touch the baddie, return early.
  if (!az_vwithin(baddie->position, center, radius +
                  baddie->data->overall_bounding_radius)) return false;

  // Semi-common case: cast a ray towards the baddie center, returning the
  // first component hit.
  if (az_ray_hits_baddie(
          baddie, center,
          az_vwithlen(az_vsub(baddie->position, center), radius),
          NULL, NULL, component_out)) return true;

  // Okay, now we have to exhaustively check each component.
  const az_baddie_data_t *data = baddie->data;
  // Calculate center relative to the positioning of the baddie.
  const az_vector_t rel_center =
    az_vrotate(az_vsub(center, baddie->position), -baddie->angle);
  // Check the non-main components first.
  for (int i = 0; i < data->num_components; ++i) {
    const az_component_data_t *component = &data->components[i];
    if (circle_touches_component(
            component, baddie->components[i].position,
            baddie->components[i].angle, radius, rel_center)) {
      if (component_out != NULL) *component_out = component;
      return true;
    }
  }
  // Now check the main body.
  if (circle_touches_component(&data->main_body, AZ_VZERO, 0.0,
                               radius, rel_center)) {
    if (component_out != NULL) *component_out = &data->main_body;
    return true;
  }

  return false;
}

/*===========================================================================*/

static bool ray_hits_component(
    const az_component_data_t *component, az_vector_t position, double angle,
    az_vector_t start, az_vector_t delta, az_vector_t *point_out,
    az_vector_t *normal_out) {
  if (component->polygon.num_vertices > 0) {
    return (az_ray_hits_bounding_circle(start, delta, position,
                                        component->bounding_radius) &&
            az_ray_hits_polygon_trans(component->polygon, position,
                                      angle, start, delta,
                                      point_out, normal_out));
  } else {
    return az_ray_hits_circle(component->bounding_radius, position,
                              start, delta, point_out, normal_out);
  }
}

bool az_ray_hits_baddie(
    const az_baddie_t *baddie, az_vector_t start, az_vector_t delta,
    az_vector_t *point_out, az_vector_t *normal_out,
    const az_component_data_t **component_out) {
  assert(baddie->kind != AZ_BAD_NOTHING);
  const az_baddie_data_t *data = baddie->data;

  // Common case: if ray definitely misses baddie, return early.
  if (!az_ray_hits_bounding_circle(start, delta, baddie->position,
                                   data->overall_bounding_radius)) {
    return false;
  }

  // Calculate start and delta relative to the positioning of the baddie.
  const az_vector_t rel_start = az_vrotate(az_vsub(start, baddie->position),
                                           -baddie->angle);
  az_vector_t rel_delta = az_vrotate(delta, -baddie->angle);
  const az_component_data_t *hit_component = NULL;
  az_vector_t point = AZ_VZERO;

  // Check if we hit the main body of the baddie.
  if (ray_hits_component(&data->main_body, AZ_VZERO, 0.0, rel_start,
                         rel_delta, &point, normal_out)) {
    hit_component = &data->main_body;
    rel_delta = az_vsub(point, rel_start);
  }

  // Now check if we hit any of the baddie's components.
  for (int i = 0; i < data->num_components; ++i) {
    assert(i < AZ_ARRAY_SIZE(baddie->components));
    const az_component_data_t *component = &data->components[i];
    if (ray_hits_component(component, baddie->components[i].position,
                           baddie->components[i].angle, rel_start, rel_delta,
                           &point, normal_out)) {
      hit_component = component;
      rel_delta = az_vsub(point, rel_start);
    }
  }

  // Fix up *point_out and *normal_out and return.
  if (hit_component != NULL) {
    if (point_out != NULL) {
      *point_out = az_vadd(az_vrotate(point, baddie->angle), baddie->position);
    }
    if (normal_out != NULL) {
      *normal_out = az_vrotate(*normal_out, baddie->angle);
    }
    if (component_out != NULL) *component_out = hit_component;
    return true;
  }
  return false;
}

/*===========================================================================*/

static bool circle_hits_component(
    const az_component_data_t *component, az_vector_t position, double angle,
    double radius, az_vector_t start, az_vector_t delta,
    az_vector_t *pos_out, az_vector_t *impact_out) {
  if (component->polygon.num_vertices > 0) {
    return (az_ray_hits_bounding_circle(start, delta, position,
                                        component->bounding_radius + radius) &&
            az_circle_hits_polygon_trans(component->polygon, position, angle,
                                         radius, start, delta,
                                         pos_out, impact_out));
  } else {
    return az_circle_hits_circle(component->bounding_radius, position,
                                 radius, start, delta, pos_out, impact_out);
  }
}

bool az_circle_hits_baddie(
    const az_baddie_t *baddie, double radius, az_vector_t start,
    az_vector_t delta, az_vector_t *pos_out, az_vector_t *impact_out,
    const az_component_data_t **component_out) {
  assert(baddie->kind != AZ_BAD_NOTHING);
  const az_baddie_data_t *data = baddie->data;

  // Common case: if circle definitely misses baddie, return early.
  if (!az_ray_hits_bounding_circle(start, delta, baddie->position,
                                   data->overall_bounding_radius + radius)) {
    return false;
  }

  // Calculate start and delta relative to the positioning of the baddie.
  const az_vector_t rel_start = az_vrotate(az_vsub(start, baddie->position),
                                           -baddie->angle);
  az_vector_t rel_delta = az_vrotate(delta, -baddie->angle);
  const az_component_data_t *hit_component = NULL;
  az_vector_t pos = AZ_VZERO;

  // Check if we hit the main body of the baddie.
  if (circle_hits_component(&data->main_body, AZ_VZERO, 0.0, radius, rel_start,
                            rel_delta, &pos, impact_out)) {
    hit_component = &data->main_body;
    rel_delta = az_vsub(pos, rel_start);
  }

  // Now check if we hit any of the baddie's components.
  for (int i = 0; i < data->num_components; ++i) {
    assert(i < AZ_ARRAY_SIZE(baddie->components));
    const az_component_data_t *component = &data->components[i];
    if (circle_hits_component(component, baddie->components[i].position,
                              baddie->components[i].angle, radius, rel_start,
                              rel_delta, &pos, impact_out)) {
      hit_component = component;
      rel_delta = az_vsub(pos, rel_start);
    }
  }

  // Fix up *pos_out and *impact_out and return.
  if (hit_component != NULL) {
    if (pos_out != NULL) {
      *pos_out = az_vadd(az_vrotate(pos, baddie->angle), baddie->position);
    }
    if (impact_out != NULL) {
      *impact_out = az_vadd(az_vrotate(*impact_out, baddie->angle),
                            baddie->position);
    }
    if (component_out != NULL) *component_out = hit_component;
    return true;
  }
  return false;
}

/*===========================================================================*/

static bool arc_circle_hits_component(
    const az_component_data_t *component, az_vector_t component_position,
    double component_angle, double circle_radius, az_vector_t circle_start,
    az_vector_t spin_center, double spin_angle,
    double *angle_out, az_vector_t *pos_out, az_vector_t *impact_out) {
  if (component->polygon.num_vertices > 0) {
    return (az_arc_ray_might_hit_bounding_circle(
                circle_start, spin_center, spin_angle, component_position,
                component->bounding_radius + circle_radius) &&
            az_arc_circle_hits_polygon_trans(
                component->polygon, component_position, component_angle,
                circle_radius, circle_start, spin_center, spin_angle,
                angle_out, pos_out, impact_out));
  } else {
    return az_arc_circle_hits_circle(
        component->bounding_radius, component_position, circle_radius,
        circle_start, spin_center, spin_angle, angle_out, pos_out, impact_out);
  }
}

bool az_arc_circle_hits_baddie(
    const az_baddie_t *baddie, double circle_radius,
    az_vector_t start, az_vector_t spin_center, double spin_angle,
    double *angle_out, az_vector_t *pos_out, az_vector_t *impact_out,
    const az_component_data_t **component_out) {
  assert(baddie->kind != AZ_BAD_NOTHING);
  const az_baddie_data_t *data = baddie->data;

  // Common case: if circle definitely misses baddie, return early.
  if (!az_arc_ray_might_hit_bounding_circle(
          start, spin_center, spin_angle, baddie->position,
          data->overall_bounding_radius + circle_radius)) {
    return false;
  }

  // Calculate start and spin_center relative to the positioning of the baddie.
  const az_vector_t rel_start =
    az_vrotate(az_vsub(start, baddie->position), -baddie->angle);
  const az_vector_t rel_spin_center =
    az_vrotate(az_vsub(spin_center, baddie->position), -baddie->angle);
  const az_component_data_t *hit_component = NULL;

  // Check if we hit the main body of the baddie.
  if (arc_circle_hits_component(
          &data->main_body, AZ_VZERO, 0.0, circle_radius, rel_start,
          rel_spin_center, spin_angle, &spin_angle, pos_out, impact_out)) {
    hit_component = &data->main_body;
  }

  // Now check if we hit any of the baddie's components.
  for (int i = 0; i < data->num_components; ++i) {
    assert(i < AZ_ARRAY_SIZE(baddie->components));
    const az_component_data_t *component = &data->components[i];
    if (arc_circle_hits_component(
            component, baddie->components[i].position,
            baddie->components[i].angle, circle_radius, rel_start,
            rel_spin_center, spin_angle, &spin_angle, pos_out, impact_out)) {
      hit_component = component;
    }
  }

  // Fix up *pos_out and *impact_out and return.
  if (hit_component != NULL) {
    if (angle_out != NULL) *angle_out = spin_angle;
    if (pos_out != NULL) {
      *pos_out = az_vadd(az_vrotate(*pos_out, baddie->angle),
                         baddie->position);
    }
    if (impact_out != NULL) {
      *impact_out = az_vadd(az_vrotate(*impact_out, baddie->angle),
                            baddie->position);
    }
    if (component_out != NULL) *component_out = hit_component;
    return true;
  }
  return false;
}

/*===========================================================================*/
