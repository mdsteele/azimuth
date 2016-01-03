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
#ifndef AZIMUTH_STATE_VICTORY_H_
#define AZIMUTH_STATE_VICTORY_H_

#include "azimuth/state/baddie.h"
#include "azimuth/state/particle.h"
#include "azimuth/state/projectile.h"
#include "azimuth/util/audio.h"
#include "azimuth/util/clock.h"

/*===========================================================================*/

typedef struct {
  az_clock_t clock;
  az_soundboard_t soundboard;
  double clear_time;
  int percent_completion;

  enum {
    AZ_VS_START = 0,
    AZ_VS_SNAPDRAGON,
    AZ_VS_ROCKWYRM,
    AZ_VS_GUNSHIP,
    AZ_VS_FORCEFIEND,
    AZ_VS_KILOFUGE,
    AZ_VS_NOCTURNE,
    AZ_VS_MAGBEEST,
    AZ_VS_SUPERGUNSHIP,
    AZ_VS_CORE,
    AZ_VS_EXPLODE,
    AZ_VS_DONE
  } step;
  double step_timer;

  az_baddie_t baddies[4];
  az_particle_t particles[100];
  az_projectile_t projectiles[20];
} az_victory_state_t;

/*===========================================================================*/

void az_victory_add_baddie(az_victory_state_t *state, az_baddie_kind_t kind,
                           az_vector_t position, double angle);

void az_victory_add_particle(
    az_victory_state_t *state, az_particle_kind_t kind, az_color_t color,
    az_vector_t position, az_vector_t velocity, double angle, double lifetime,
    double param1, double param2);

void az_victory_add_projectile(az_victory_state_t *state, az_proj_kind_t kind,
                               az_vector_t position, double angle);

void az_victory_clear_objects(az_victory_state_t *state);

/*===========================================================================*/

#endif // AZIMUTH_STATE_VICTORY_H_
