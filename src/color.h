// Generation of distinct colors.

#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct Color {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} Color;

bool color_equal(Color a, Color b);

Color color_distinct(int index);
