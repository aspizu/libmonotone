#include "libmonotone.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define HZ_FIXED_POINT 100
#define END_OF_SONG 0xFF
#define NOTE_OFF 0x7F
#define ROWS_PER_PATTERN 64
#define MAX_PATTERNS 256
#define TRACK_SIZE 128
#define DEFAULT_SAMPLE_RATE 44100
#define DEFAULT_TICK_RATE 4
#define MONOTONE_MAGIC_COOKIE "monotone"
#define MIN_FILE_SIZE 0x15F

static size_t note_hz[] = {
    0,     // ---
    2750,  // A0
    2914,  // A#0 / Bb0
    3087,  // B0
    3270,  // C1
    3465,  // C#1 / Db1
    3671,  // D1
    3889,  // D#1 / Eb1
    4120,  // E1
    4365,  // F1
    4625,  // F#1 / Gb1
    4900,  // G1
    5191,  // G#1 / Ab1
    5500,  // A1
    5827,  // A#1 / Bb1
    6174,  // B1
    6541,  // C2
    6930,  // C#2 / Db2
    7342,  // D2
    7778,  // D#2 / Eb2
    8241,  // E2
    8731,  // F2
    9250,  // F#2 / Gb2
    9800,  // G2
    10383, // G#2 / Ab2
    11000, // A2
    11654, // A#2 / Bb2
    12347, // B2
    13081, // C3
    13859, // C#3 / Db3
    14683, // D3
    15556, // D#3 / Eb3
    16481, // E3
    17461, // F3
    18500, // F#3 / Gb3
    19600, // G3
    20765, // G#3 / Ab3
    22000, // A3
    23308, // A#3 / Bb3
    24694, // B3
    26163, // C4
    27718, // C#4 / Db4
    29366, // D4
    31113, // D#4 / Eb4
    32963, // E4
    34923, // F4
    36999, // F#4 / Gb4
    39200, // G4
    41530, // G#4 / Ab4
    44000, // A4
    46616, // A#4 / Bb4
    49388, // B4
    52325, // C5
    55437, // C#5 / Db5
    58733, // D5
    62225, // D#5 / Eb5
    65925, // E5
    69846, // F5
    73999, // F#5 / Gb5
    78399, // G5
    83061, // G#5 / Ab5
    88000, // A5
    93233, // A#5 / Bb5
    98777, // B5
    104650, // C6
    110873, // C#6 / Db6
    117466, // D6
    124451, // D#6 / Eb6
    131851, // E6
    139691, // F6
    147998, // F#6 / Gb6
    156798, // G6
    166122, // G#6 / Ab6
    176000, // A6
    186466, // A#6 / Bb6
    197553, // B6
    209300, // C7
    221746, // C#7 / Db7
    234932, // D7
    248902, // D#7 / Eb7
    263702, // E7
    279383, // F7
    295996, // F#7 / Gb7
    313596, // G7
    332244, // G#7 / Ab7
    352000, // A7
    372931, // A#7 / Bb7
    395107, // B7
    418601, // C8
    443492, // C#8 / Db8
    469864, // D8
    497803, // D#8 / Eb8
    527404, // E8
    558765, // F8
    591991, // F#8 / Gb8
    627193, // G8
    664488, // G#8 / Ab8
    704000, // A8
    745862, // A#8 / Bb8
    790213, // B8
    837202, // C9
};

monotone_err_t monotone_init(monotone_t* monotone, uint8_t* data, size_t size) {
    // First byte is always 8 for some reason.
    if (strncmp((const char*) data + 1, MONOTONE_MAGIC_COOKIE, sizeof(MONOTONE_MAGIC_COOKIE)-1) != 0) {
        return MONOTONE_INVALID_FORMAT;
    }
    if (size < MIN_FILE_SIZE) {
        return MONOTONE_INVALID_FORMAT;
    }
    monotone->total_patterns = data[0x5C];
    monotone->total_tracks = data[0x5D];
    monotone->pattern_order = data + 0x5F;
    monotone->pattern_data = data + 0x15F;
    monotone->pattern_size = TRACK_SIZE * monotone->total_tracks;
    monotone->tracks = malloc(sizeof(monotone_track_t) * monotone->total_tracks);
    if (monotone->tracks == NULL) {
        return MONOTONE_OUT_OF_MEMORY;
    }
    for (size_t i = 0; i < monotone->total_tracks; i++) {
        monotone->tracks[i] = (monotone_track_t) {
            .hz = 0,
            .note = 0,
            .target_hz = 0,
        };
    }
    // Set default values if uninitialized.
    if (monotone->tick_rate == 0) {
        monotone->tick_rate = DEFAULT_TICK_RATE;
    }
    if (monotone->sample_rate == 0) {
        monotone->sample_rate = DEFAULT_SAMPLE_RATE;
    }
    return MONOTONE_OK;
}

