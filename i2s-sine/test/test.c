#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

#define SAMPLING_FREQ 16000

size_t sine_float(int freq, unsigned int samples_per_sec, int amp, int16_t *sine, unsigned int samples);
size_t sine_cordic16(int freq, unsigned int samples_per_sec, int amp, int16_t *sine, unsigned int samples);
size_t sine_cordic32(int freq, unsigned int samples_per_sec, int amp, int16_t *sine, unsigned int samples);

int16_t sine1[SAMPLING_FREQ];
int16_t sine2[SAMPLING_FREQ];
int16_t sine3[SAMPLING_FREQ];

int main(int argc, char **argv)
{
	int i, v1, v2;

	if (argc == 3) {
		v2 = atoi(argv[2]);
		v1 = atoi(argv[1]);
	} else {
		v2 = ARRAY_SIZE(sine1);
		v1 = 0;
	}

	if (v1 >= v2) {
		fprintf(stderr, "invalid range: v1 (%d) >= v2(%d)\n", v1, v2);
		exit(1);
	}

	sine_float(440, SAMPLING_FREQ, 2000, &sine1[0], ARRAY_SIZE(sine1));
	sine_cordic16(440, SAMPLING_FREQ, 2000, &sine2[0], ARRAY_SIZE(sine2));
	sine_cordic32(440, SAMPLING_FREQ, 2000, &sine3[0], ARRAY_SIZE(sine3));

	fprintf(stdout, "# sample float cordic16 c16_delta float cordic32 c32_delta\n");

	for(i = v1; i < v2; i++) {
		fprintf(stdout, "%08d %+08d %+08d %+08d %+08d %+08d %+08d\n", i, sine1[i], sine2[i], sine1[i] - sine2[i], sine1[i], sine3[i], sine1[i] - sine3[i]);
	}       

	return 0;
}
