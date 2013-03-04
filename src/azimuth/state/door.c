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

#include "azimuth/state/door.h"

#include <assert.h>
#include <stdbool.h>

#include "azimuth/state/player.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/polygon.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

AZ_STATIC_ASSERT(AZ_NUM_DOOR_KINDS == AZ_DOOR_FORCEFIELD);

bool az_can_open_door(az_door_kind_t door_kind,
                      az_damage_flags_t damage_kind) {
  assert(door_kind != AZ_DOOR_NOTHING);
  switch (door_kind) {
    case AZ_DOOR_NOTHING: AZ_ASSERT_UNREACHABLE();
    case AZ_DOOR_NORMAL: return true;
    case AZ_DOOR_LOCKED: return false;
    case AZ_DOOR_ROCKET:
      return (damage_kind & (AZ_DMGF_ROCKET | AZ_DMGF_HYPER_ROCKET));
    case AZ_DOOR_HYPER_ROCKET:
      return (damage_kind & AZ_DMGF_HYPER_ROCKET);
    case AZ_DOOR_BOMB:
      return (damage_kind & (AZ_DMGF_BOMB | AZ_DMGF_MEGA_BOMB));
    case AZ_DOOR_MEGA_BOMB:
      return (damage_kind & AZ_DMGF_MEGA_BOMB);
    case AZ_DOOR_PASSAGE: return false;
    case AZ_DOOR_FORCEFIELD: return false;
  }
  AZ_ASSERT_UNREACHABLE();
}

/*===========================================================================*/

// If door->is_open is false but door->openness is at least OPEN_THRESHOLD,
// then the door will still be counted as open for the purposes of collisions
// with the outside of the door.
#define OPEN_THRESHOLD 0.75

const double AZ_DOOR_BOUNDING_RADIUS = 58.31;

static const az_vector_t closed_door_vertices[] = {
  {30, 50}, {-30, 50}, {-30, -50}, {30, -50}, {40, -35}, {40, 35}
};
static const az_polygon_t closed_door_polygon =
  AZ_INIT_POLYGON(closed_door_vertices);

static const az_vector_t open_door_vertices[] = {
  {30, 50}, {-30, 50}, {-30, -50}, {30, -50},
  {30, -49}, {-29, -49}, {-29, 49}, {30, 49}
};
static const az_polygon_t open_door_polygon =
  AZ_INIT_POLYGON(open_door_vertices);

static const az_vector_t closed_forcefield_vertices[] = {
  {-14, 54}, {-12, 48}, {-10, 48}, {-10, -48}, {-12, -48}, {-14, -54},
  {14, -54}, {12, -48}, {10, -48}, {10, 48}, {12, 48}, {14, 54}
};
static const az_polygon_t closed_forcefield_polygon =
  AZ_INIT_POLYGON(closed_forcefield_vertices);

static const az_vector_t open_forcefield_vertices_1[] = {
  {-14, 54}, {-12, 48}, {12, 48}, {14, 54}
};
static const az_polygon_t open_forcefield_polygon_1 =
  AZ_INIT_POLYGON(open_forcefield_vertices_1);

static const az_vector_t open_forcefield_vertices_2[] = {
  {-14, -54}, {-12, -48}, {12, -48}, {14, -54}
};
static const az_polygon_t open_forcefield_polygon_2 =
  AZ_INIT_POLYGON(open_forcefield_vertices_2);

bool az_circle_touches_door_outside(
    const az_door_t *door, double radius, az_vector_t center) {
  assert(door->kind != AZ_DOOR_NOTHING);
  if (door->kind == AZ_DOOR_PASSAGE) return false;
  if (!az_vwithin(center, door->position,
                  radius + AZ_DOOR_BOUNDING_RADIUS)) return false;
  const bool is_open = (door->is_open || door->openness >= OPEN_THRESHOLD);
  if (door->kind == AZ_DOOR_FORCEFIELD && is_open) {
    return (az_circle_touches_polygon_trans(
                open_forcefield_polygon_1, door->position, door->angle,
                radius, center) ||
            az_circle_touches_polygon_trans(
                open_forcefield_polygon_2, door->position, door->angle,
                radius, center));
  } else {
    return az_circle_touches_polygon_trans(
        (is_open ? open_door_polygon :
         (door->kind == AZ_DOOR_FORCEFIELD ?
          closed_forcefield_polygon : closed_door_polygon)),
        door->position, door->angle, radius, center);
  }
}

bool az_ray_hits_door_outside(
    const az_door_t *door, az_vector_t start, az_vector_t delta,
    az_vector_t *point_out, az_vector_t *normal_out) {
  assert(door->kind != AZ_DOOR_NOTHING);
  if (door->kind == AZ_DOOR_PASSAGE) return false;
  if (!az_ray_hits_bounding_circle(start, delta, door->position,
                                   AZ_DOOR_BOUNDING_RADIUS)) return false;
  const bool is_open = (door->is_open || door->openness >= OPEN_THRESHOLD);
  if (door->kind == AZ_DOOR_FORCEFIELD && is_open) {
    az_vector_t point;
    bool hit = az_ray_hits_polygon_trans(
                   open_forcefield_polygon_1, door->position, door->angle,
                   start, delta, &point, normal_out);
    if (hit) delta = az_vsub(point, start);
    hit |= az_ray_hits_polygon_trans(
               open_forcefield_polygon_2, door->position, door->angle,
               start, delta, &point, normal_out);
    if (hit && point_out != NULL) *point_out = point;
    return hit;
  } else {
    return az_ray_hits_polygon_trans(
        (is_open ? open_door_polygon :
         (door->kind == AZ_DOOR_FORCEFIELD ?
          closed_forcefield_polygon : closed_door_polygon)),
        door->position, door->angle, start, delta, point_out, normal_out);
  }
}

