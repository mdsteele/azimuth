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

#include "azimuth/util/music.h"

#include <assert.h>
#include <math.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "azimuth/util/misc.h"
#include "azimuth/util/sound.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

#define MAX_NUM_PARTS 26
#define MAX_NOTES_PER_TRACK 2048
#define MAX_SPEC_LENGTH 256
#define HALF_STEPS_PER_OCTAVE 12

// Frequencies in Hz of each note in octave zero:
static const double pitch_frequencies[] = {
  16.3515978313, // c0
  17.3239144361, // c#0
  18.3540479948, // d0
  19.4454364826, // d#0
  20.6017223071, // e0
  21.8267644646, // f0
  23.1246514195, // f#0
  24.4997147489, // g0
  25.9565435987, // g#0
  27.5000000000, // a0
  29.1352350949, // a#0
  30.8677063285  // b0
};

AZ_STATIC_ASSERT(AZ_ARRAY_SIZE(pitch_frequencies) == HALF_STEPS_PER_OCTAVE);

/*===========================================================================*/

typedef struct {
  az_music_t *music;
  FILE *file;
  int current_char; // the most recent character read from the file
  int line, col;
  jmp_buf jump;
  int num_drums;
  const az_sound_data_t *drums;

  int spec_string_length;
  char spec_string[MAX_SPEC_LENGTH];

  az_music_part_t parts[MAX_NUM_PARTS];
  int part_indices[MAX_NUM_PARTS];  // maps from letter to index in parts

  struct {
    int num_notes;
    az_music_note_t notes[MAX_NOTES_PER_TRACK];
  } tracks[AZ_MUSIC_NUM_TRACKS];

  enum {
    BEGIN_LINE,
    NEW_NOTE,
    ADJUST_PITCH,
    CONTINUE_DURATION,
    BEGIN_DURATION,
    ADJUST_DURATION,
    AFTER_DIRECTIVE,
    LINE_COMMENT
  } state;

  double loudness_multiplier;
  double seconds_per_whole_note;
  int current_part;
  int current_track;
  int current_key; // -k = k flats, +k = k sharps
  int named_pitch; // -1 = rest, 0 = C, 1 = C#, ..., 11 = B
  int base_pitch; // named_pitch with current_key applied
  int pitch_adjust; // 1 = sharp, -1 = flat
  int octave;
  int global_transpose; // num half steps
  int local_transpose[AZ_MUSIC_NUM_TRACKS]; // num half steps
  const az_sound_data_t *current_drum;
  double base_duration; // 1 = whole note
  double extra_duration; // 1 = whole note
} az_music_parser_t;

az_music_parser_t *new_music_parser(
    az_music_t *music, FILE *file, int num_drums,
    const az_sound_data_t *drums) {
  assert(num_drums >= 0);
  assert(drums != NULL || num_drums == 0);
  az_music_parser_t *parser = AZ_ALLOC(1, az_music_parser_t);
  parser->music = music;
  parser->file = file;
  parser->line = 1;
  parser->num_drums = num_drums;
  parser->drums = drums;
  AZ_ARRAY_LOOP(part_index, parser->part_indices) *part_index = -1;
  parser->loudness_multiplier = 1.0;
  parser->seconds_per_whole_note = 2.0;
  parser->current_part = -1;
  parser->current_track = -1;
  return parser;
}

void free_music_parser(az_music_parser_t *parser) {
  AZ_ARRAY_LOOP(part, parser->parts) {
    AZ_ARRAY_LOOP(track, part->tracks) {
      free(track->notes);
    }
  }
  free(parser);
}

/*===========================================================================*/

#define TRY_READ(...) \
  (fscanf(parser->file, __VA_ARGS__) == AZ_COUNT_ARGS(__VA_ARGS__) - 1)

#define PARSE_ERROR(...) do { \
    fprintf(stderr, "line %d: parse error: ", parser->line); \
    fprintf(stderr, __VA_ARGS__); \
    longjmp(parser->jump, 1); \
  } while (false)

