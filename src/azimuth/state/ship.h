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

#pragma once
#ifndef AZIMUTH_STATE_SHIP_H_
#define AZIMUTH_STATE_SHIP_H_

#include <stdbool.h>

#include "azimuth/state/player.h"
#include "azimuth/state/uid.h"
#include "azimuth/state/wall.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

typedef struct {
  bool up, down, left, right;
  bool fire_pressed, fire_held;
  bool ordn_held;
  bool util_pressed, util_held;
  bool burn;
} az_controls_t;

typedef struct {
  az_controls_t controls;
  az_player_t player;
  az_vector_t position; // pixels
  az_vector_t velocity; // pixels/second
  double angle; // radians
  double gun_charge; // from 0.0 (uncharged) to 1.0 (fully charged)
  double shield_flare; // from 0.0 (nothing) to 1.0 (was just now hit)
  struct {
    bool active;
    az_uid_t node_uid;
    double distance;
  } tractor_beam;
} az_ship_t;

const extern az_polygon_t AZ_SHIP_POLYGON;

// Return true if the ship is present (i.e. not destroyed), false otherwise.
bool az_ship_is_present(const az_ship_t *ship);

// Return true if the given point should count as intersecting the ship's
// shield perimeter.
bool az_point_hits_ship(const az_ship_t *ship, az_vector_t point);

// Determine if a ray, travelling delta from start, will hit the ship.  If it
// does, stores the intersection point in *point_out (if point_out is non-NULL)
// and the normal vector in *normal_out (if normal_out is non-NULL).
bool az_ray_hits_ship(const az_ship_t *ship, az_vector_t start,
                      az_vector_t delta, az_vector_t *point_out,
                      az_vector_t *normal_out);

// Determine if a circle with the given radius, travelling delta from start,
// will hit the ship.  If it does, the function stores in *pos_out
// the earliest position of the circle at which it touches the wall (if pos_out
// is non-NULL) and in *impact_out the point of intersection (if impact_out is
// non-NULL).
bool az_circle_hits_ship(const az_ship_t *ship, double radius,
                         az_vector_t start, az_vector_t delta,
                         az_vector_t *point_out, az_vector_t *impact_out);

/*===========================================================================*/

#endif // AZIMUTH_STATE_SHIP_H_
