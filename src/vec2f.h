// Vectors of 2 floats.

#pragma once
#include "serialization.h"

typedef SVectorFloat Vec2f;

Vec2f vec2f_from_polar(float angle, float length);

Vec2f vec2f_scale(Vec2f vector, float multiplier);

Vec2f vec2f_add(Vec2f a, Vec2f b);

Vec2f vec2f_subtract(Vec2f a, Vec2f b);

float vec2f_dot_product(Vec2f a, Vec2f b);

float vec2f_length(Vec2f vector);

float vec2f_distance(Vec2f a, Vec2f b);

Vec2f vec2f_velocity_add(Vec2f u, Vec2f v, float speed_limit);

Vec2f vec2f_wrap_position(Vec2f position, SVectorInt limits);
