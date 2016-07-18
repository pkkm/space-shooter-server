// Growable array (like std::vector in C++).

#pragma once
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct Vec {
	void *array;
	size_t n_elems;
	size_t elem_size;
	size_t n_allocated;
} Vec;

enum { VEC_INITIAL_N_ALLOCATED = 1 };
enum { VEC_STRETCH_FACTOR  = 2 };

void vec_allocate(Vec *vec, size_t n_allocated) {
	assert(n_allocated > 0 && n_allocated >= vec->n_elems);
	vec->n_allocated = n_allocated;
	vec->array = realloc(vec->array, n_allocated * vec->elem_size);
}

void vec_init(Vec *vec, size_t elem_size) {
	assert(elem_size > 0);
	vec->n_elems = 0;
	vec->elem_size = elem_size;
	vec->array = NULL;
	vec_allocate(vec, VEC_INITIAL_N_ALLOCATED);
}

void vec_ensure_allocated(Vec *vec, size_t n_elems) {
	if (vec->n_allocated < n_elems) {
		size_t stretched_n_elems = vec->n_allocated * VEC_STRETCH_FACTOR;
		if (stretched_n_elems >= n_elems)
			vec_allocate(vec, stretched_n_elems);
		else
			vec_allocate(vec, n_elems);
	}
}

void vec_resize(Vec *vec, size_t n_elems) {
	vec_ensure_allocated(vec, n_elems);
	vec->n_elems = n_elems;
}

void *vec_elem_ptr(Vec *vec, size_t i_elem) {
	return (char *) vec->array + (i_elem * vec->elem_size);
}

void *vec_get(Vec *vec, size_t i_elem) {
	assert(i_elem <= vec->n_elems);
	return vec_elem_ptr(vec, i_elem);
}

void vec_set(Vec *vec, size_t i_elem, const void *elem) {
	assert(i_elem <= vec->n_elems);
	memcpy(vec_elem_ptr(vec, i_elem), elem, vec->elem_size);
}

void vec_push(Vec *vec, const void *elem) {
	vec_resize(vec, vec->n_elems + 1);
	vec_set(vec, vec->n_elems - 1, elem);
}

void vec_pop(Vec *vec) {
	assert(vec->n_elems > 0);
	vec->n_elems--;
}

void vec_insert(Vec *vec, size_t i_new_elem, const void *new_elem) {
	assert(i_new_elem <= vec->n_elems);

	size_t n_elems_after = vec->n_elems - i_new_elem;

	vec_resize(vec, vec->n_elems + 1);
	memmove(vec_elem_ptr(vec, i_new_elem + 1),
	        vec_elem_ptr(vec, i_new_elem),
	        n_elems_after * vec->elem_size);

	vec_set(vec, i_new_elem, new_elem);
}

void vec_delete(Vec *vec, size_t i_elem) {
	assert(i_elem < vec->n_elems);

	size_t n_elems_after = vec->n_elems - 1 - i_elem;
	memmove(vec_elem_ptr(vec, i_elem),
	        vec_elem_ptr(vec, i_elem + 1),
	        n_elems_after * vec->elem_size);

	vec_resize(vec, vec->n_elems - 1);
}
