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

#include "azimuth/state/sound.h"

#include <assert.h>
#include <stdlib.h>

#include "azimuth/util/audio.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/sound.h"

/*===========================================================================*/

static az_sound_spec_t sound_specs[] = {
  [AZ_SND_BEAM_FREEZE] = {
    .wave_kind = AZ_TRIANGLE_WAVE,
    .env_sustain = 1.0, .start_freq = 0.3943662,
    .vibrato_depth = 0.1056338, .vibrato_speed = 0.57042253,
    .square_duty = 0.35298
  },
  [AZ_SND_BEAM_NORMAL] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_sustain = 0.1971831, .start_freq = 0.2943662,
    .vibrato_depth = 0.1056338, .vibrato_speed = 0.57042253,
    .square_duty = 0.35298
  },
  [AZ_SND_BEAM_PHASE] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_sustain = 0.1971831, .start_freq = 0.3443662,
    .vibrato_depth = 0.1056338, .vibrato_speed = 0.57042253,
    .square_duty = 0.35298
  },
  [AZ_SND_BEAM_PIERCE] = {
    .wave_kind = AZ_SAWTOOTH_WAVE,
    .env_sustain = 0.1971831, .start_freq = 0.3943662,
    .vibrato_depth = 0.1056338, .vibrato_speed = 0.57042253,
    .square_duty = 0.35298
  },
  [AZ_SND_BLINK_MEGA_BOMB] = {
    .wave_kind = AZ_SAWTOOTH_WAVE,
    .env_sustain = 0.119718313217,
    .env_decay = 0.0704225376248,
    .start_freq = 0.41549295187,
    .square_duty = 0.19812,
  },
  [AZ_SND_CHARGED_GUN] = {
    .wave_kind = AZ_SAWTOOTH_WAVE,
    .env_sustain = 0.323943674564,
    .start_freq = 0.232394367456, .freq_slide = 0.004733738,
    .vibrato_depth = 0.33098590374, .vibrato_speed = 0.33098590374,
    .arp_speed = -0.4154, .square_duty = -0.8286, .phaser_offset = -0.7323943,
    .volume_adjust = -0.462962989807
  },
  [AZ_SND_CHARGED_MISSILE_BEAM] = {
    .wave_kind = AZ_WOBBLE_WAVE,
    .env_decay = 0.563380300999,
    .start_freq = 0.373239427805, .freq_slide = 0.1549295187,
    .vibrato_depth = 0.760563373566, .vibrato_speed = 0.66197180748,
    .phaser_offset = 0.718309879303, .phaser_sweep = 0.436619758606,
    .lpf_cutoff = 0.6768399, .hpf_cutoff = 0.036
  },
  [AZ_SND_CHARGED_ORDNANCE] = {
    .wave_kind = AZ_TRIANGLE_WAVE,
    .env_sustain = 0.6,
    .start_freq = 0.202394367456, .freq_slide = 0.004733738,
    .vibrato_depth = 0.33098590374, .vibrato_speed = 0.25,
    .arp_speed = -0.4154, .square_duty = -0.8286, .phaser_offset = -0.7323943,
    .volume_adjust = -0.462962989807
  },
  [AZ_SND_CHARGING_GUN] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_attack = 1.0, .env_sustain = 0.34507, .env_decay = 0.246478870511,
    .start_freq = 0.1197183, .freq_slide = 0.118591,
    .freq_delta_slide = 0.056338,
    .square_duty = 0.40368, .duty_sweep = 0.0140844583511
  },
  [AZ_SND_CHARGING_ORDNANCE] = {
    .wave_kind = AZ_TRIANGLE_WAVE,
    .env_attack = 0.2, .env_sustain = 1.0,
    .start_freq = 0.0897183, .freq_slide = 0.118591,
    .freq_delta_slide = 0.056338,
    .square_duty = 0.40368, .duty_sweep = 0.0140844583511,
    .volume_adjust = -0.4
  },
  [AZ_SND_CLOAK_BEGIN] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_sustain = 0.352112680674, .env_decay = 0.27622,
    .start_freq = 0.146478870511, .freq_slide = 0.12,
    .vibrato_depth = 0.2, .vibrato_speed = 0.2,
    .square_duty = 0.53694, .volume_adjust = -0.7
  },
  [AZ_SND_CLOAK_END] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_sustain = 0.352112680674, .env_decay = 0.27622,
    .start_freq = 0.201408463717, .freq_slide = -0.18,
    .vibrato_depth = 0.2, .vibrato_speed = 0.2,
    .square_duty = 0.53694, .volume_adjust = -0.7
  },
  [AZ_SND_CPLUS_ACTIVE] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.2827, .env_punch = 0.61646, .env_decay = 1.0,
    .start_freq = 0.443661957979, .freq_delta_slide = -0.25352114439,
    .vibrato_depth = 0.27069, .vibrato_speed = 0.00366,
    .arp_mod = 0.253521084785, .arp_speed = 0.65505,
    .repeat_speed = 0.732394337654,
    .phaser_offset = -0.04278001, .phaser_sweep = -0.112676084042
  },
  [AZ_SND_CPLUS_CHARGED] = {
    .wave_kind = AZ_WOBBLE_WAVE,
    .env_sustain = 0.6126761, .env_punch = 0.08549776, .env_decay = 0.204225,
    .start_freq = 0.84507, .freq_slide = 0.00128, .freq_delta_slide = 0.11465,
    .vibrato_depth = 0.4195169, .vibrato_speed = -0.39,
    .arp_mod = 0.1044, .arp_speed = -0.5518,
    .square_duty = 0.189, .duty_sweep = 0.01924883,
    .phaser_offset = -0.4636848, .phaser_sweep = 0.02882149,
    .lpf_cutoff = 0.5151533, .lpf_ramp = 0.4815216, .lpf_resonance = 0.312,
    .hpf_cutoff = 0.6101004, .hpf_ramp = -0.01561149, .volume_adjust = -0.222
  },
  [AZ_SND_CPLUS_IMPACT] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.33175, .env_punch = 0.53942, .env_decay = 0.788732409477,
    .start_freq = 0.1128691, .freq_slide = 0.05820001,
    .phaser_offset = 0.47598, .phaser_sweep = -0.25746
  },
  [AZ_SND_CPLUS_READY] = {
    .wave_kind = AZ_TRIANGLE_WAVE,
    .env_sustain = 0.6, .env_punch = 0.1355418,
    .start_freq = 0.5106531, .freq_delta_slide = 0.5334117,
    .vibrato_depth = -0.0002874958, .vibrato_speed = -0.161,
    .arp_mod = 0.9934, .arp_speed = -0.331,
    .square_duty = 0.8678, .duty_sweep = 0.000924009, .repeat_speed = 0.663,
    .phaser_offset = 0.0006630559, .phaser_sweep = -0.3438828,
    .lpf_cutoff = 0.4502442, .lpf_resonance = -0.4142,
    .hpf_cutoff = 0.0008809352, .hpf_ramp = -0.2542205, .volume_adjust = -0.5
  },
  [AZ_SND_DOOR_CLOSE] = {
    .wave_kind = AZ_WOBBLE_WAVE,
    .env_sustain = 0.352112680674, .env_decay = 0.12622,
    .start_freq = 0.401408463717, .freq_slide = -0.197183,
    .square_duty = 0.53694, .volume_adjust = -0.5
  },
  [AZ_SND_DOOR_OPEN] = {
    .wave_kind = AZ_WOBBLE_WAVE,
    .env_sustain = 0.352112680674, .env_decay = 0.12622,
    .start_freq = 0.246478870511, .freq_slide = 0.197183,
    .square_duty = 0.53694, .volume_adjust = -0.5
  },
  [AZ_SND_DRILLING] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.74, .start_freq = 0.1056338,
    .vibrato_depth = 0.640845, .vibrato_speed = 0.56338,
    .volume_adjust = -0.5
  },
  [AZ_SND_DROP_BOMB] = {
    .wave_kind = AZ_SINE_WAVE,
    .env_sustain = 0.15671, .env_decay = 0.00384,
    .start_freq = 0.380281686783, .square_duty = 0.36672,
    .hpf_cutoff = 0.1, .volume_adjust = -0.314814834595
  },
  [AZ_SND_EXPLODE_BOMB] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.32941, .env_punch = 0.572, .env_decay = 0.23305,
    .start_freq = 0.1548265, .freq_slide = -0.36946,
    .vibrato_depth = 0.63161, .vibrato_speed = 0.38466,
    .phaser_offset = 0.12768, .phaser_sweep = -0.28872
  },
  [AZ_SND_EXPLODE_HYPER_ROCKET] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.31483, .env_punch = 0.26024, .env_decay = 0.4112,
    .start_freq = 0.161971837282, .repeat_speed = 0.3807,
    .volume_adjust = 0.333333435059
  },
  [AZ_SND_EXPLODE_MEGA_BOMB] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.4366197, .env_punch = 0.443, .env_decay = 0.3309859,
    .start_freq = 0.161395, .freq_slide = -0.0140845179558,
    .arp_mod = -0.5244799, .arp_speed = 0.74647885561, .repeat_speed = 0.59345
  },
  [AZ_SND_EXPLODE_ROCKET] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.29941, .env_punch = 0.69464, .env_decay = 0.3665,
    .start_freq = 0.6008111, .freq_slide = -0.25694,
    .repeat_speed = 0.7266001, .volume_adjust = 0.222222213745
  },
  [AZ_SND_EXPLODE_SHIP] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.852112650, .env_punch = 1.0, .env_decay = 0.767605662,
    .start_freq = 0.112676054239, .freq_slide = -0.0985915660858,
    .vibrato_depth = 0.281690150499, .vibrato_speed = 0.12924
  },
  [AZ_SND_FIRE_GUN_CHARGED_BEAM] = {
    .wave_kind = AZ_SAWTOOTH_WAVE,
    .env_sustain = 0.36394, .env_decay = 0.274647891521,
    .start_freq = 0.1338, .freq_slide = 0.2676, .freq_delta_slide = -0.140845,
    .phaser_offset = -0.323943674564, .phaser_sweep = 0.436619758606
  },
  [AZ_SND_FIRE_GUN_FREEZE] = {
    .wave_kind = AZ_WOBBLE_WAVE,
    .env_sustain = 0.13848, .env_punch = 0.12036, .env_decay = 0.21536,
    .start_freq = 0.9788732, .freq_limit = 0.32917, .freq_slide = -0.22554,
    .square_duty = 0.46895, .duty_sweep = 0.1842,
    .hpf_cutoff = 0.007470001
  },
  [AZ_SND_FIRE_GUN_NORMAL] = {
    .wave_kind = AZ_SINE_WAVE,
    .env_sustain = 0.10346, .env_punch = 0.16836, .env_decay = 0.18956,
    .start_freq = 0.89034, .freq_limit = 0.08224, .freq_slide = -0.61922,
    .square_duty = 0.2347, .duty_sweep = 0.0176, .hpf_cutoff = 0.25965
  },
  [AZ_SND_FIRE_GUN_PIERCE] = {
    .wave_kind = AZ_SAWTOOTH_WAVE,
    .env_sustain = 0.23076, .env_decay = 0.05284,
    .start_freq = 0.7278, .freq_limit = 0.29536, .freq_slide = -0.29848,
    .square_duty = 0.46265, .duty_sweep = -0.18305
  },
  [AZ_SND_FIRE_HYPER_ROCKET] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.3, .env_punch = 0.25, .env_decay = 0.5,
    .start_freq = 0.2, .vibrato_depth = 0.5, .vibrato_speed = 0.75,
    .repeat_speed = 0.55, .phaser_offset = 0.25, .phaser_sweep = -0.25
  },
  [AZ_SND_FIRE_LASER_PULSE] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_sustain = 0.11852, .env_punch = 0.132, .env_decay = 0.183098584414,
    .start_freq = 0.535211265087, .freq_limit = 0.2, .freq_slide = -0.26748,
    .square_duty = 0.18045, .duty_sweep = 0.18344
  },
  [AZ_SND_FIRE_MISSILE_BEAM] = {
    .wave_kind = AZ_SAWTOOTH_WAVE,
    .env_sustain = 0.03084, .env_decay = 0.66197180748,
    .start_freq = 0.929577469826, .freq_slide = -0.0140845179558,
    .vibrato_depth = 0.704225361347, .vibrato_speed = 0.739436626434,
    .phaser_offset = 0.352112650871, .phaser_sweep = 0.549295783043,
    .volume_adjust = 0.4
  },
  [AZ_SND_FIRE_OTH_ROCKET] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.605633795261, .env_punch = 0.160833105445,
    .env_decay = 0.732394337654, .start_freq = 0.345070421696,
    .freq_slide = -0.00747528579086, .freq_delta_slide = -0.00656020781025,
    .arp_mod = 0.704225301743, .arp_speed = 0.732394337654,
    .phaser_offset = -0.16901409626, .phaser_sweep = -0.563380241394,
    .lpf_cutoff = 0.795774638653, .lpf_ramp = -0.126760542393,
    .lpf_resonance = 0.718309879303, .volume_adjust = 0.6
  },
  [AZ_SND_FIRE_OTH_SPRAY] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_attack = 0.0563380271196, .env_sustain = 0.309859156609,
    .env_punch = 0.0774647891521, .env_decay = 0.598591566086,
    .start_freq = 0.387323945761, .freq_slide = 0.0563380718231,
    .freq_delta_slide = -0.08450704813,
    .vibrato_depth = 0.0774647891521, .vibrato_speed = 0.654929578304,
    .arp_mod = -0.140845060349,
    .square_duty = 0.570422530174, .duty_sweep = -0.281690120697,
    .repeat_speed = 0.612676084042, .phaser_sweep = 0.774647831917,
    .lpf_cutoff = 0.718309879303, .lpf_ramp = -0.0098278503865,
    .lpf_resonance = 0.366197168827
  },
  [AZ_SND_FIRE_ROCKET] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.39907, .env_punch = 0.65582, .env_decay = 0.39085,
    .start_freq = 0.7881, .freq_slide = -0.30088,
    .vibrato_depth = 0.59703, .vibrato_speed = 0.03828,
    .phaser_offset = -0.09300001, .phaser_sweep = -0.19305,
    .volume_adjust = -0.42592590332
  },
  [AZ_SND_FIRE_STINGER] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.2112676, .env_punch = 0.2112676, .env_decay = 0.28873238,
    .start_freq = 0.3653719, .freq_slide = 0.49295771122
  },
  [AZ_SND_HEAT_DAMAGE] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.605633795261, .freq_slide = 0.591549277306,
    .freq_delta_slide = -0.943661987782, .repeat_speed = 0.0352112688124,
    .phaser_offset = 0.309859156609, .phaser_sweep = -0.33802819252,
    .hpf_cutoff = 0.133802816272, .volume_adjust = -0.2
  },
  [AZ_SND_HIT_WALL] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.03245, .env_decay = 0.10518,
    .start_freq = 0.43334, .freq_slide = -0.57352
  },
  [AZ_SND_KILL_ATOM] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_sustain = 0.211267605424, .env_punch = 0.405718,
    .env_decay = 0.605633795261, .start_freq = 0.7841117,
    .freq_slide = -0.236357, .freq_delta_slide = -0.9898345,
    .vibrato_depth = 0.04728087, .vibrato_speed = -0.4434,
    .arp_mod = 0.6122, .arp_speed = 0.2002,
    .square_duty = -0.3186, .duty_sweep = -0.4922905,
    .repeat_speed = 0.4604,
    .phaser_offset = 0.6059253, .phaser_sweep = -0.3536932,
    .lpf_cutoff = 0.982626, .lpf_ramp = 0.2353423, .lpf_resonance = 0.8618,
    .volume_adjust = 0.8
  },
  [AZ_SND_KILL_DRAGONFLY] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_attack = 0.03548157, .env_sustain = 0.378881,
    .env_punch = 0.3252649, .env_decay = -0.36613,
    .start_freq = 0.05789758,
    .freq_slide = 0.5313399, .freq_delta_slide = -0.1058238,
    .vibrato_depth = 0.9644304, .vibrato_speed = 0.9172,
    .arp_mod = 0.00940001, .arp_speed = 0.281,
    .square_duty = -0.1706, .duty_sweep = 0.585956,
    .repeat_speed = 0.8049999,
    .phaser_offset = -0.5914348, .phaser_sweep = -0.8174004,
    .lpf_cutoff = 0.8984792, .lpf_ramp = 0.3468362, .lpf_resonance = -0.007,
    .hpf_cutoff = 0.1794088, .hpf_ramp = -0.1530325,
    .volume_adjust = 0.3
  },
  [AZ_SND_KILL_TURRET] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.10687, .env_punch = 0.24554, .env_decay = 0.463,
    .start_freq = 0.0972442, .freq_slide = 0.05804, .repeat_speed = 0.56475,
  },
  [AZ_SND_KLAXON] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_sustain = 0.4507, .start_freq = 0.32394,
    .freq_slide = 0.112676, .freq_delta_slide = -0.197183,
    .square_duty = 0.873239, .phaser_offset = 0.40845,
    .volume_adjust = -0.24
  },
  [AZ_SND_KLAXON_DIRE] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_sustain = 0.4507, .start_freq = 0.32394,
    .freq_slide = 0.212676, .freq_delta_slide = -0.197183,
    .square_duty = 0.873239, .phaser_offset = 0.40845,
    .volume_adjust = -0.19
  },
  [AZ_SND_LAUNCH_OTH_RAZORS] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_attack = -0.0173337701708, .env_sustain = 0.450704216957,
    .env_punch = 0.0839145034552, .env_decay = 0.133802816272,
    .start_freq = 0.211267605424, .freq_delta_slide = -0.464788734913,
    .vibrato_speed = 0.725352108479,
    .arp_mod = -0.0140845179558, .arp_speed = -0.966199994087,
    .repeat_speed = 0.58450704813, .phaser_sweep = 0.563380241394,
    .lpf_cutoff = 0.781690120697, .lpf_ramp = -0.464788734913,
    .lpf_resonance = 0.274647891521
  },
  [AZ_SND_METAL_CLINK] = {
    .wave_kind = AZ_SAWTOOTH_WAVE,
    .env_decay = 0.260563373566, .start_freq = 0.407042229176,
    .phaser_offset = -0.197183, .lpf_cutoff = 0.2253521, .lpf_resonance = 1.0
  },
  [AZ_SND_ORION_BOOSTER] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.338028, .env_punch = 0.4225352, .env_decay = 0.6,
    .start_freq = 0.190140843391, .freq_slide = 0.19992,
    .vibrato_depth = 0.39781, .vibrato_speed = 0.06258,
    .phaser_offset = 0.746478915215, .phaser_sweep = -0.0704225301743
  },
  [AZ_SND_PICKUP_ORDNANCE] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_sustain = 0.17572, .env_decay = 0.2746479,
    .start_freq = 0.26918, .freq_slide = 0.25928,
    .square_duty = 0.01332, .repeat_speed = 0.69412
  },
  [AZ_SND_PICKUP_SHIELDS] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_sustain = 0.1376, .env_decay = 0.288732379675,
    .start_freq = 0.225352108479, .freq_slide = 0.2832,
    .repeat_speed = 0.542253494263
  },
  [AZ_SND_SONIC_SCREECH] = {
    .wave_kind = AZ_SAWTOOTH_WAVE,
    .env_sustain = 0.24708, .env_punch = 0.1941, .env_decay = 0.36444,
    .start_freq = 0.6959, .freq_limit = 0.28848,
    .freq_slide = -0.29102, .freq_delta_slide = 0.352112650871,
    .vibrato_depth = 0.316901415586, .vibrato_speed = 0.563380300999,
    .phaser_offset = 0.521126747131, .phaser_sweep = -0.239436626434,
    .hpf_cutoff = 0.394366204739
  },
  [AZ_SND_SPLASH] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_attack = 0.302816897631, .env_decay = 0.464788734913,
    .start_freq = 0.84507, .freq_slide = 0.422535, .freq_delta_slide = -0.0986,
    .phaser_sweep = 1.0, .hpf_cutoff = 0.5
  },
  [AZ_SND_TRACTOR_BEAM] = {
    .wave_kind = AZ_WOBBLE_WAVE, .env_sustain = 1.0, .start_freq = 0.18,
    .vibrato_depth = 0.2, .vibrato_speed = 0.3, .volume_adjust = -0.4
  }
};

