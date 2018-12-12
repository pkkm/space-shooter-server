#include "serialization.h"
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

const SProtocolId S_PROTOCOL_ID = 0xEC3B5FA9; // Randomly chosen.
const SVersion S_PROTOCOL_VERSION = {7, 0};

void s_swap_endianness(void *target, size_t size) {
	char *first = target;
	char *last = (char *) target + size - 1;
	assert(last > first);

	while (first < last) {
		char temp = *first;
		*first = *last;
		*last = temp;

		first++;
		last--;
	}
}

bool s_little_endian(void) {
	volatile uint32_t test_num = 0x12345678;
	return *((uint8_t *) &test_num) == 0x78;
}

void s_to_big_endian(void *target, size_t size) {
	if (s_little_endian())
		s_swap_endianness(target, size);
}

void s_from_big_endian(void *target, size_t size) {
	if (s_little_endian())
		s_swap_endianness(target, size);
}

void s_ptr_to_relative(void *relative_ptr, void *pointee) {
	SRelativePtr result = (char *) pointee - (char *) relative_ptr;
	memcpy(relative_ptr, &result, sizeof(result));
}

void *s_relative_to_ptr(void *relative_ptr) {
	SRelativePtr offset;
	memcpy(&offset, relative_ptr, sizeof(offset));
	return (char *) relative_ptr + offset;
}

bool s_relative_valid(void *relative_ptr, void *valid_begin, void *valid_end) {
	void *pointee = s_relative_to_ptr(relative_ptr);
	return pointee >= valid_begin && pointee < valid_end;
}

void s_array_init(void *array, void *elems_begin, size_t n_elems) {
	SArray result;
	result.n_elems = n_elems;
	memcpy(array, &result, sizeof(result));
	s_ptr_to_relative((char *) array + offsetof(SArray, begin), elems_begin);
}

bool s_array_valid(void *array, size_t elem_size,
                   void *valid_begin, void *valid_end) {
	void *begin = s_relative_to_ptr(
		(char *) array + offsetof(SArray, begin));

	SArray array_;
	memcpy(&array_, array, sizeof(array_));
	void *end = (char *) begin + elem_size * array_.n_elems;

	return array_.n_elems == 0
		|| (begin >= valid_begin && begin < valid_end &&
		    end >= valid_begin && end <= valid_end);
}

void s_packet_header_init(void *header, SPacketType type) {
	SPacketHeader result;
	result.protocol_id = S_PROTOCOL_ID;
	result.protocol_version = S_PROTOCOL_VERSION;
	result.type = type;
	memcpy(header, &result, sizeof(result));
}
