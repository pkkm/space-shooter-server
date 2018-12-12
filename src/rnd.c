#include "rnd.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

RndState rnd_state_new(uint64_t seed) {
	assert(seed != 0);
	RndState state;
	state.x = state.y = seed;
	return state;
}

// XorShift128+ -- very fast PRNG.
static uint64_t rnd_xs128p(RndState *state) {
	assert(state->x != 0 || state->y != 0);
	uint64_t x = state->x;
	uint64_t y = state->y;
	state->x = y;
	x ^= x << 23;
	state->y = x ^ y ^ (x >> 17) ^ (y >> 26);
	return state->y + y;
}

uint64_t rnd_next(RndState *state) {
	return rnd_xs128p(state);
}

int rnd_in_range(RndState *state, int min, int max) {
	assert(min <= max);
	assert((unsigned int) max - min + 1 <= RAND_MAX);

	static const unsigned int RANGE_RAND = (unsigned int) RAND_MAX + 1;
	unsigned int range_wanted = max - min + 1;
	unsigned int range_accepted = RANGE_RAND - (range_wanted % RANGE_RAND);

	unsigned int random;
	do {
		random = rnd_next(state);
	} while (!(random < range_accepted));

	return min + (random % range_wanted);
}
