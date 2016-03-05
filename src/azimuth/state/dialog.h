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
#ifndef AZIMUTH_STATE_DIALOG_H_
#define AZIMUTH_STATE_DIALOG_H_

#include <stdbool.h>
#include <stdio.h> // for FILE

#include "azimuth/util/prefs.h"
#include "azimuth/util/rw.h"

/*===========================================================================*/

// The number of different portraits there are, not counting AZ_POR_NOTHING:
#define AZ_NUM_PORTRAITS 10

typedef enum {
  AZ_POR_NOTHING = 0,
  AZ_POR_HOPPER,
  AZ_POR_HQ,
  AZ_POR_CPU_A,
  AZ_POR_CPU_B,
  AZ_POR_CPU_C,
  AZ_POR_TRICHORD,
  AZ_POR_AZIMUTH,
  AZ_POR_TRICHORD_VIDEO,
  AZ_POR_HUMAN,
  AZ_POR_HUMAN_VIDEO,
} az_portrait_t;

typedef enum {
  AZ_DLS_INACTIVE = 0,  // not in dialogue
  AZ_DLS_BEGIN,  // dialogue box is opening
  AZ_DLS_BLANK, // dialogue box is open, but no text shown
  AZ_DLS_TALK,  // one character is talking
  AZ_DLS_WAIT,  // waiting for player to press enter
  AZ_DLS_END  // dialogue box is closing
} az_dialogue_step_t;

typedef struct {
  az_dialogue_step_t step;
  double progress; // progress on current step (0 to 1)
  bool hidden;
  bool top_turn;  // true if top portrait talking, false if bottom portrait
  az_portrait_t top, bottom;
  const char *paragraph;
  int paragraph_length;
  int chars_to_print;
} az_dialogue_state_t;

typedef enum {
  AZ_MLS_INACTIVE = 0,  // not in monologue
  AZ_MLS_BEGIN,  // monologue box is opening
  AZ_MLS_BLANK, // monologue box is open, but no text shown
  AZ_MLS_TALK,  // narrator is talking
  AZ_MLS_WAIT,  // waiting for player to press enter
  AZ_MLS_END  // monologue box is closing
} az_monologue_step_t;

typedef struct {
  az_monologue_step_t step;
  double progress; // 0.0 to 1.0
  const char *paragraph;
  int paragraph_length;
  int chars_to_print;
} az_monologue_state_t;

/*===========================================================================*/

// Serialize the paragraph and return true, or return false on error (e.g. if
// file I/O fails).
bool az_write_paragraph(const char *paragraph, az_writer_t *writer);

// Parse and allocate the paragraph; return NULL on failure.
char *az_read_paragraph(az_reader_t *reader);

// Return the number of lines in the paragraph.  This will be at least 1,
// even for an empty string.
int az_paragraph_num_lines(const char *paragraph);

// Return the length, in printed characters, of the line in the paragraph
// starting at paragraph[start].
int az_paragraph_line_length(
    const az_preferences_t *prefs, const char *paragraph, int start);

// Return the total length, in printed characters, of the paragraph.
int az_paragraph_total_length(
    const az_preferences_t *prefs, const char *paragraph);

// Set the dialogue state for starting to display the given paragraph.
void az_set_dialogue_text(
    az_dialogue_state_t *dialogue, const az_preferences_t *prefs,
    const char *paragraph, bool top_turn);

/*===========================================================================*/

#endif // AZIMUTH_STATE_DIALOG_H_