void monotone_deinit(monotone_t* monotone) {
    free(monotone->tracks);
    monotone->tracks = NULL;
}

size_t monotone_generate(monotone_t* monotone, uint8_t* buffer, size_t sample_count, size_t samples_per_tick) {
    size_t samples_generated = 0;
    size_t tick_count = sample_count / samples_per_tick;
    for (size_t i = 0; i < tick_count; i++) {
        if (!monotone_tick(monotone)) break;
        for (size_t j = 0; j < samples_per_tick; j++) {
            size_t sample = 0;
            for (size_t k = 0; k < monotone->total_tracks; k++) {
                monotone_track_t* track = &monotone->tracks[k];
                if (track->note == NOTE_OFF) continue;
                sample += (monotone->time * track->hz * 5 / (monotone->sample_rate * 2)) % 256 < 127 ? 255 : 0;
            }
            sample /= monotone->total_tracks;
            // left channel
            *buffer++ = sample;
            // right channel
            *buffer++ = sample;
            monotone->time++;
            samples_generated++;
        }
    }
    return samples_generated;
}

bool monotone_tick(monotone_t* monotone) {
    if (monotone->tick == monotone->tick_rate) {
        monotone->tick = 0;
        monotone->row++;
    }
    if (monotone->row == ROWS_PER_PATTERN) {
        monotone->row = 0;
        monotone->pattern++;
    }
    if (monotone->pattern == MAX_PATTERNS) {
        monotone->pattern = 0;
    }
    size_t pattern = monotone->pattern_order[monotone->pattern];
    if (pattern == END_OF_SONG) {
        for (size_t i = 0; i < monotone->total_tracks; i++) {
            monotone->tracks[i].hz = 0;
        }
        return false;
    }
    bool was_pattern_jumped = false;
    for (size_t i = 0; i < monotone->total_tracks; i++) {
        monotone_track_t* track = &monotone->tracks[i];
        size_t cell = pattern * monotone->pattern_size + monotone->row * monotone->total_tracks * 2 + i * 2;
        uint8_t note = monotone->pattern_data[cell + 1] >> 1;
        monotone_effect_t effect_type = (monotone->pattern_data[cell + 1] & 1) << 2 | monotone->pattern_data[cell] >> 6;
        uint8_t effect_x = monotone->pattern_data[cell] >> 3 & 0b111;
        uint8_t effect_y = monotone->pattern_data[cell] & 0b111;
        uint16_t effect_xy = effect_x << 3 | effect_y;
        if (note != 0 && note != track->note) {
            if (effect_type == 3) {
                track->target_hz = note_hz[note];
            } else {
                track->hz = note_hz[note];
            }
            track->note = note;
        }
        if (effect_type == ARPEGGIATE && effect_xy != 0) {
            size_t tick = monotone->tick % 3;
            if (tick == 1) {
                track->hz = note_hz[track->note + effect_x];
            }
            else if (tick == 2) {
                track->hz = note_hz[track->note + effect_y];
            }
            else {
                track->hz = note_hz[track->note];
            }
        }
        else if (effect_type == PORTAMENTO_UP) {
            track->hz += effect_xy*HZ_FIXED_POINT;
        }
        else if (effect_type == PORTAMENTO_DOWN) {
            track->hz -= effect_xy*HZ_FIXED_POINT;
        }
        else if (effect_type == PORTAMENTO_TO_NOTE) {
            if (track->hz < track->target_hz) {
                track->hz += effect_xy*HZ_FIXED_POINT;
                if (track->hz > track->target_hz) {
                    track->hz = track->target_hz;
                }
            }
            else if (track->hz > track->target_hz) {
                track->hz -= effect_xy*HZ_FIXED_POINT;
                if (track->hz < track->target_hz) {
                    track->hz = track->target_hz;
                }
            }
        }
        else if (effect_type == VIBRATO) {

        }
        else if (effect_type == PATTERN_JUMP && monotone->tick == monotone->tick_rate - 1) {
            monotone->pattern = effect_xy;
            monotone->tick = 0;
            monotone->row = 0;
            was_pattern_jumped = true;
        }
        else if (effect_type == ROW_JUMP && monotone->tick == monotone->tick_rate - 1) {
            monotone->tick = 0;
            monotone->row = effect_xy;
            if (!was_pattern_jumped) {
                monotone->pattern++;
                // pattern wrap-around is done at the beginning of this function.
            }
        }
        else if (effect_type == SET_SPEED) {
            monotone->tick_rate = effect_xy;
        }
    }
    monotone->tick++;
    return true;
}
