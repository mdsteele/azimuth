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

#include "azimuth/util/misc.h"

#include <windows.h>

/*===========================================================================*/

#define NANOS_PER_SECOND 1000000000u

uint64_t az_current_time_nanos(void) {
  static uint64_t ticks_per_second = 0;
  if (ticks_per_second == 0) {
    LARGE_INTEGER frequency;
    if (!QueryPerformanceFrequency(&frequency)) {
      AZ_FATAL("QueryPerformanceFrequency failed.\n");
    }
    ticks_per_second = frequency.QuadPart;
  }
  LARGE_INTEGER counter;
  if (!QueryPerformanceCounter(&counter)) {
    AZ_FATAL("QueryPerformanceCounter failed.\n");
  }
  const uint64_t ticks = counter.QuadPart;
  // Avoid overflow in case ticks_per_second is large.
  const uint64_t seconds = ticks / ticks_per_second;
  const uint64_t leftover = ticks % ticks_per_second;
  const uint64_t nanos = leftover * NANOS_PER_SECOND / ticks_per_second;
  return NANOS_PER_SECOND * seconds + nanos;
}

uint64_t az_sleep_until(uint64_t time) {
  const uint64_t now = az_current_time_nanos();
  if (time <= now) return now;
  LARGE_INTEGER decimicros = {.QuadPart = (time - now) / 100u};
  HANDLE timer = CreateWaitableTimer(NULL, TRUE, NULL);
  if (!timer) AZ_FATAL("CreateWaitableTimer failed.\n");
  if (!SetWaitableTimer(timer, &decimicros, 0, NULL, NULL, FALSE)) {
    AZ_FATAL("SetWaitableTimer failed.\n");
  }
  WaitForSingleObject(timer, INFINITE);
  CloseHandle(timer);
  return time;
}

/*===========================================================================*/