static int next_char(az_music_parser_t *parser) {
  const int ch = fgetc(parser->file);
  if (parser->current_char == '\n') ++parser->line;
  parser->current_char = ch;
  return ch;
}

static void parse_music_header(az_music_parser_t *parser) {
  if (!TRY_READ("@M \"")) PARSE_ERROR("invalid file header\n");
  assert(parser->spec_string_length == 0);
  while (true) {
    const int ch = fgetc(parser->file);
    if (ch == EOF) PARSE_ERROR("EOF in middle of spec string\n");
    if (ch == '"') break;
    if (!(ch == '|' || ('A' <= ch && ch <= 'Z'))) {
      PARSE_ERROR("invalid char in spec string: '%c'\n", ch);
    }
    if (parser->spec_string_length >= MAX_SPEC_LENGTH) {
      PARSE_ERROR("music spec is too long\n");
    }
    parser->spec_string[parser->spec_string_length++] = ch;
  }
  parser->state = AFTER_DIRECTIVE;
}

static void begin_line(az_music_parser_t *parser) {
  parser->state = BEGIN_LINE;
  parser->named_pitch = 0;
  parser->base_pitch = 0;
  parser->pitch_adjust = 0;
  parser->octave = 4;
  parser->base_duration = 0.25; // quarter note
  parser->extra_duration = 0.0;
}

static void parse_directive(az_music_parser_t *parser) {
  switch (next_char(parser)) {
    case 'k': {
      char num_char, accidental;
      if (!TRY_READ("ey %c%c", &num_char, &accidental)) goto invalid;
      if (num_char < '0' || num_char > '7') {
        PARSE_ERROR("invalid key number: '%c'\n", num_char);
      }
      const int number = num_char - '0';
      switch (accidental) {
        case '#': parser->current_key = number; break;
        case 'b': parser->current_key = -number; break;
        case 'N': parser->current_key = 0; break;
        default: PARSE_ERROR("invalid key accidental: '%c'\n", accidental);
      }
    } break;
    case 'l': {
      double loudness;
      if (!TRY_READ("oudness %lf", &loudness)) goto invalid;
      if (loudness <= 0.0) {
        PARSE_ERROR("loudness must be positive\n");
      }
      parser->loudness_multiplier = loudness;
    } break;
    case 't':
      switch (next_char(parser)) {
        case 'e': {
          double quarter_notes_per_minute;
          if (!TRY_READ("mpo %lf", &quarter_notes_per_minute)) goto invalid;
          if (quarter_notes_per_minute <= 0.0) {
            PARSE_ERROR("tempo must be positive\n");
          }
          parser->seconds_per_whole_note = 240.0 / quarter_notes_per_minute;
        } break;
        case 'r': {
          int half_steps;
          if (!TRY_READ("anspose %d", &half_steps)) goto invalid;
          parser->global_transpose = half_steps;
        } break;
        default: goto invalid;
      }
      break;
    default:
    invalid:
      PARSE_ERROR("invalid directive\n");
  }
  parser->current_track = -1;
  parser->state = AFTER_DIRECTIVE;
}

static void finish_part(az_music_parser_t *parser) {
  if (parser->current_part < 0) return;
  assert(parser->current_part < MAX_NUM_PARTS);
  az_music_part_t *part = &parser->parts[parser->current_part];
  for (int i = 0; i < AZ_MUSIC_NUM_TRACKS; ++i) {
    az_music_track_t *track = &part->tracks[i];
    track->num_notes = parser->tracks[i].num_notes;
    track->notes = AZ_ALLOC(track->num_notes, az_music_note_t);
    memcpy(track->notes, parser->tracks[i].notes,
           track->num_notes * sizeof(az_music_note_t));
  }
  AZ_ZERO_ARRAY(parser->tracks);
  AZ_ZERO_ARRAY(parser->local_transpose);
}

