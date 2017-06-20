// Growable array (like std::vector in C++).

#pragma once
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct Vector {
	void *array;
	size_t n_elems;
	size_t elem_size;
	size_t n_allocated;
} Vector;

enum { VECTOR_INITIAL_N_ALLOCATED = 1 };
enum { VECTOR_STRETCH_FACTOR  = 2 };

void vector_allocate(Vector *vector, size_t n_allocated) {
	assert(n_allocated > 0 && n_allocated >= vector->n_elems);
	vector->n_allocated = n_allocated;
	vector->array = realloc(vector->array, n_allocated * vector->elem_size);
}

void vector_init(Vector *vector, size_t elem_size) {
	assert(elem_size > 0);
	vector->n_elems = 0;
	vector->elem_size = elem_size;
	vector->array = NULL;
	vector_allocate(vector, VECTOR_INITIAL_N_ALLOCATED);
}

void vector_ensure_allocated(Vector *vector, size_t n_elems) {
	if (vector->n_allocated < n_elems) {
		size_t stretched_n_elems = vector->n_allocated * VECTOR_STRETCH_FACTOR;
		if (stretched_n_elems >= n_elems)
			vector_allocate(vector, stretched_n_elems);
		else
			vector_allocate(vector, n_elems);
	}
}

void vector_resize(Vector *vector, size_t n_elems) {
	vector_ensure_allocated(vector, n_elems);
	vector->n_elems = n_elems;
}

void *vector_elem_ptr(Vector *vector, size_t i_elem) {
	return (char *) vector->array + (i_elem * vector->elem_size);
}

void *vector_get(Vector *vector, size_t i_elem) {
	assert(i_elem <= vector->n_elems);
	return vector_elem_ptr(vector, i_elem);
}

void vector_set(Vector *vector, size_t i_elem, const void *elem) {
	assert(i_elem <= vector->n_elems);
	memcpy(vector_elem_ptr(vector, i_elem), elem, vector->elem_size);
}

void vector_push(Vector *vector, const void *elem) {
	vector_resize(vector, vector->n_elems + 1);
	vector_set(vector, vector->n_elems - 1, elem);
}

void vector_pop(Vector *vector) {
	assert(vector->n_elems > 0);
	vector->n_elems--;
}

void vector_insert(Vector *vector, size_t i_new_elem, const void *new_elem) {
	assert(i_new_elem <= vector->n_elems);

	size_t n_elems_after = vector->n_elems - i_new_elem;

	vector_resize(vector, vector->n_elems + 1);
	memmove(vector_elem_ptr(vector, i_new_elem + 1),
	        vector_elem_ptr(vector, i_new_elem),
	        n_elems_after * vector->elem_size);

	vector_set(vector, i_new_elem, new_elem);
}

void vector_delete(Vector *vector, size_t i_elem) {
	assert(i_elem < vector->n_elems);

	size_t n_elems_after = vector->n_elems - 1 - i_elem;
	memmove(vector_elem_ptr(vector, i_elem),
	        vector_elem_ptr(vector, i_elem + 1),
	        n_elems_after * vector->elem_size);

	vector_resize(vector, vector->n_elems - 1);
}