AZ_STATIC_ASSERT(AZ_ARRAY_SIZE(sound_specs) == AZ_NUM_SOUND_KEYS + 1);

/*===========================================================================*/

static az_sound_data_t sound_datas[AZ_ARRAY_SIZE(sound_specs)];

static bool sound_data_initialized = false;

static void destroy_sound_datas(void) {
  assert(sound_data_initialized);
  sound_data_initialized = false;
  AZ_ARRAY_LOOP(data, sound_datas) {
    az_destroy_sound_data(data);
  }
}

static const az_sound_data_t *sound_data_for_key(az_sound_key_t sound_key) {
  assert(sound_data_initialized);
  const int sound_index = (int)sound_key;
  assert(sound_index >= 0);
  assert(sound_index < AZ_ARRAY_SIZE(sound_datas));
  const az_sound_data_t *sound_data = &sound_datas[sound_index];
  if (sound_data->num_samples == 0) return NULL;
  return sound_data;
}

void az_init_sound_datas(void) {
  assert(!sound_data_initialized);
  for (int i = 1; i < AZ_ARRAY_SIZE(sound_specs); ++i) {
    az_create_sound_data(&sound_specs[i], &sound_datas[i]);
  }
  sound_data_initialized = true;
  atexit(destroy_sound_datas);
  assert(sound_data_for_key(AZ_SND_NOTHING) == NULL);
}

/*===========================================================================*/

void az_play_sound(az_soundboard_t *soundboard, az_sound_key_t sound_key) {
  az_play_sound_data(soundboard, sound_data_for_key(sound_key));
}

void az_loop_sound(az_soundboard_t *soundboard, az_sound_key_t sound_key) {
  az_loop_sound_data(soundboard, sound_data_for_key(sound_key));
}

void az_persist_sound(az_soundboard_t *soundboard, az_sound_key_t sound_key) {
  az_persist_sound_data(soundboard, sound_data_for_key(sound_key));
}

void az_hold_sound(az_soundboard_t *soundboard, az_sound_key_t sound_key) {
  az_hold_sound_data(soundboard, sound_data_for_key(sound_key));
}

void az_reset_sound(az_soundboard_t *soundboard, az_sound_key_t sound_key) {
  az_reset_sound_data(soundboard, sound_data_for_key(sound_key));
}

/*===========================================================================*/
