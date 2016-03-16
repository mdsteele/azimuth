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

#include "azimuth/system/timer.h"

#include <stdint.h>
#include <time.h>

#include "azimuth/util/misc.h"
#include "azimuth/util/warning.h"

/*===========================================================================*/

#define NANOS_PER_SECOND 1000000000u

uint64_t az_current_time_nanos(void) {
  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
    AZ_FATAL("clock_gettime failed.\n");
  }
  return NANOS_PER_SECOND * (uint64_t)ts.tv_sec + (uint64_t)ts.tv_nsec;
}

uint64_t az_sleep_until(uint64_t time) {
  const uint64_t now = az_current_time_nanos();
  if (time > now) {
    const struct timespec ts = {
      .tv_sec = time / NANOS_PER_SECOND,
      .tv_nsec = time % NANOS_PER_SECOND
    };
    const int err = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL);
    if (err != 0) {
      AZ_WARNING_ONCE("clock_nanosleep failed with error %d.\n", err);
    }
  }
  return now;
}

/*===========================================================================*/
