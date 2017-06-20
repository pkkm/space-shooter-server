// Vectors of 2 floats.

#pragma once
#include <math.h>
#include "serialization.h"

typedef SVectorFloat Vec2f;

Vec2f vec2f_from_polar(float angle, float length) {
	Vec2f result;
	result.x = cos(angle) * length;
	result.y = -sin(angle) * length;
	return result;
}

Vec2f vec2f_scale(Vec2f vector, float multiplier) {
	Vec2f result;
	result.x = vector.x * multiplier;
	result.y = vector.y * multiplier;
	return result;
}

Vec2f vec2f_add(Vec2f a, Vec2f b) {
	Vec2f result;
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	return result;
}

Vec2f vec2f_subtract(Vec2f a, Vec2f b) {
	Vec2f result;
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	return result;
}

float vec2f_dot_product(Vec2f a, Vec2f b) {
	return a.x * b.x + a.y * b.y;
}

float vec2f_length(Vec2f vector) {
	return sqrt(vec2f_dot_product(vector, vector));
}

float vec2f_distance(Vec2f a, Vec2f b) {
	return vec2f_length(vec2f_subtract(a, b));
}

Vec2f vec2f_velocity_add(Vec2f u, Vec2f v, float speed_limit) {
	// Pseudo-relativistic velocity addition (so that players don't accelerate to ridiculous speeds).
	float sqr_limit = pow(speed_limit, 2);
	return vec2f_scale(
		vec2f_add(u, v),
		sqr_limit / (sqr_limit + fmax(0, vec2f_dot_product(u, v))));
}

Vec2f vec2f_wrap_position(Vec2f position, SVectorInt limits) {
	if (position.x < 0)
		position.x += limits.x;
	else if (position.x > limits.x)
		position.x -= limits.x;

	if (position.y < 0)
		position.y += limits.y;
	else if (position.y > limits.y)
		position.y -= limits.y;

	return position;
}
