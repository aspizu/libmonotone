#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/** A Monotone instance. */
typedef struct monotone_s monotone_t;
/** A track. */
typedef struct track_s track_t;

struct track_s {
    size_t hz;
    uint8_t note;
    size_t target_hz;
};

struct monotone_s {
    /** The number of patterns in the song. */
    size_t total_patterns;
    /** The number of tracks in the song. */
    size_t total_tracks;
    /** The size of one pattern in bytes. */
    size_t pattern_size;
    /** The order of patterns in the song. length = 256 */
    uint8_t* pattern_order;
    /** The data of the patterns. length = `pattern_size` * `total_patterns` */
    uint8_t* pattern_data;
    /** The sample rate at which to generate samples. (Default = 44100) */
    size_t sample_rate;
    /** State of each track. length = `total_tracks` */
    track_t* tracks;
    /** The tick rate is the no. of ticks per row. (Default = 4) */
    size_t tick_rate;
    /** The current tick count modulo `tick_rate`. */
    size_t tick;
    /** The current row, 0-63 */
    size_t row;
    /** The current pattern in the pattern order, 0-255 */
    size_t pattern;
    /** No. of samples elapsed. */
    size_t time;
};

enum monotone_err_e {
    MONOTONE_OK,
    MONOTONE_INVALID_FORMAT,
    MONOTONE_OUT_OF_MEMORY,
};

typedef enum monotone_err_e monotone_err_t;

/** Initialize a Monotone instance with a song.

    @param monotone The Monotone instance to initialize. Set configuration values before calling, otherwise sane defaults are set.
    @param data The song data.
    @param size The size of the song data.

    @returns `MONOTONE_OK` on success.
    @returns `MONOTONE_INVALID_FORMAT` if the song data is invalid.
    @returns `MONOTONE_OUT_OF_MEMORY` if the Monotone instance could not be allocated.
*/
monotone_err_t monotone_init(monotone_t* monotone, uint8_t* data, size_t size);
/** Deinitialize a Monotone instance.

    @param monotone The Monotone instance to deinitialize.
*/
void monotone_deinit(monotone_t* monotone);
/** Generate a buffer of samples.

    @param monotone The Monotone instance.
    @param buffer The buffer to write samples to. Must be at least `sample_count * samples_per_tick` bytes long.
    @param sample_count The number of samples to generate.
    @param samples_per_tick The number of samples per tick.

    @returns no. of samples generated (0 if song ended).
*/
size_t monotone_generate(monotone_t* monotone, uint8_t* buffer, size_t sample_count, size_t samples_per_tick);

bool monotone_tick(monotone_t* monotone);
