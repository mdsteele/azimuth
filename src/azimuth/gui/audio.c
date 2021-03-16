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
#include <stdint.h>

#include <SDL.h>

#include "azimuth/util/audio.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/music.h"
#include "azimuth/util/sound.h"
#include "azimuth/util/vector.h"
#include "azimuth/util/warning.h"

/*===========================================================================*/
// Constants:

// We use 16-bit mono at 22050 samples/sec and a buffer size of 1024 samples.
#define AUDIO_FORMAT AUDIO_S16SYS
#define AUDIO_CHANNELS 1
AZ_STATIC_ASSERT(AZ_AUDIO_RATE == 22050);
#define AUDIO_BUFFERSIZE 1024

// Values controlling global_music_volume and global_sound_volume:
#define VOLUME_SHIFT 8
#define MAX_VOLUME (1 << VOLUME_SHIFT)
// How many sound effects we can play simultaneously:
#define MAX_SIMULTANEOUS_SOUNDS 16

/*===========================================================================*/
// Globals:

// All of these globals are protected by the SDL audio mutex.  Any reads/writes
// outside of audio_callback() must be wrapped in calls to SDL_LockAudio() and
// SDL_UnlockAudio().

static int global_music_volume = MAX_VOLUME; // 0 to MAX_VOLUME
static int global_sound_volume = MAX_VOLUME; // 0 to MAX_VOLUME

static az_music_synth_t music_synth;
static int music_fade_volume = 0; // 0 to MAX_VOLUME
static int music_fade_slowdown = 0;
static int music_fade_counter = 0;
static const az_music_t *next_music = NULL;
static int next_music_flag = 0;

static struct {
  const az_sound_data_t *data;
  size_t sample_index;
  int volume; // 0 to MAX_VOLUME
  bool loop, persisted, paused, finished;
} active_sounds[MAX_SIMULTANEOUS_SOUNDS];

static void audio_callback(void *userdata, Uint8 *bytes, int numbytes) {
  assert(numbytes % sizeof(int16_t) == 0);
  const int num_samples = numbytes / sizeof(int16_t);
  int16_t *samples = (int16_t*)bytes;

  if (next_music != NULL && music_fade_volume == 0) {
    az_reset_music_synth(&music_synth, next_music, next_music_flag);
    music_fade_volume = MAX_VOLUME;
    music_fade_slowdown = 0;
    music_fade_counter = 0;
    next_music = NULL;
    next_music_flag = 0;
  }
  az_synthesize_music(&music_synth, samples, num_samples);

  for (int i = 0; i < num_samples; ++i) {
    int sound_sample = 0;
    AZ_ARRAY_LOOP(sound, active_sounds) {
      if (sound->data == NULL) continue;
      if (sound->paused || sound->finished) {
        assert(sound->persisted);
        continue;
      }
      assert(sound->data->num_samples > 0);
      assert(sound->sample_index < sound->data->num_samples);
      sound_sample += (sound->data->samples[sound->sample_index] *
                       sound->volume) >> VOLUME_SHIFT;
      ++sound->sample_index;
      if (sound->sample_index >= sound->data->num_samples) {
        if (sound->loop) {
          assert(sound->persisted);
          sound->sample_index = 0;
        } else if (sound->persisted) {
          sound->finished = true;
        } else AZ_ZERO_OBJECT(sound);
      }
    }
    int sample = (global_music_volume * (int)samples[i]) >> VOLUME_SHIFT;
    sample = (music_fade_volume * sample) >> VOLUME_SHIFT;
    sample += (global_sound_volume * sound_sample) >> VOLUME_SHIFT;
    samples[i] = az_imin(az_imax(INT16_MIN, sample), INT16_MAX);

    // Fade out music, if applicable:
    if (music_fade_slowdown > 0 && music_fade_volume > 0) {
      assert(music_fade_counter > 0);
      assert(music_fade_counter <= music_fade_slowdown);
      --music_fade_counter;
      if (music_fade_counter == 0) {
        music_fade_counter = music_fade_slowdown;
        --music_fade_volume;
        if (music_fade_volume == 0 && music_synth.music != NULL) {
          az_reset_music_synth(&music_synth, NULL, 0);
        }
      }
    }
  }
}

/*===========================================================================*/
// Music:

static void tick_music(const az_soundboard_t *soundboard) {
  if (soundboard->change_current_music_flag) {
    music_synth.flag = soundboard->new_current_music_flag;
  }
  if (soundboard->change_music) {
    if (soundboard->next_music == music_synth.music) {
      music_fade_slowdown = 0;
      next_music = NULL;
      next_music_flag = 0;
      if (soundboard->change_next_music_flag) {
        music_synth.flag = soundboard->new_next_music_flag;
      }
    } else {
      music_fade_slowdown =
        (int)((soundboard->music_fade_out_seconds * AZ_AUDIO_RATE) /
              MAX_VOLUME);
      next_music = soundboard->next_music;
      next_music_flag = (soundboard->change_next_music_flag ?
                         soundboard->new_next_music_flag : 0);
    }
    music_fade_counter = music_fade_slowdown;
  } else assert(!soundboard->change_next_music_flag);
}