static void parse_part_heading(az_music_parser_t *parser) {
  char letter;
  if (!TRY_READ("Part %c", &letter)) PARSE_ERROR("invalid part header\n");
  if (letter < 'A' || letter > 'Z') {
    PARSE_ERROR("invalid part name: '%c'\n", letter);
  }
  if (parser->part_indices[letter - 'A'] >= 0) {
    PARSE_ERROR("reused part name: '%c'\n", letter);
  }
  finish_part(parser);
  ++parser->current_part;
  if (parser->current_part >= MAX_NUM_PARTS) PARSE_ERROR("too many parts\n");
  parser->part_indices[letter - 'A'] = parser->current_part;
  parser->current_track = -1;
  parser->state = AFTER_DIRECTIVE;
}

static az_music_note_t *next_note(az_music_parser_t *parser) {
  assert(parser->current_part >= 0);
  assert(parser->current_part < MAX_NUM_PARTS);
  assert(parser->current_track >= 0);
  assert(parser->current_track < AZ_MUSIC_NUM_TRACKS);
  if (parser->tracks[parser->current_track].num_notes >=
      MAX_NOTES_PER_TRACK) {
    PARSE_ERROR("too many notes on track %d\n", parser->current_track);
  }
  parser->state = NEW_NOTE;
  return &parser->tracks[parser->current_track]
    .notes[parser->tracks[parser->current_track].num_notes++];
}

static void parse_dutymod(az_music_parser_t *parser) {
  az_music_note_t *note = next_note(parser);
  note->type = AZ_NOTE_DUTYMOD;
  double depth, speed;
  if (!TRY_READ("%lf,%lf", &depth, &speed)) {
    PARSE_ERROR("invalid Dutymod params\n");
  }
  note->attributes.dutymod.depth = depth * 0.01;
  note->attributes.dutymod.speed = fmax(0.0, speed);
}

static void parse_envelope(az_music_parser_t *parser) {
  az_music_note_t *note = next_note(parser);
  note->type = AZ_NOTE_ENVELOPE;
  double attack, decay;
  if (!TRY_READ("%lf,%lf", &attack, &decay)) {
    PARSE_ERROR("invalid Envelope params\n");
  }
  note->attributes.envelope.attack_time =
    fmax(0.0, 0.0025 * parser->seconds_per_whole_note * attack);
  note->attributes.envelope.decay_fraction =
    fmin(fmax(0.0, decay * 0.01), 1.0);
}

static void parse_loudness(az_music_parser_t *parser) {
  az_music_note_t *note = next_note(parser);
  note->type = AZ_NOTE_LOUDNESS;
  double volume;
  if (!TRY_READ("%lf", &volume)) PARSE_ERROR("invalid Loudness param\n");
  note->attributes.loudness.volume =
    parser->loudness_multiplier * fmax(0.0, volume / 100.0);
}

static void parse_transpose(az_music_parser_t *parser) {
  assert(parser->current_track >= 0);
  assert(parser->current_track < AZ_MUSIC_NUM_TRACKS);
  int half_steps;
  if (!TRY_READ("%d", &half_steps)) PARSE_ERROR("invalid Transpose param\n");
  parser->local_transpose[parser->current_track] = half_steps;
}

static void parse_vibrato(az_music_parser_t *parser) {
  az_music_note_t *note = next_note(parser);
  note->type = AZ_NOTE_VIBRATO;
  double depth, speed;
  if (!TRY_READ("%lf,%lf", &depth, &speed)) {
    PARSE_ERROR("invalid Vibrato params\n");
  }
  note->attributes.vibrato.depth = depth * 0.01;
  note->attributes.vibrato.speed = fmax(0.0, speed);
}

