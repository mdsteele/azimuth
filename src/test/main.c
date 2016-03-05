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

#include "test/test.h"

/*===========================================================================*/

int main(int argc, char **argv) {
  RUN_TEST(test_alloc);
  RUN_TEST(test_arc_circle_hits_circle);
  RUN_TEST(test_arc_circle_hits_line);
  RUN_TEST(test_arc_circle_hits_line_segment);
  RUN_TEST(test_arc_circle_hits_point);
  RUN_TEST(test_arc_circle_hits_polygon);
  RUN_TEST(test_arc_circle_hits_polygon_trans);
  RUN_TEST(test_arc_ray_hits_circle);
  RUN_TEST(test_arc_ray_hits_line);
  RUN_TEST(test_arc_ray_hits_line_segment);
  RUN_TEST(test_arc_ray_hits_polygon);
  RUN_TEST(test_arc_ray_hits_polygon_trans);
  RUN_TEST(test_array_size);
  RUN_TEST(test_circle_hits_arc);
  RUN_TEST(test_circle_hits_circle);
  RUN_TEST(test_circle_hits_line);
  RUN_TEST(test_circle_hits_line_segment);
  RUN_TEST(test_circle_hits_point);
  RUN_TEST(test_circle_hits_polygon);
  RUN_TEST(test_circle_hits_polygon_trans);
  RUN_TEST(test_circle_touches_line);
  RUN_TEST(test_circle_touches_line_segment);
  RUN_TEST(test_circle_touches_polygon);
  RUN_TEST(test_circle_touches_polygon_trans);
  RUN_TEST(test_clock_mod);
  RUN_TEST(test_clock_zigzag);
  RUN_TEST(test_color3f);
  RUN_TEST(test_create_sound_data);
  RUN_TEST(test_cubic_bezier_angle);
  RUN_TEST(test_cubic_bezier_arc_length);
  RUN_TEST(test_cubic_bezier_arc_param);
  RUN_TEST(test_cubic_bezier_point);
  RUN_TEST(test_find_knee);
  RUN_TEST(test_hint_matches);
  RUN_TEST(test_hsva_color);
  RUN_TEST(test_is_number_key);
  RUN_TEST(test_lead_target);
  RUN_TEST(test_modulo);
  RUN_TEST(test_mod2pi);
  RUN_TEST(test_paragraph_length);
  RUN_TEST(test_paragraph_read);
  RUN_TEST(test_parse_music);
  RUN_TEST(test_parse_music_instructions);
  RUN_TEST(test_persist_sound);
  RUN_TEST(test_player_flags);
  RUN_TEST(test_player_give_upgrade);
  RUN_TEST(test_player_set_room_visited);
  RUN_TEST(test_player_set_zone_mapped);
  RUN_TEST(test_polygon_contains);
  RUN_TEST(test_polygon_contains_circle);
  RUN_TEST(test_position_visible);
  RUN_TEST(test_prefs_defaults);
  RUN_TEST(test_prefs_missing_values);
  RUN_TEST(test_prefs_save_load);
  RUN_TEST(test_randint);
  RUN_TEST(test_random);
  RUN_TEST(test_ray_hits_arc);
  RUN_TEST(test_ray_hits_bounding_circle);
  RUN_TEST(test_ray_hits_circle);
  RUN_TEST(test_ray_hits_line_segment);
  RUN_TEST(test_ray_hits_polygon);
  RUN_TEST(test_ray_hits_polygon_trans);
  RUN_TEST(test_script_clone);
  RUN_TEST(test_script_print);
  RUN_TEST(test_script_scan);
  RUN_TEST(test_select_gun);
  RUN_TEST(test_signmod);
  RUN_TEST(test_sound_volume);
  RUN_TEST(test_strdup);
  RUN_TEST(test_strprintf);
  RUN_TEST(test_transition_color);
  RUN_TEST(test_uids);
  RUN_TEST(test_vaddlen);
  RUN_TEST(test_vcaplen);
  RUN_TEST(test_vpolar);
  RUN_TEST(test_vproj);
  RUN_TEST(test_vreflect);
  RUN_TEST(test_vrotate);
  RUN_TEST(test_vunit);
  RUN_TEST(test_vwithlen);
  RUN_TEST(test_zero_array);
  RUN_TEST(test_zero_object);

  return final_test_summary();
}

/*===========================================================================*/
