/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Header structure for WAV file with only one data chunk
 *
 * @note See this for reference: http://soundfile.sapp.org/doc/WaveFormat/
 *
 * @note Assignment to variables in this struct directly is only possible for little endian architectures
 *       (including Xtensa & RISC-V)
 */

typedef struct {
	char chunk_id[4]; /*!< Contains the letters "RIFF" in ASCII form */
	uint32_t chunk_size; /*!< This is the size of the rest of the chunk following this number */
	char chunk_format[4]; /*!< Contains the letters "WAVE" */
} dsc_chunk_t; /*!< Canonical WAVE format starts with the RIFF header */

typedef struct {
	char subchunk_id[4]; /*!< Contains the letters "fmt " */
	uint32_t subchunk_size; /*!< This is the size of the rest of the Subchunk which follows this number */
	uint16_t audio_format; /*!< PCM = 1, values other than 1 indicate some form of compression */
	uint16_t num_of_channels; /*!< Mono = 1, Stereo = 2, etc. */
	uint32_t sample_rate; /*!< 8000, 44100, etc. */
	uint32_t byte_rate; /*!< ==SampleRate * NumChannels * BitsPerSample s/ 8 */
	uint16_t block_align; /*!< ==NumChannels * BitsPerSample / 8 */
	uint16_t bits_per_sample; /*!< 8 bits = 8, 16 bits = 16, etc. */
} fmt_chunk_t; /*!< The "fmt " subchunk describes the sound data's format */

typedef struct {
	char subchunk_id[4]; /*!< Contains the letters "data" */
	uint32_t subchunk_size; /*!< ==NumSamples * NumChannels * BitsPerSample / 8 */
	int16_t data[0]; /*!< Holds raw audio data */
} data_chunk_t; /*!< The "data" subchunk contains the size of the data and the actual sound */

typedef struct {
	dsc_chunk_t descriptor_chunk;
	fmt_chunk_t fmt_chunk;
	data_chunk_t data_chunk;
} wav_header_t;

#ifdef __cplusplus
}
#endif