static void parse_waveform(az_music_parser_t *parser) {
  az_music_note_t *note = next_note(parser);
  note->type = AZ_NOTE_WAVEFORM;
  const char ch = next_char(parser);
  if (ch == 'p' || ch == 't') {
    note->attributes.waveform.kind =
      (ch == 'p' ? AZ_SQUARE_WAVE : AZ_TRIANGLE_WAVE);
    double duty;
    if (!TRY_READ("%lf", &duty)) PARSE_ERROR("invalid waveform duty\n");
    note->attributes.waveform.duty = fmin(fmax(0.0, duty / 100.0), 1.0);
  } else if (ch == 'n') {
    note->attributes.waveform.kind = AZ_NOISE_WAVE;
  } else if (ch == 's') {
    note->attributes.waveform.kind = AZ_SINE_WAVE;
  } else PARSE_ERROR("invalid waveform kind: '%c'\n", ch);
}

static void start_pitch(az_music_parser_t *parser, int named_pitch,
                        int flat_key, int sharp_key) {
  if (parser->current_part < 0) {
    PARSE_ERROR("can't start pitch outside of any part\n");
  }
  if (parser->current_track < 0) {
    PARSE_ERROR("can't start pitch before setting the current track\n");
  }
  parser->named_pitch = named_pitch;
  parser->base_pitch = named_pitch;
  if (parser->current_key <= flat_key) --parser->base_pitch;
  else if (parser->current_key >= sharp_key) ++parser->base_pitch;
  parser->pitch_adjust = 0;
  parser->state = ADJUST_PITCH;
}

static void start_drum(az_music_parser_t *parser) {
  if (parser->current_part < 0) {
    PARSE_ERROR("can't start drum outside of any part\n");
  }
  if (parser->current_track < 0) {
    PARSE_ERROR("can't start drum before setting the current track\n");
  }
  int drum_number;
  if (!TRY_READ("%d", &drum_number) ||
      drum_number < 0 || drum_number >= parser->num_drums) {
    PARSE_ERROR("invalid drum number\n");
  }
  parser->current_drum = &parser->drums[drum_number];
  parser->state = BEGIN_DURATION;
}

static void emit_note(az_music_parser_t *parser) {
  az_music_note_t *note = next_note(parser);
  parser->base_duration += parser->extra_duration;
  parser->extra_duration = 0.0;
  const double duration_seconds =
    parser->seconds_per_whole_note * parser->base_duration;
  if (parser->current_drum != NULL) {
    note->type = AZ_NOTE_DRUM;
    note->attributes.drum.duration = duration_seconds;
    note->attributes.drum.data = parser->current_drum;
    parser->current_drum = NULL;
  } else if (parser->named_pitch < 0) {
    note->type = AZ_NOTE_REST;
    note->attributes.rest.duration = duration_seconds;
  } else {
    note->type = AZ_NOTE_TONE;
    const int absolute_pitch =
      az_imax(0, parser->base_pitch + parser->pitch_adjust +
              HALF_STEPS_PER_OCTAVE * parser->octave +
              parser->global_transpose +
              parser->local_transpose[parser->current_track]);
    const int octave = absolute_pitch / HALF_STEPS_PER_OCTAVE;
    const int pitch = absolute_pitch % HALF_STEPS_PER_OCTAVE;
    note->attributes.tone.frequency = pitch_frequencies[pitch] * (1 << octave);
    note->attributes.tone.duration = duration_seconds;
  }
}