bool az_circle_hits_door_outside(
    const az_door_t *door, double radius, az_vector_t start, az_vector_t delta,
    az_vector_t *pos_out, az_vector_t *impact_out) {
  assert(door->kind != AZ_DOOR_NOTHING);
  if (door->kind == AZ_DOOR_PASSAGE) return false;
  if (!az_ray_hits_bounding_circle(start, delta, door->position,
                                   AZ_DOOR_BOUNDING_RADIUS + radius)) {
    return false;
  }
  const bool is_open = (door->is_open || door->openness >= OPEN_THRESHOLD);
  if (door->kind == AZ_DOOR_FORCEFIELD && is_open) {
    az_vector_t pos;
    bool hit = az_circle_hits_polygon_trans(
                   open_forcefield_polygon_1, door->position, door->angle,
                   radius, start, delta, &pos, impact_out);
    if (hit) delta = az_vsub(pos, start);
    hit |= az_circle_hits_polygon_trans(
               open_forcefield_polygon_2, door->position, door->angle,
               radius, start, delta, &pos, impact_out);
    if (hit && pos_out != NULL) *pos_out = pos;
    return hit;
  } else {
    return az_circle_hits_polygon_trans(
        (is_open ? open_door_polygon :
         (door->kind == AZ_DOOR_FORCEFIELD ?
          closed_forcefield_polygon : closed_door_polygon)),
        door->position, door->angle, radius, start, delta,
        pos_out, impact_out);
  }
}

bool az_arc_circle_hits_door_outside(
    const az_door_t *door, double circle_radius,
    az_vector_t start, az_vector_t spin_center, double spin_angle,
    double *angle_out, az_vector_t *pos_out, az_vector_t *impact_out) {
  assert(door->kind != AZ_DOOR_NOTHING);
  if (door->kind == AZ_DOOR_PASSAGE) return false;
  if (!az_arc_ray_might_hit_bounding_circle(
          start, spin_center, spin_angle, door->position,
          AZ_DOOR_BOUNDING_RADIUS + circle_radius)) return false;
  const bool is_open = (door->is_open || door->openness >= OPEN_THRESHOLD);
  if (door->kind == AZ_DOOR_FORCEFIELD && is_open) {
    const bool hit =
      (az_arc_circle_hits_polygon_trans(
           open_forcefield_polygon_1, door->position, door->angle,
           circle_radius, start, spin_center, spin_angle,
           &spin_angle, pos_out, impact_out) ||
       az_arc_circle_hits_polygon_trans(
           open_forcefield_polygon_2, door->position, door->angle,
           circle_radius, start, spin_center, spin_angle,
           &spin_angle, pos_out, impact_out));
    if (hit && angle_out != NULL) *angle_out = spin_angle;
    return hit;
  } else {
    return az_arc_circle_hits_polygon_trans(
        (is_open ? open_door_polygon :
         (door->kind == AZ_DOOR_FORCEFIELD ?
          closed_forcefield_polygon : closed_door_polygon)),
        door->position, door->angle, circle_radius, start, spin_center,
        spin_angle, angle_out, pos_out, impact_out);
  }
}

/*===========================================================================*/

static const az_vector_t entrance_vertices[] = {
  {-10, 49}, {-30, 49}, {-30, -49}, {-10, -49}
};
static const az_polygon_t entrance_polygon =
  AZ_INIT_POLYGON(entrance_vertices);

bool az_ray_hits_door_inside(
    const az_door_t *door, az_vector_t start, az_vector_t delta,
    az_vector_t *point_out, az_vector_t *normal_out) {
  assert(door->kind != AZ_DOOR_NOTHING);
  if (door->kind == AZ_DOOR_FORCEFIELD ||
      (door->kind != AZ_DOOR_PASSAGE && !door->is_open)) return false;
  return (az_ray_hits_bounding_circle(start, delta, door->position,
                                      AZ_DOOR_BOUNDING_RADIUS) &&
          az_ray_hits_polygon_trans(entrance_polygon, door->position,
                                    door->angle, start, delta, point_out,
                                    normal_out));
}

bool az_circle_hits_door_inside(
    const az_door_t *door, double radius, az_vector_t start, az_vector_t delta,
    az_vector_t *pos_out, az_vector_t *impact_out) {
  assert(door->kind != AZ_DOOR_NOTHING);
  if (door->kind == AZ_DOOR_FORCEFIELD ||
      (door->kind != AZ_DOOR_PASSAGE && !door->is_open)) return false;
  return (az_ray_hits_bounding_circle(start, delta, door->position,
                                      AZ_DOOR_BOUNDING_RADIUS + radius) &&
          az_circle_hits_polygon_trans(entrance_polygon, door->position,
                                       door->angle, radius, start, delta,
                                       pos_out, impact_out));
}

bool az_arc_circle_hits_door_inside(
    const az_door_t *door, double circle_radius,
    az_vector_t start, az_vector_t spin_center, double spin_angle,
    double *angle_out, az_vector_t *pos_out, az_vector_t *impact_out) {
  assert(door->kind != AZ_DOOR_NOTHING);
  if (door->kind == AZ_DOOR_FORCEFIELD ||
      (door->kind != AZ_DOOR_PASSAGE && !door->is_open)) return false;
  return (az_arc_ray_might_hit_bounding_circle(
              start, spin_center, spin_angle, door->position,
              AZ_DOOR_BOUNDING_RADIUS + circle_radius) &&
          az_arc_circle_hits_polygon_trans(
              entrance_polygon, door->position, door->angle,
              circle_radius, start, spin_center, spin_angle,
              angle_out, pos_out, impact_out));
}

/*===========================================================================*/
