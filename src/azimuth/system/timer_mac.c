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

#include <assert.h>
#include <mach/mach_time.h>
#include <stdint.h>

/*===========================================================================*/

static mach_timebase_info_data_t timebase_info;

uint64_t az_current_time_nanos(void) {
  if (timebase_info.denom == 0) mach_timebase_info(&timebase_info);
  return mach_absolute_time() * timebase_info.numer / timebase_info.denom;
}

uint64_t az_sleep_until(uint64_t time) {
  const uint64_t now = az_current_time_nanos();
  if (time > now) {
    assert(timebase_info.denom != 0);
    assert(timebase_info.numer != 0);
    mach_wait_until(time * timebase_info.denom / timebase_info.numer);
  }
  return now;
}

/*===========================================================================*/