/*===========================================================================*/
// Sound effects:

static void tick_sounds(const az_soundboard_t *soundboard) {
  // First, go through each of the active persisted sounds and update their
  // status based on the soundboard.
  bool already_active[soundboard->num_persists];
  memset(already_active, 0, sizeof(bool) * soundboard->num_persists);
  AZ_ARRAY_LOOP(sound, active_sounds) {
    if (sound->data == NULL) continue;
    if (!sound->persisted) {
      assert(!sound->loop);
      continue;
    }
    // Check if this sound is in the soundboard's persists array.  If not, we
    // will implicitly halt and reset the sound.
    bool reset = true;
    for (int i = 0; i < soundboard->num_persists; ++i) {
      if (soundboard->persists[i].sound_data != sound->data) continue;
      if (!soundboard->persists[i].reset) {
        reset = false;
        already_active[i] = true;
        sound->volume = (int)(soundboard->persists[i].volume * MAX_VOLUME);
        sound->paused = !soundboard->persists[i].play;
      }
      break;
    }
    // If we're supposed to reset this sound, halt it.
    if (reset) AZ_ZERO_OBJECT(sound);
  }

  // Second, go through the soundboard and start playing any new persistent
  // sounds that need to be started.
  for (int i = 0; i < soundboard->num_persists; ++i) {
    if (already_active[i] || !soundboard->persists[i].play) continue;
    if (soundboard->persists[i].sound_data->num_samples == 0) continue;
    bool success = false;
    AZ_ARRAY_LOOP(sound, active_sounds) {
      if (sound->data != NULL) continue;
      sound->data = soundboard->persists[i].sound_data;
      sound->sample_index = 0;
      sound->volume = (int)(soundboard->persists[i].volume * MAX_VOLUME);
      sound->loop = soundboard->persists[i].loop;
      sound->persisted = true;
      sound->paused = false;
      success = true;
      break;
    }
    if (!success) AZ_WARNING_ONCE("Could not play persistent sound\n");
  }

  // Third, start playing any new one-shot sounds.
  for (int i = 0; i < soundboard->num_oneshots; ++i) {
    if (soundboard->oneshots[i].sound_data->num_samples == 0) continue;
    bool success = false;
    AZ_ARRAY_LOOP(sound, active_sounds) {
      if (sound->data != NULL) continue;
      sound->data = soundboard->oneshots[i].sound_data;
      sound->sample_index = 0;
      sound->volume = (int)(soundboard->oneshots[i].volume * MAX_VOLUME);
      sound->loop = false;
      sound->persisted = false;
      sound->paused = false;
      success = true;
      break;
    }
    if (!success) AZ_WARNING_ONCE("Could not play sound effect\n");
  }
}

/*===========================================================================*/
// Audio system:

static bool audio_system_initialized = false;
static bool audio_system_paused = false;

void az_init_audio(void) {
  assert(!audio_system_initialized);

  SDL_AudioSpec audio_spec = {
    .freq = AZ_AUDIO_RATE,
    .format = AUDIO_FORMAT,
    .channels = AUDIO_CHANNELS,
    .samples = AUDIO_BUFFERSIZE,
    .callback = &audio_callback
  };
  if (SDL_OpenAudio(&audio_spec, NULL) != 0) {
    AZ_FATAL("SDL_OpenAudio failed: %s\n", SDL_GetError());
  }

  atexit(SDL_CloseAudio);
  audio_system_initialized = true;
}

static int to_int_volume(float volume) {
  return az_imin(az_imax(0, (int)(volume * (float)MAX_VOLUME)), MAX_VOLUME);
}

void az_set_global_music_volume(float volume) {
  assert(audio_system_initialized);
  assert(!audio_system_paused);
  const int new_volume = to_int_volume(volume);
  SDL_LockAudio(); {
    global_music_volume = new_volume;
  } SDL_UnlockAudio();
}

void az_set_global_sound_volume(float volume) {
  assert(audio_system_initialized);
  assert(!audio_system_paused);
  const int new_volume = to_int_volume(volume);
  SDL_LockAudio(); {
    global_sound_volume = new_volume;
  } SDL_UnlockAudio();
}

void az_tick_audio(az_soundboard_t *soundboard) {
  assert(audio_system_initialized);
  assert(!audio_system_paused);
  SDL_LockAudio(); {
    tick_music(soundboard);
    tick_sounds(soundboard);
  } SDL_UnlockAudio();
  AZ_ZERO_OBJECT(soundboard);
}

void az_pause_all_audio(void) {
  if (!audio_system_initialized) return;
  assert(!audio_system_paused);
  SDL_PauseAudio(1);
  audio_system_paused = true;
}

void az_unpause_all_audio(void) {
  if (!audio_system_initialized) return;
  assert(audio_system_paused);
  audio_system_paused = false;
  SDL_PauseAudio(0);
}

/*===========================================================================*/
