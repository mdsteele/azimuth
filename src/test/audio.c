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

#include <stdio.h>

#include "azimuth/util/audio.h"
#include "azimuth/util/sound.h"
#include "test/test.h"

/*===========================================================================*/

void test_create_sound_data(void) {
  // Create a sound effect.  It should have some samples.
  const az_sound_spec_t spec = {
    .wave_kind = AZ_SINE_WAVE, .env_decay = 0.125, .start_freq = 0.5
  };
  az_sound_data_t data = { .num_samples = 0 };
  az_create_sound_data(&spec, &data);
  EXPECT_INT_EQ(782, data.num_samples);
  EXPECT_TRUE(data.samples != NULL);
  // After destroying the sound data, its fields should be zeroed.
  az_destroy_sound_data(&data);
  EXPECT_INT_EQ(0, data.num_samples);
  EXPECT_TRUE(data.samples == NULL);
}

void test_persist_sound(void) {
  az_soundboard_t soundboard = { .num_persists = 0 };
  const az_sound_data_t sound1, sound2, sound3, sound4;
  az_persist_sound_data(&soundboard, &sound1);
  az_reset_sound_data(&soundboard, &sound2);
  az_hold_sound_data(&soundboard, &sound3);
  az_persist_sound_data(&soundboard, &sound2);
  az_loop_sound_data(&soundboard, &sound4);
  EXPECT_INT_EQ(4, soundboard.num_persists);

  EXPECT_TRUE(soundboard.persists[0].sound_data == &sound1);
  EXPECT_TRUE(soundboard.persists[0].play);
  EXPECT_FALSE(soundboard.persists[0].loop);
  EXPECT_FALSE(soundboard.persists[0].reset);

  EXPECT_TRUE(soundboard.persists[1].sound_data == &sound2);
  EXPECT_TRUE(soundboard.persists[1].play);
  EXPECT_FALSE(soundboard.persists[1].loop);
  EXPECT_TRUE(soundboard.persists[1].reset);

  EXPECT_TRUE(soundboard.persists[2].sound_data == &sound3);
  EXPECT_FALSE(soundboard.persists[2].play);
  EXPECT_FALSE(soundboard.persists[2].loop);
  EXPECT_FALSE(soundboard.persists[2].reset);

  EXPECT_TRUE(soundboard.persists[3].sound_data == &sound4);
  EXPECT_TRUE(soundboard.persists[3].play);
  EXPECT_TRUE(soundboard.persists[3].loop);
  EXPECT_FALSE(soundboard.persists[3].reset);
}

#define PARSE_MUSIC_FROM_STRING(music_string, music) do { \
    FILE *file = tmpfile(); \
    ASSERT_TRUE(file != NULL); \
    EXPECT_TRUE(0 <= fputs((music_string), file)); \
    rewind(file); \
    const bool success = az_parse_music_from_file(file, 0, NULL, (music)); \
    fclose(file); \
    ASSERT_TRUE(success); \
  } while (false)

void test_parse_music(void) {
  const char *music_string =
    "@M \"A|AB\" % foo\n"
    "%% bar\n"
    "!Part B\n"
    "1| c3q d e f |\n"
    "!Part A  % the intro!\n"
    "1 Ws L30\n"
    "1| c3q r r g |\n";
  az_music_t music;
  PARSE_MUSIC_FROM_STRING(music_string, &music);
  ASSERT_INT_EQ(4, music.num_instructions);
  EXPECT_INT_EQ(AZ_MUSOP_PLAY, music.instructions[0].opcode);
  EXPECT_INT_EQ(1, music.instructions[0].index);
  EXPECT_INT_EQ(AZ_MUSOP_PLAY, music.instructions[1].opcode);
  EXPECT_INT_EQ(1, music.instructions[1].index);
  EXPECT_INT_EQ(AZ_MUSOP_PLAY, music.instructions[2].opcode);
  EXPECT_INT_EQ(0, music.instructions[2].index);
  EXPECT_INT_EQ(AZ_MUSOP_JUMP, music.instructions[3].opcode);
  EXPECT_INT_EQ(1, music.instructions[3].index);
  const az_music_track_t *part_b_track_1 = &music.parts[0].tracks[0];
  EXPECT_INT_EQ(4, part_b_track_1->num_notes);
  const az_music_track_t *part_a_track_1 = &music.parts[1].tracks[0];
  EXPECT_INT_EQ(6, part_a_track_1->num_notes);
}

void test_parse_music_instructions(void) {
  const char *music_string =
    "@M \"$2 x :y z !4x y =5z\"\n";
  az_music_t music;
  PARSE_MUSIC_FROM_STRING(music_string, &music);
  ASSERT_INT_EQ(4, music.num_instructions);
  EXPECT_INT_EQ(AZ_MUSOP_SETF, music.instructions[0].opcode);
  EXPECT_INT_EQ(2, music.instructions[0].value);
  EXPECT_INT_EQ(AZ_MUSOP_JUMP, music.instructions[1].opcode);
  EXPECT_INT_EQ(3, music.instructions[1].index);
  EXPECT_INT_EQ(AZ_MUSOP_BFNE, music.instructions[2].opcode);
  EXPECT_INT_EQ(4, music.instructions[2].value);
  EXPECT_INT_EQ(1, music.instructions[2].index);
  EXPECT_INT_EQ(AZ_MUSOP_BFEQ, music.instructions[3].opcode);
  EXPECT_INT_EQ(5, music.instructions[3].value);
  EXPECT_INT_EQ(2, music.instructions[3].index);
}

/*===========================================================================*/
