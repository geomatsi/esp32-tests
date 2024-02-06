/*
 * Source: https://www.dcs.gla.ac.uk/~jhw/cordic/index.html
 *
 * Cordic in 32 bit signed fixed point math:
 * - function is valid for arguments in range (-pi/2, pi/2)
 * - for values [pi/2, pi) and (-pi, -pi/2]: value = pi/2 - (theta - pi/2)
 *
 * 1.0 = 1073741824
 * 1/k = 0.6072529350088812561694
 * pi = 3.1415926536897932384626
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#define cordic_1K 0x26DD3B6ALL
#define half_pi 0x6487ED51LL
#define MUL 1073741824LL

#define CORDIC_NTAB 32

static int cordic_ctab [] = {
	0x3243F6A8LL,
	0x1DAC6705LL,
	0x0FADBAFCLL,
	0x07F56EA6LL,
	0x03FEAB76LL,
	0x01FFD55BLL,
	0x00FFFAAALL,
	0x007FFF55LL,
	0x003FFFEALL,
	0x001FFFFDLL,
	0x000FFFFFLL,
	0x0007FFFFLL,
	0x0003FFFFLL, 
	0x0001FFFFLL,
	0x0000FFFFLL,
	0x00007FFFLL,
	0x00003FFFLL,
	0x00001FFFLL,
	0x00000FFFLL,
	0x000007FFLL,
	0x000003FFLL,
	0x000001FFLL,
	0x000000FFLL,
	0x0000007FLL,
	0x0000003FLL,
	0x0000001FLL,
	0x0000000FLL,
	0x00000008LL,
	0x00000004LL,
	0x00000002LL,
	0x00000001LL,
	0x00000000LL,
};

static void cordic(int64_t theta, int64_t *s, int64_t *c)
{
	int64_t k, d, tx, ty, tz;
	int64_t x = cordic_1K;
	int64_t z = theta;
	int64_t y = 0;

	for (k = 0; k < CORDIC_NTAB; ++k) {
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

static int64_t cordic_arg(int64_t freq, uint64_t samples_per_sec, int i)
{
	/* int64_t overflow:
	 * return MUL * 2 * 355 * freq * i / 113 / samples_per_sec */

	/* pi ~ 355 / 113 */
	return ((MUL * 2 * i) / samples_per_sec) * freq * 355 / 113;
}

size_t sine_cordic32(int freq, unsigned int samples_per_sec, int amp, int16_t *sine, unsigned samples)
{
	int64_t cos, sin, val;

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
size_t sine_cordic32_v2(int freq, unsigned int samples_per_sec, int amp, int16_t *sine, unsigned samples)
{
	int64_t cos, sin, val;
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