static void parse_music_data(az_music_parser_t *parser) {
  while (next_char(parser) != EOF) {
    const char ch = parser->current_char;
    switch (parser->state) {
      case BEGIN_LINE:
        switch (ch) {
          case '%': parser->state = LINE_COMMENT; break;
          case ' ': parser->state = NEW_NOTE; break;
          case '\n': begin_line(parser); break;
          case '!': parse_part_heading(parser); break;
          case '=': parse_directive(parser); break;
          case '1': case '2': case '3': case '4': case '5':
            if (parser->current_part < 0) {
              PARSE_ERROR("can't set track outside of any part\n");
            }
            parser->current_track = ch - '1';
            parser->state = NEW_NOTE;
            break;
          default: PARSE_ERROR("invalid char at start-of-line: '%c'\n", ch);
        }
        break;
      case NEW_NOTE:
        switch (ch) {
          case '%': parser->state = LINE_COMMENT; break;
          case 'D': parse_dutymod(parser); break;
          case 'E': parse_envelope(parser); break;
          case 'L': parse_loudness(parser); break;
          case 'T': parse_transpose(parser); break;
          case 'V': parse_vibrato(parser); break;
          case 'W': parse_waveform(parser); break;
          case 'a': start_pitch(parser,  9, -3, 5); break;
          case 'b': start_pitch(parser, 11, -1, 7); break;
          case 'c': start_pitch(parser,  0, -6, 2); break;
          case 'd': start_pitch(parser,  2, -4, 4); break;
          case 'e': start_pitch(parser,  4, -2, 6); break;
          case 'f': start_pitch(parser,  5, -7, 1); break;
          case 'g': start_pitch(parser,  7, -5, 3); break;
          case 'r': start_pitch(parser, -1, -9, 9); break;
          case 'x': start_drum(parser); break;
          case ' ': case '|': case '\'': break;
          case '\n': begin_line(parser); break;
          default: PARSE_ERROR("invalid char at start-of-note: '%c'\n", ch);
        }
        break;
      case ADJUST_PITCH:
        switch (ch) {
          case '%': emit_note(parser); parser->state = LINE_COMMENT; break;
          case '#':
            parser->base_pitch = parser->named_pitch;
            ++parser->pitch_adjust;
            break;
          case 'b':
            parser->base_pitch = parser->named_pitch;
            --parser->pitch_adjust;
            break;
          case 'N':
            parser->base_pitch = parser->named_pitch;
            parser->pitch_adjust = 0;
            break;
          case '0': case '1': case '2': case '3': case '4':
          case '5': case '6': case '7': case '8': case '9':
            parser->octave = ch - '0';
            parser->state = BEGIN_DURATION;
            break;
          case 'w': case 'h': case 'q': case 'e':
          case 's': case 't': case 'x':
            goto begin_duration;
          case ' ': case '|': case '\'':
            emit_note(parser);
            break;
          case '\n':
            emit_note(parser);
            begin_line(parser);
            break;
          default: PARSE_ERROR("invalid char within note: '%c'\n", ch);
        }
        break;
      case CONTINUE_DURATION:
        switch (ch) {
          case 'w': case 'h': case 'q': case 'e':
          case 's': case 't': case 'x':
            goto begin_duration;
          case '+': break;
          case ' ': case '|': case '\'': break;
          default:
            PARSE_ERROR("invalid duration continuation char: '%c'\n", ch);
        }
        break;
      case BEGIN_DURATION:
      begin_duration:
        switch (ch) {
          case 'w': parser->base_duration = 1 /  1.0;
            parser->state = ADJUST_DURATION; break;
          case 'h': parser->base_duration = 1 /  2.0;
            parser->state = ADJUST_DURATION; break;
          case 'q': parser->base_duration = 1 /  4.0;
            parser->state = ADJUST_DURATION; break;
          case 'e': parser->base_duration = 1 /  8.0;
            parser->state = ADJUST_DURATION; break;
          case 's': parser->base_duration = 1 / 16.0;
            parser->state = ADJUST_DURATION; break;
          case 't': parser->base_duration = 1 / 32.0;
            parser->state = ADJUST_DURATION; break;
          case 'x': parser->base_duration = 1 / 64.0;
            parser->state = ADJUST_DURATION; break;
          case ' ': case '|': case '\'':
            emit_note(parser);
            break;
          case '\n':
            emit_note(parser);
            begin_line(parser);
            break;
          default: PARSE_ERROR("invalid duration char: '%c'\n", ch);
        }
        break;
      case ADJUST_DURATION:
        switch (ch) {
          case '.':
            parser->extra_duration += parser->base_duration;
            parser->base_duration *= 0.5;
            break;
          case '3':
            parser->base_duration /= 1.5;
            break;
          case '5':
            parser->base_duration /= 1.25;
            break;
          case '+':
            parser->extra_duration += parser->base_duration;
            parser->base_duration = 0.0;
            parser->state = CONTINUE_DURATION;
            break;
          case ' ': case '|': case '\'':
            emit_note(parser);
            break;
          case '\n':
            emit_note(parser);
            begin_line(parser);
            break;
          default: PARSE_ERROR("invalid duration adjustment char: '%c'\n", ch);
        }
        break;
      case AFTER_DIRECTIVE:
        switch (ch) {
          case '%': parser->state = LINE_COMMENT; break;
          case ' ': break;
          case '\n': begin_line(parser); break;
          default: PARSE_ERROR("invalid char after directive: '%c'\n", ch);
        }
        break;
      case LINE_COMMENT:
        switch (ch) {
          case '\n': begin_line(parser); break;
          default: break;
        }
        break;
    }
  }
}

