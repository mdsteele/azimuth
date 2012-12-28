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

#include "azimuth/state/ship.h"

#include <assert.h>
#include <stdbool.h>

#include "azimuth/state/wall.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/polygon.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

#define SHIP_BOUNDING_RADIUS 20.0

static const az_vector_t ship_vertices[] = {
  {20, 0}, {15, 4}, {6, 12}, {-10, 12}, {-14, 4},
  {-14, -4}, {-10, -12}, {6, -12}, {15, -4}
};

const az_polygon_t AZ_SHIP_POLYGON = {
  .num_vertices = AZ_ARRAY_SIZE(ship_vertices),
  .vertices = ship_vertices
};

bool az_ship_is_present(const az_ship_t *ship) {
  return ship->player.shields > 0.0;
}

bool az_point_touches_ship(const az_ship_t *ship, az_vector_t point) {
  assert(az_ship_is_present(ship));
  return (az_vwithin(point, ship->position, SHIP_BOUNDING_RADIUS) &&
          az_polygon_contains(AZ_SHIP_POLYGON,
                              az_vrotate(az_vsub(point, ship->position),
                                         -ship->angle)));
}

bool az_ray_hits_ship(const az_ship_t *ship, az_vector_t start,
                      az_vector_t delta, az_vector_t *point_out,
                      az_vector_t *normal_out) {
  assert(az_ship_is_present(ship));
  return (az_ray_hits_bounding_circle(start, delta, ship->position,
                                      SHIP_BOUNDING_RADIUS) &&
          az_ray_hits_polygon_trans(AZ_SHIP_POLYGON, ship->position,
                                    ship->angle, start, delta,
                                    point_out, normal_out));
}

bool az_circle_hits_ship(
    const az_ship_t *ship, double radius, az_vector_t start, az_vector_t delta,
    az_vector_t *pos_out, az_vector_t *impact_out) {
  assert(az_ship_is_present(ship));
  return (az_ray_hits_bounding_circle(start, delta, ship->position,
                                      SHIP_BOUNDING_RADIUS + radius) &&
          az_circle_hits_polygon_trans(AZ_SHIP_POLYGON, ship->position,
                                       ship->angle, radius, start, delta,
                                       pos_out, impact_out));
}

bool az_arc_circle_hits_ship(
    const az_ship_t *ship, double circle_radius,
    az_vector_t start, az_vector_t spin_center, double spin_angle,
    double *angle_out, az_vector_t *pos_out, az_vector_t *impact_out) {
  assert(az_ship_is_present(ship));
  return (az_arc_ray_might_hit_bounding_circle(
              start, spin_center, spin_angle, ship->position,
              SHIP_BOUNDING_RADIUS + circle_radius) &&
          az_arc_circle_hits_polygon_trans(
              AZ_SHIP_POLYGON, ship->position, ship->angle,
              circle_radius, start, spin_center, spin_angle,
              angle_out, pos_out, impact_out));
}

/*===========================================================================*/
