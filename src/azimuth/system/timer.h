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
#ifndef AZIMUTH_SYSTEM_TIMER_H_
#define AZIMUTH_SYSTEM_TIMER_H_

#include <stdint.h>

/*===========================================================================*/

// Get the current time in nanoseconds, as measured from some unspecified zero
// point.  This time is guaranteed to be monotonic even in the face of e.g. NTP
// time adjustments.
uint64_t az_current_time_nanos(void);

// Sleep until az_current_time_nanos() would return the given time value and
// return the time at which the sleep *started*.  Returns without sleeping if
// the returned current time is greater than or equal to the given time.
uint64_t az_sleep_until(uint64_t time);

/*===========================================================================*/

#endif // AZIMUTH_SYSTEM_TIMER_H_
