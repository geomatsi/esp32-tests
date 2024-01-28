#include <stdint.h>
#include <stdlib.h>
#include <math.h>

size_t sine_float(int freq, unsigned int samples_per_sec, int amp, int16_t *sine, unsigned int samples)
{
	for (int i = 0; i <  samples; i++) {
		*(sine + i) = amp * sin(2 * M_PI * freq * i / samples_per_sec);
	}

	return samples * sizeof(*sine);
}
