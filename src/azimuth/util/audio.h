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
#ifndef AZIMUTH_UTIL_AUDIO_H_
#define AZIMUTH_UTIL_AUDIO_H_

#include <stdbool.h>

#include "azimuth/util/music.h"
#include "azimuth/util/sound.h"

/*===========================================================================*/

// A soundboard keeps track of what sounds/music we want to play next.  It will
// be periodically read and flushed by our audio system.  To initialize it,
// simply zero it with a memset.  One should not manipulate the fields of this
// struct directly; instead, use the various functions below.
typedef struct {
  bool change_music, change_current_music_flag, change_next_music_flag;
  int new_current_music_flag, new_next_music_flag;
  const az_music_t *next_music;
  double music_fade_out_seconds;
  int num_oneshots;
  struct {
    const az_sound_data_t *sound_data;
    float volume; // 0 to 1
  } oneshots[10];
  int num_persists;
  struct {
    const az_sound_data_t *sound_data;
    float volume; // 0 to 1
    bool play;
    bool loop;
    bool reset;
  } persists[10];
} az_soundboard_t;

/*===========================================================================*/

// Indicate that we would like to change which music is playing.
void az_change_music_data(az_soundboard_t *soundboard, const az_music_t *music,
                          double fade_out_seconds);

// Change the music flag.
void az_change_music_flag(az_soundboard_t *soundboard, int flag);

// Indicate that we should play the given sound (once).  The sound will not
// loop, and cannot be cancelled or paused once started.
void az_play_sound_data(az_soundboard_t *soundboard,
                        const az_sound_data_t *sound_data, float volume);

// Indicate that we should start playing, or continue to play, the given sound.
// To keep the sound going, we must call this function every frame with the
// same sound, otherwise the sound will stop.  As long as we keep calling this
// function, the sound will continue to loop.
void az_loop_sound_data(az_soundboard_t *soundboard,
                        const az_sound_data_t *sound_data, float volume);

// Indicate that we should start playing, or continue to play, the given sound.
// To keep the sound going, we must call this function every frame with the
// same sound, otherwise the sound will stop.  The sound will play only once,
// and won't restart until we either stop calling this function for at least
// one frame before calling it again, or we call az_reset_sound.
void az_persist_sound_data(az_soundboard_t *soundboard,
                           const az_sound_data_t *sound_data, float volume);

// Indicate that, if the given persisted or looped sound is currently playing,
// we should pause it for this frame.  To keep the sound from resetting, we
// must call this function every frame with the same sound; the sound will
// resume once we start calling az_persist_sound or az_loop_sound again.
void az_hold_sound_data(az_soundboard_t *soundboard,
                        const az_sound_data_t *sound_data);

// Indicate that the given persisted or looped sound should restart from the
// beginning, even if we also call az_persist_sound or az_loop_sound this frame
// to keep the sound going.
void az_reset_sound_data(az_soundboard_t *soundboard,
                         const az_sound_data_t *sound_data);

/*===========================================================================*/

#endif // AZIMUTH_UTIL_AUDIO_H_