static void parse_spec_string(az_music_parser_t *parser) {
  parser->music->spec_length = 0;
  parser->music->loop_point = -1;
  for (int i = 0; i < parser->spec_string_length; ++i) {
    if (parser->spec_string[i] == '|') {
      if (parser->music->loop_point >= 0) {
        PARSE_ERROR("can't have more than one loop point in spec\n");
      }
      parser->music->loop_point = i;
    } else ++parser->music->spec_length;
  }
  if (parser->music->loop_point < 0) parser->music->loop_point = 0;
  parser->music->spec = AZ_ALLOC(parser->music->spec_length, int);
  for (int i = 0, j = 0; j < parser->spec_string_length; ++j) {
    const char ch = parser->spec_string[j];
    if (ch == '|') continue;
    assert('A' <= ch && ch <= 'Z');
    const int part_index = parser->part_indices[ch - 'A'];
    if (part_index < 0) {
      PARSE_ERROR("spec mentions nonexistent part %c\n", ch);
    }
    assert(part_index <= parser->current_part);
    parser->music->spec[i++] = part_index;
  }
}

static bool parse_music(az_music_parser_t *parser) {
  if (setjmp(parser->jump) != 0) {
    az_destroy_music(parser->music);
    return false;
  }
  parse_music_header(parser);
  parse_music_data(parser);
  finish_part(parser);
  parse_spec_string(parser);
  const size_t num_parts = parser->current_part + 1;
  parser->music->num_parts = num_parts;
  parser->music->parts = AZ_ALLOC(num_parts, az_music_part_t);
  memcpy(parser->music->parts, parser->parts,
         num_parts * sizeof(az_music_part_t));
  AZ_ZERO_ARRAY(parser->parts);
  return true;
}

/*===========================================================================*/

bool az_parse_music_from_file(
    const char *filepath, int num_drums, const az_sound_data_t *drums,
    az_music_t *music_out) {
  assert(music_out != NULL);
  AZ_ZERO_OBJECT(music_out);
  FILE *file = fopen(filepath, "r");
  if (file == NULL) return false;
  az_music_parser_t *parser =
    new_music_parser(music_out, file, num_drums, drums);
  const bool success = parse_music(parser);
  free_music_parser(parser);
  fclose(file);
  return success;
}

void az_destroy_music(az_music_t *music) {
  if (music == NULL) return;
  for (int p = 0; p < music->num_parts; ++p) {
    az_music_part_t *part = &music->parts[p];
    AZ_ARRAY_LOOP(track, part->tracks) free(track->notes);
  }
  free(music->parts);
  free(music->spec);
  AZ_ZERO_OBJECT(music);
}

/*===========================================================================*/

#define SECONDS_PER_SAMPLE (1.0 / (double)(AZ_AUDIO_RATE))
// How many virtual samples to average together for each sample array entry:
#define SYNTH_SUPERSAMPLE 8
// The wave amplitude produced by an L100 note:
#define BASE_LOUDNESS 5000.0

