// Random number generator.

#pragma once
#include <stdint.h>

typedef struct RndState {
	uint64_t x;
	uint64_t y;
} RndState;

RndState rnd_state_new(uint64_t seed);

uint64_t rnd_next(RndState *state);

int rnd_in_range(RndState *state, int min, int max);
