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

#include <assert.h>
#include <stdlib.h>

#include <SDL/SDL.h>

#include "azimuth/state/music.h"
#include "azimuth/util/music.h"
#include "azimuth/util/sound.h"

/*===========================================================================*/

static az_music_t music;
static az_music_synth_t synth;

static void audio_callback(void *userdata, Uint8 *bytes, int numbytes) {
  assert(numbytes % sizeof(int16_t) == 0);
  const int num_samples = numbytes / sizeof(int16_t);
  int16_t *samples = (int16_t*)bytes;
  az_synthesize_music(&synth, samples, num_samples);
}

static void destroy_music(void) {
  az_destroy_music(&music);
}

int main(int argc, char **argv) {
  if (argc < 2 || argc > 3) {
    fprintf(stderr, "Usage: %s filename [flag]\n", argv[0]);
    return EXIT_FAILURE;
  }

  int music_flag = 0;
  if (argc >= 3) {
    if (sscanf(argv[2], "%d", &music_flag) < 1) {
      fprintf(stderr, "Invalid flag value: %s\n", argv[2]);
      return EXIT_FAILURE;
    }
  }

  // Initialize drum kit:
  int num_drums = 0;
  const az_sound_data_t *drums = NULL;
  az_get_drum_kit(&num_drums, &drums);

  // Load music:
  if (!az_parse_music_from_path(argv[1], num_drums, drums, &music)) {
    fprintf(stderr, "ERROR: failed to parse music.\n");
    return EXIT_FAILURE;
  }
  atexit(destroy_music);
  az_reset_music_synth(&synth, &music, music_flag);

  // Initialize SDL audio:
  if (SDL_Init(SDL_INIT_AUDIO) != 0) {
    fprintf(stderr, "ERROR: SDL_Init failed.\n");
    return EXIT_FAILURE;
  }
  atexit(SDL_Quit);
  SDL_AudioSpec audio_spec = {
    .freq = AZ_AUDIO_RATE,
    .format = AUDIO_S16SYS,
    .channels = 1,
    .samples = 4096,
    .callback = &audio_callback
  };
  if (SDL_OpenAudio(&audio_spec, NULL) != 0) {
    fprintf(stderr, "ERROR: SDL_OpenAudio failed.\n");
    return EXIT_FAILURE;
  }
  atexit(SDL_CloseAudio);

  // Start playing music:
  SDL_PauseAudio(0); // unpause audio
  fprintf(stderr, "Press Ctrl-D to stop playing.\n");

  // Read flag values from stdin:
  int ch;
  do {
    fprintf(stdout, "flag=%d> ", music_flag);
    fflush(stdout);
    int flag = 0;
    while ((ch = getchar()) >= '0' && ch <= '9') {
      flag = 10 * flag + (ch - '0');
    }
    SDL_LockAudio(); {
      synth.flag = music_flag = flag;
    } SDL_UnlockAudio();
  } while (ch != EOF);
  fprintf(stdout, "\n");

  return EXIT_SUCCESS;
}

/*===========================================================================*/
