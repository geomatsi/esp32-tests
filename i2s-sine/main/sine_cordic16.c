/*
 * Source: https://www.dcs.gla.ac.uk/~jhw/cordic/index.html
 *
 * Cordic in 32 bit signed fixed point math:
 * - function is valid for arguments in range (-pi/2, pi/2)
 * - for values [pi/2, pi) and (-pi, -pi/2]: value = pi/2 - (theta - pi/2)
 *
 * 1.0 = 16384
 * 1/k = 0.6072529350088812561694
 * pi = 3.1415926536897932384626
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#define cordic_1K 0x000026DDL
#define half_pi   0x00006487L
#define MUL       16384L

#define CORDIC_NTAB 16

static int cordic_ctab [] = {
	0x00003243L,
	0x00001DACL,
	0x00000FADL,
	0x000007F5L,
	0x000003FEL,
	0x000001FFL,
	0x000000FFL,
	0x0000007FL,
	0x0000003FL,
	0x0000001FL,
	0x0000000FL,
	0x00000007L,
	0x00000003L,
	0x00000001L,
	0x00000000L,
	0x00000000L,
};

static void cordic(int32_t theta, int32_t *s, int32_t *c)
{
	int32_t k, d, tx, ty, tz;
	int32_t x = cordic_1K;
	int32_t z = theta;
	int32_t y = 0;

	for (k = 0; k < CORDIC_NTAB; k++) {
		d = (z >= 0) ? 0 : -1;
		tx = x - (((y >> k) ^ d) - d);
		ty = y + (((x >> k) ^ d) - d);
		tz = z - ((cordic_ctab[k] ^ d) - d);

		x = tx;
		y = ty;
		z = tz;
	}

	*c = x;
	*s = y;
}

static int32_t cordic_arg(int32_t freq, uint32_t samples_per_sec, int i)
{
	/* int32_t overflow:
	 * return MUL * 2 * 355 * freq * i / 113 / samples_per_sec */

	/* pi ~ 355 / 113 */
	return ((MUL * 2 * i) / samples_per_sec) * freq * 355 / 113;
}

size_t sine_cordic16(int freq, unsigned int samples_per_sec, int amp, int16_t *sine, unsigned samples)
{
	int32_t cos, sin, val;

	for (int i = 0; i < samples; i++) {
		val = cordic_arg(freq, samples_per_sec, i);

		if (val > 4 * half_pi)
			val -= (val / (4 * half_pi)) * 4 * half_pi;

		if (val <= half_pi) {
			cordic(val, &sin, &cos);
		} else if ((half_pi < val) && (val <= 2 * half_pi)) {
			cordic(2 * half_pi - val, &sin, &cos);
		} else if ((2 * half_pi < val) && (val <= 3 * half_pi)) {
			cordic(val - 2 * half_pi, &sin, &cos);
			sin *= -1;
		} else if ((3 * half_pi < val) && (val <= 4 * half_pi)) {
			cordic(4 * half_pi - val, &sin, &cos);
			sin *= -1;
		} else {
			assert(0);
		}

		val = amp * sin / MUL;
		assert(val > INT16_MIN && val < INT16_MAX);
		*(sine + i) = val;
	}

	return samples * sizeof(*sine);
}

/* use cordic for the first period, then copy-paste samples from the first period */
size_t sine_cordic16_v2(int freq, unsigned int samples_per_sec, int amp, int16_t *sine, unsigned samples)
{
	int32_t cos, sin, val;
	int i, k;

	for (i = 0; i < samples; i++) {
		val = cordic_arg(freq, samples_per_sec, i);

		if (val > 4 * half_pi)
			break;

		if (val <= half_pi) {
			cordic(val, &sin, &cos);
		} else if ((half_pi < val) && (val <= 2 * half_pi)) {
			cordic(2 * half_pi - val, &sin, &cos);
		} else if ((2 * half_pi < val) && (val <= 3 * half_pi)) {
			cordic(val - 2 * half_pi, &sin, &cos);
			sin *= -1;
		} else if ((3 * half_pi < val) && (val <= 4 * half_pi)) {
			cordic(4 * half_pi - val, &sin, &cos);
			sin *= -1;
		} else {
			assert(0);
		}

		val = amp * sin / MUL;
		assert(val > INT16_MIN && val < INT16_MAX);
		*(sine + i) = val;
	}

	/* last sample of the first period */
	k = i - 2;

	for(i = (k + 1); i < samples; i += (k + 1)) {
		if ((i + (k + 1)) < samples) {
			memcpy(sine + i, sine, (k + 1) * sizeof(*sine));
		} else {
			memcpy(sine + i, sine, (samples - i) * sizeof(*sine));
		}
	}

	return samples * sizeof(*sine);
}
