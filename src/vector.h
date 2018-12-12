// Growable array (like std::vector in C++).

#pragma once
#include <stddef.h>

typedef struct Vector {
	void *array;
	size_t n_elems;
	size_t elem_size;
	size_t n_allocated;
} Vector;

void vector_allocate(Vector *vector, size_t n_allocated);

void vector_init(Vector *vector, size_t elem_size);

void vector_ensure_allocated(Vector *vector, size_t n_elems);

void vector_resize(Vector *vector, size_t n_elems);

void *vector_elem_ptr(Vector *vector, size_t i_elem);

void *vector_get(Vector *vector, size_t i_elem);

void vector_set(Vector *vector, size_t i_elem, const void *elem);

void vector_push(Vector *vector, const void *elem);

void vector_pop(Vector *vector);

void vector_insert(Vector *vector, size_t i_new_elem, const void *new_elem);

void vector_delete(Vector *vector, size_t i_elem);
