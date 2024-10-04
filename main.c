#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "libmonotone.h"

#define DEFAULT_SAMPLE_RATE 44100
#define DEFAULT_SAMPLES_PER_TICK 735

void usage(const char* prog_name) {
    fprintf(stderr, "Usage: %s <input.mon> <output.pcm>\n", prog_name);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    const char* input_filename = argv[1];
    const char* output_filename = argv[2];

    // Open the input .mon file
    FILE* input_file = fopen(input_filename, "rb");
    if (!input_file) {
        perror("Failed to open input file");
        return EXIT_FAILURE;
    }

    // Get the size of the .mon file
    fseek(input_file, 0, SEEK_END);
    size_t file_size = ftell(input_file);
    fseek(input_file, 0, SEEK_SET);

    // Allocate buffer to hold the .mon file data
    uint8_t* song_data = malloc(file_size);
    if (!song_data) {
        fclose(input_file);
        fprintf(stderr, "Out of memory while allocating song data.\n");
        return EXIT_FAILURE;
    }

    // Read the .mon file into the buffer
    if (fread(song_data, 1, file_size, input_file) != file_size) {
        fclose(input_file);
        free(song_data);
        fprintf(stderr, "Failed to read input file completely.\n");
        return EXIT_FAILURE;
    }
    fclose(input_file);

    // Initialize the Monotone instance
    monotone_t monotone;
    memset(&monotone, 0, sizeof(monotone_t)); // Ensure clean initialization
    monotone.sample_rate = DEFAULT_SAMPLE_RATE;
    monotone.tick_rate = 4; // Default tick rate

    monotone_err_t init_result = monotone_init(&monotone, song_data, file_size);
    if (init_result != MONOTONE_OK) {
        free(song_data);
        fprintf(stderr, "Failed to initialize Monotone instance: %d\n", init_result);
        return EXIT_FAILURE;
    }

    // Open the output .pcm file
    FILE* output_file = fopen(output_filename, "wb");
    if (!output_file) {
        monotone_deinit(&monotone);
        free(song_data);
        perror("Failed to open output file");
        return EXIT_FAILURE;
    }

    // Buffer for generated PCM samples
    size_t samples_per_tick = DEFAULT_SAMPLES_PER_TICK;
    uint8_t* buffer = malloc(samples_per_tick*2);
    if (!buffer) {
        fclose(output_file);
        monotone_deinit(&monotone);
        free(song_data);
        fprintf(stderr, "Out of memory while allocating sample buffer.\n");
        return EXIT_FAILURE;
    }

    // Generate samples and write them to the output file
    while (true) {
        size_t generated = monotone_generate(&monotone, buffer, samples_per_tick, samples_per_tick);
        if (generated == 0) break;
        if (fwrite(buffer, 1, generated*2, output_file) != generated*2) {
            fprintf(stderr, "Failed to write to output file.\n");
            break;
        }
    }

    // Clean up
    fclose(output_file);
    monotone_deinit(&monotone);
    free(song_data);
    free(buffer);

    printf("Successfully converted %s to %s.\n", input_filename, output_filename);
    return EXIT_SUCCESS;
}
