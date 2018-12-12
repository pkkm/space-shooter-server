// Network serialization format.

#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h> // For __STDC_IEC_559__.

// Warn about format of floats if we're not sure about it.
#if __STDC_IEC_559__ == 0
#if defined(_MSC_VER)
#pragma message("WARNING: Please ensure that the storage format of floats on your machine is IEEE 754-compliant.")
#else
#warn Please ensure that the storage format of floats on your machine is IEEE 754-compliant.
#endif
#endif

#pragma pack(push, 1)
// It's OK to access members of packed structs directly (struct->member), but be careful when doing it via pointers (int *ptr = &struct->member): they may be unaligned and cause a fault on some architectures. See <http://stackoverflow.com/questions/8568432/is-gccs-attribute-packed-pragma-pack-unsafe>. For safe unaligned access, use memcpy.
// We're going to write defensively in this file, but assume x86 (little endian, optional alignment) in the rest of the game.


/// Endianness. (We assume either little-endian or big-endian.)

void s_swap_endianness(void *target, size_t size);
bool s_little_endian(void);
void s_to_big_endian(void *target, size_t size);
void s_from_big_endian(void *target, size_t size);


/// General types.

typedef uint8_t SBool;

typedef struct SVectorInt {
	int32_t x;
	int32_t y;
} SVectorInt;

typedef struct SVectorFloat {
	float x;
	float y;
} SVectorFloat;

typedef struct SColor {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} SColor;

typedef int32_t SRelativePtr;

void s_ptr_to_relative(void *relative_ptr, void *pointee);
void *s_relative_to_ptr(void *relative_ptr);
bool s_relative_valid(void *relative_ptr, void *valid_begin, void *valid_end);

typedef struct SArray {
	uint32_t n_elems;
	SRelativePtr begin;
} SArray;

void s_array_init(void *array, void *elems_begin, size_t n_elems);
bool s_array_valid(
	void *array, size_t elem_size, void *valid_begin, void *valid_end);


/// Packet.

typedef uint32_t SProtocolId;

typedef struct SVersion {
	uint16_t major;
	uint16_t minor;
} SVersion;

const SProtocolId S_PROTOCOL_ID;
const SVersion S_PROTOCOL_VERSION;

typedef int8_t SPacketType;
enum SPacketType {
	S_PT_SIMULATION_TICK,
	S_PT_PLAYER_INPUT,
};

typedef struct SPacketHeader {
	SProtocolId protocol_id;
	SVersion protocol_version;
	SPacketType type;
} SPacketHeader;

void s_packet_header_init(void *header, SPacketType type);


/// Game-related types.
// Headings are like in math: radians counterclockwise from the right.
// Integer times are in ticks, floating-point times are in seconds.

typedef uint16_t SPlayerId;

typedef int8_t SPlayerRotation;
enum SPlayerRotation {
	S_PR_NONE,
	S_PR_LEFT,
	S_PR_RIGHT,
};

typedef int8_t SPlayerAcceleration;
enum SPlayerAcceleration {
	S_PA_NONE,
	S_PA_FORWARD,
	S_PA_REVERSE,
};

typedef struct SPlayerInput {
	SPlayerAcceleration accelerate;
	SPlayerRotation rotate;
	bool shoot;
} SPlayerInput;

typedef struct SPlayer {
	SPlayerId id;
	SBool alive;
	SVectorFloat position;
	float heading;
	uint32_t score;
	SColor color;
} SPlayer;

typedef struct SExplosion {
	SVectorFloat position;
	uint16_t n_ticks_since_creation;
} SExplosion;

typedef struct SProjectile {
	SVectorFloat position;
	float heading;
	uint16_t n_ticks_since_creation;
} SProjectile;

typedef uint64_t SSequenceNum;

typedef struct SPlayerInputPacket {
	SSequenceNum sequence_num;
	SPlayerInput input;
} SPlayerInputPacket;

typedef struct SGameSettings {
	float player_timeout; // Seconds.
	SVectorInt level_size;
	uint16_t fps;
	uint16_t projectile_lifetime;
} SGameSettings;

typedef struct SSimulationTickPacket {
	SSequenceNum sequence_num;
	SSequenceNum ack_input_sequence_num;
	SGameSettings game_settings;
	SPlayerId your_player_id;
	SArray players; // Array of SPlayer.
	SArray explosions; // Array of SExplosion.
	SArray projectiles; // Array of SProjectile.
} SSimulationTickPacket;


#pragma pack(pop)
