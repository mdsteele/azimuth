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
#ifndef AZIMUTH_UTIL_MUSIC_H_
#define AZIMUTH_UTIL_MUSIC_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "azimuth/util/sound.h"

/*===========================================================================*/

#define AZ_MUSIC_NUM_TRACKS 5

typedef enum {
  AZ_NOTE_REST,
  AZ_NOTE_TONE,
  AZ_NOTE_DRUM,
  AZ_NOTE_DUTYMOD,
  AZ_NOTE_ENVELOPE,
  AZ_NOTE_LOUDNESS,
  AZ_NOTE_VIBRATO,
  AZ_NOTE_WAVEFORM,
} az_note_type_t;

typedef struct {
  az_note_type_t type;
  union {
    struct {
      double duration; // seconds
    } rest;
    struct {
      double duration; // seconds
      double frequency; // Hz
    } tone;
    struct {
      double duration; // seconds
      const az_sound_data_t *data;
    } drum;
    struct {
      double depth;
      double speed;
    } dutymod;
    struct {
      double attack_time; // seconds
      double decay_fraction; // fraction of tone duration
    } envelope;
    struct {
      double volume; // multiplier; 1.0 is neutral
    } loudness;
    struct {
      double depth;
      double speed;
    } vibrato;
    struct {
      az_sound_wave_kind_t kind;
      double duty; // 0.0 to 1.0
    } waveform;
  } attributes;
} az_music_note_t;

typedef struct {
  int num_notes;
  az_music_note_t *notes;
} az_music_track_t;

typedef struct {
  az_music_track_t tracks[AZ_MUSIC_NUM_TRACKS];
} az_music_part_t;

typedef enum {
  AZ_MUSOP_NOP = 0, // Do nothing
  AZ_MUSOP_PLAY, // Play part <index>
  AZ_MUSOP_SETF, // Set flag to <value>
  AZ_MUSOP_BFEQ, // If flag == <value>, jump to <index>
  AZ_MUSOP_BFNE, // If flag != <value>, jump to <index>
  AZ_MUSOP_JUMP  // Jump unconditionally to <index>
} az_music_opcode_t;

typedef struct {
  az_music_opcode_t opcode;
  int value;
  int index;
} az_music_instruction_t;

typedef struct {
  int num_parts;
  az_music_part_t *parts;
  int num_instructions;
  az_music_instruction_t *instructions;
} az_music_t;

bool az_parse_music_from_path(
    const char *filepath, int num_drums, const az_sound_data_t *drums,
    az_music_t *music_out);

bool az_parse_music_from_file(
    FILE *file, int num_drums, const az_sound_data_t *drums,
    az_music_t *music_out);

void az_destroy_music(az_music_t *music);

/*===========================================================================*/

typedef struct {
  const az_music_t *music;
  int flag;
  int pc;
  int steps_since_last_sustain;
  double time_index;
  struct {
    const az_music_track_t *track;
    int note_index;
    size_t drum_index;
    double time_from_note_start;
    az_sound_wave_kind_t waveform;
    double duty;
    double loudness;
    double attack_time, decay_fraction;
    double dutymod_depth, dutymod_speed;
    double vibrato_depth, vibrato_speed;
    int iphase;
    uint64_t noise_bits;
  } voices[AZ_MUSIC_NUM_TRACKS];
  bool stopped;
} az_music_synth_t;

void az_reset_music_synth(az_music_synth_t *synth, const az_music_t *music,
                          int flag);

void az_synthesize_music(az_music_synth_t *synth, int16_t *samples,
                         int num_samples);

/*===========================================================================*/

#endif // AZIMUTH_UTIL_MUSIC_H_
