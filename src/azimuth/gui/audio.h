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
#ifndef AZIMUTH_GUI_AUDIO_H_
#define AZIMUTH_GUI_AUDIO_H_

#include "azimuth/util/audio.h"

/*===========================================================================*/

// Initialize our audio system (once the GUI has been initialized).  This is
// called by az_init_gui, and should not be called from elsewhere.
void az_init_audio_mixer(void);

// Call this once per frame to update our audio system.  The audio system must
// be initialized first (by calling az_init_gui, which will in turn call
// az_init_audio_mixer above).
void az_tick_audio_mixer(az_soundboard_t *soundboard);

/*===========================================================================*/

#endif // AZIMUTH_GUI_AUDIO_H_
