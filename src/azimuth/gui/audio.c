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

#include "azimuth/gui/audio.h"

#include <assert.h>
#include <stdbool.h>

#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>

#include "azimuth/system/resource.h"
#include "azimuth/util/audio.h"
#include "azimuth/util/misc.h"

/*===========================================================================*/

static struct {
  const char *filename;
  Mix_Music *music; // may be NULL if music failed to load
} music_entries[] = {
  [AZ_MUS_TITLE] = { .filename = "title-screen.mp3" },
  [AZ_MUS_CNIDAM_ZONE] = { .filename = "cnidam-zone.mp3" },
  [AZ_MUS_ZENITH_CORE] = { .filename = "zenith-core.mp3" }
};

static void load_all_music(void) {
  const char *resource_dir = az_get_resource_directory();
  char path_buffer[strlen(resource_dir) + 50u];
  AZ_ARRAY_LOOP(entry, music_entries) {
    assert(entry->filename != NULL);
    assert(strlen(entry->filename) > 0);
    sprintf(path_buffer, "%s/music/%s", resource_dir, entry->filename);
    entry->music = Mix_LoadMUS(path_buffer);
    if (entry->music == NULL) {
      fprintf(stderr, "Warning: failed to load %s\n", entry->filename);
    }
  }
}

static void free_all_music(void) {
  AZ_ARRAY_LOOP(entry, music_entries) {
    if (entry->music != NULL) {
      Mix_FreeMusic(entry->music);
      entry->music = NULL;
    }
  }
}

static void tick_music(const az_soundboard_t *soundboard) {
  static enum { STOPPED, FADING_OUT, PLAYING } music_status = STOPPED;
  static az_music_key_t current_music = AZ_MUS_TITLE;
  static bool music_pending = false;
  static az_music_key_t next_music = AZ_MUS_TITLE;

  // If nothing is playing right now (e.g. because we finished fading out, or
  // because the last music we started failed to load at program startup), set
  // us to STOPPED status.
  if (!Mix_PlayingMusic()) music_status = STOPPED;

  // If desired, change which music will play next.
  int fade_out_millis = -1;
  switch (soundboard->music_action) {
    case AZ_MUSA_NOTHING: break;
    case AZ_MUSA_CHANGE:
      next_music = soundboard->next_music;
      music_pending = true;
      if (current_music != next_music || music_status != PLAYING) {
        fade_out_millis = 500;
      }
      break;
    case AZ_MUSA_STOP:
      assert(soundboard->music_fade_out_millis >= 0);
      music_pending = false;
      fade_out_millis = soundboard->music_fade_out_millis;
      break;
  }
  if (fade_out_millis >= 0 && music_status == PLAYING) {
    if (fade_out_millis > 0) {
      Mix_FadeOutMusic(fade_out_millis);
      music_status = FADING_OUT;
    } else {
      Mix_HaltMusic();
      music_status = STOPPED;
    }
  }

  // If we're ready to start the next music, do so.
  if (music_pending && music_status == STOPPED) {
    current_music = next_music;
    music_pending = false;
    Mix_Music *music_data = music_entries[current_music].music;
    if (music_data != NULL && Mix_PlayMusic(music_data, -1) == 0) {
      music_status = PLAYING;
    }
  }
}

/*===========================================================================*/

// We use 16-bit stereo at 22050 samples/sec and a buffer size of 4096 samples.
#define AUDIO_FORMAT AUDIO_S16SYS
#define AUDIO_CHANNELS 2
#define AUDIO_RATE 22050
#define AUDIO_BUFFERSIZE 4096

static bool audio_mixer_initialized = false;

static void shut_down_audio_mixer(void) {
  free_all_music();
  Mix_CloseAudio();
}

void az_init_audio_mixer(void) {
  assert(!audio_mixer_initialized);
  // Initialize the SDL mixer.
  if (Mix_OpenAudio(AUDIO_RATE, AUDIO_FORMAT, AUDIO_CHANNELS,
                    AUDIO_BUFFERSIZE) != 0) {
    AZ_FATAL("Mix_OpenAudio failed.\n");
  }
  atexit(shut_down_audio_mixer);
  // Load our music and sound data.
  load_all_music();
  // TODO load/initialize sound data
  audio_mixer_initialized = true;
}

void az_tick_audio_mixer(az_soundboard_t *soundboard) {
  assert(audio_mixer_initialized);
  tick_music(soundboard);
  // TODO tick sounds
  memset(soundboard, 0, sizeof(*soundboard));
}

/*===========================================================================*/