static uint64_t generate_noise(void) {
  // This is a simple linear congruential generator, using the parameters
  // suggested by http://nuclear.llnl.gov/CNP/rng/rngman/node4.html
  static uint64_t seed = UINT64_C(123456789123456789);
  seed = UINT64_C(2862933555777941757) * seed + UINT64_C(3037000493);
  return seed;
}

static void synth_begin_next_part(az_music_synth_t *synth) {
  const az_music_t *music = synth->music;
  assert(music != NULL);
  ++synth->spec_index;
  if (synth->spec_index >= music->spec_length) {
    synth->spec_index = music->loop_point;
  }
  assert(synth->spec_index >= 0);
  assert(synth->spec_index < music->spec_length);
  const int part_index = music->spec[synth->spec_index];
  assert(part_index >= 0);
  assert(part_index < music->num_parts);
  const az_music_part_t *part = &music->parts[part_index];
  for (int i = 0; i < AZ_MUSIC_NUM_TRACKS; ++i) {
    synth->voices[i].track = &part->tracks[i];
    synth->voices[i].note_index = 0;
    synth->voices[i].time_from_note_start = 0.0;
  }
}

static void synth_advance(az_music_synth_t *synth) {
  while (true) {
    int num_tracks_finished = 0;
    AZ_ARRAY_LOOP(voice, synth->voices) {
      while (true) {
        if (voice->note_index >= voice->track->num_notes) {
          ++num_tracks_finished;
          break;
        }
        const az_music_note_t *note = &voice->track->notes[voice->note_index];
        switch (note->type) {
          case AZ_NOTE_REST:
            if (voice->time_from_note_start < note->attributes.rest.duration) {
              goto sustain;
            }
            voice->time_from_note_start -= note->attributes.rest.duration;
            break;
          case AZ_NOTE_TONE:
            if (voice->time_from_note_start < note->attributes.tone.duration) {
              goto sustain;
            }
            voice->time_from_note_start -= note->attributes.tone.duration;
            break;
          case AZ_NOTE_DRUM:
            if (voice->time_from_note_start <
                note->attributes.drum.duration) {
              goto sustain;
            }
            voice->time_from_note_start -= note->attributes.drum.duration;
            break;
          case AZ_NOTE_DUTYMOD:
            voice->dutymod_depth = note->attributes.dutymod.depth;
            voice->dutymod_speed = note->attributes.dutymod.speed;
            break;
          case AZ_NOTE_ENVELOPE:
            voice->attack_time = note->attributes.envelope.attack_time;
            voice->decay_fraction = note->attributes.envelope.decay_fraction;
            break;
          case AZ_NOTE_LOUDNESS:
            voice->loudness = BASE_LOUDNESS * note->attributes.loudness.volume;
            assert(voice->loudness >= 0.0);
            break;
          case AZ_NOTE_VIBRATO:
            voice->vibrato_depth = note->attributes.vibrato.depth;
            voice->vibrato_speed = note->attributes.vibrato.speed;
            break;
          case AZ_NOTE_WAVEFORM:
            voice->waveform = note->attributes.waveform.kind;
            voice->duty = note->attributes.waveform.duty;
            break;
        }
        ++voice->note_index;
        voice->drum_index = 0;
      }
    sustain:;
    }
    if (num_tracks_finished < AZ_MUSIC_NUM_TRACKS) return;
    synth_begin_next_part(synth);
  }
}

void az_reset_music_synth(az_music_synth_t *synth, const az_music_t *music) {
  assert(synth != NULL);
  AZ_ZERO_OBJECT(synth);
  if (music == NULL) return;
  synth->music = music;
  AZ_ARRAY_LOOP(voice, synth->voices) {
    voice->waveform = AZ_SQUARE_WAVE;
    voice->duty = 0.5;
    voice->loudness = BASE_LOUDNESS;
    voice->noise_bits = generate_noise();
  }
  synth->spec_index = -1;
  synth_begin_next_part(synth);
  synth_advance(synth);
}

void az_synthesize_music(az_music_synth_t *synth, int16_t *samples,
                         int num_samples) {
  assert(synth != NULL);
  if (synth->music == NULL) {
    memset(samples, 0, num_samples * sizeof(int16_t));
    return;
  }
  for (int sample_index = 0; sample_index < num_samples; ++sample_index) {
    int sample = 0;
    AZ_ARRAY_LOOP(voice, synth->voices) {
      if (voice->loudness <= 0.0) continue;
      const az_music_track_t *track = voice->track;
      if (voice->note_index >= track->num_notes) continue;
      const az_music_note_t *note = &track->notes[voice->note_index];
      if (note->type == AZ_NOTE_TONE) {
        const double vibrato = 1.0 + voice->vibrato_depth *
          sin(voice->time_from_note_start * voice->vibrato_speed);
        const double period =
          1.0 / (note->attributes.tone.frequency * vibrato);
        const int iperiod = AZ_AUDIO_RATE * SYNTH_SUPERSAMPLE * period;
        double amplitude = 0.0;
        for (int s = 0; s < SYNTH_SUPERSAMPLE; ++s) {
          ++voice->iphase;
          if (voice->iphase >= iperiod) {
            voice->iphase %= iperiod;
            if (voice->waveform == AZ_NOISE_WAVE) {
              voice->noise_bits = generate_noise();
            }
          }
          const double phase = (double)voice->iphase / (double)iperiod;
          const double duty = voice->duty *
            (1.0 + voice->dutymod_depth *
             sin(voice->time_from_note_start * voice->dutymod_speed));
          switch (voice->waveform) {
            case AZ_NOISE_WAVE:
              amplitude += ((voice->noise_bits >>
                             (64 * voice->iphase / iperiod)) & 0x1 ?
                            1.0 : -1.0);
              break;
            case AZ_SINE_WAVE:
              amplitude += sin(phase * AZ_TWO_PI);
              break;
            case AZ_SQUARE_WAVE:
              amplitude += (phase < duty ? 1.0 : -1.0);
              break;
            case AZ_TRIANGLE_WAVE:
              amplitude += (phase < duty ? 2.0 * (phase / duty) - 1.0 :
                            1.0 - 2.0 * ((phase - duty) / (1.0 - duty)));
              break;
            case AZ_SAWTOOTH_WAVE:
            case AZ_WOBBLE_WAVE:
              AZ_ASSERT_UNREACHABLE();
          }
        }
        const double decay_time =
          note->attributes.tone.duration * voice->decay_fraction;
        assert(decay_time >= 0.0);
        const double time_remaining =
          note->attributes.tone.duration - voice->time_from_note_start;
        assert(time_remaining > 0.0);
        const double envelope =
          (voice->time_from_note_start < voice->attack_time ?
           voice->time_from_note_start / voice->attack_time : 1.0) *
          (time_remaining < decay_time ? time_remaining / decay_time : 1.0);
        sample += (amplitude / SYNTH_SUPERSAMPLE) * envelope * voice->loudness;
      } else if (note->type == AZ_NOTE_DRUM) {
        const az_sound_data_t *data = note->attributes.drum.data;
        if (voice->drum_index < data->num_samples) {
          sample += ((1.0 / BASE_LOUDNESS) * voice->loudness *
                     data->samples[voice->drum_index]);
          ++voice->drum_index;
        }
      } else assert(note->type == AZ_NOTE_REST);
    }
    samples[sample_index] = az_imin(az_imax(INT16_MIN, sample), INT16_MAX);

    AZ_ARRAY_LOOP(voice, synth->voices) {
      if (voice->note_index < voice->track->num_notes) {
        voice->time_from_note_start += SECONDS_PER_SAMPLE;
      }
    }
    synth_advance(synth);
  }
}

/*===========================================================================*/
