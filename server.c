// When compiling, use "--std=c99 -lm".

#define _POSIX_C_SOURCE 199309L // To get modern time-related functions.
#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <math.h>
#include <time.h>

#include "cpsock.h"
#include "cptime.h"
#include "serialization.h"
#include "vec.h"
#include "color.h"

typedef SVectorInt VectorInt;
typedef SVectorFloat VectorFloat;
typedef SPlayerId PlayerId;
typedef SPlayerInput PlayerInput;
typedef SSequenceNum SequenceNum;

typedef struct Player {
	SPlayerId id;
	struct sockaddr_storage address;

	PlayerInput input;
	SequenceNum input_sequence_num;
	Cptime last_input_time;

	bool alive;
	int ticks_until_respawn;

	VectorFloat position;
	float heading;
	VectorFloat velocity;
	int last_shot_tick;

	int score;
	Color color;
} Player;

typedef struct Projectile {
	VectorFloat position;
	float heading;
	VectorFloat velocity;
	SPlayerId shooter_id;
	int creation_tick;
} Projectile;

typedef struct Explosion {
	VectorFloat position;
	int creation_tick;
} Explosion;

#if !defined(M_PI)
#define M_PI 3.14159265358979323846264338327
#endif

#if defined(PLATFORM_WINDOWS) // Problems with binding an IPv6 socket.
const bool USE_IPV6 = false;
#else
const bool USE_IPV6 = true;
#endif
const unsigned short LISTEN_PORT = 6642;
const float PLAYER_TIMEOUT = 30; // Seconds.

enum { FPS = 30 };

// Sizes (in pixels).
const VectorInt LEVEL_SIZE = {800, 600};
const float PLAYER_RADIUS = 30;

// Speeds and accelerations (in pixels / tick and pixels / tick^2).
const float PLAYER_ACCELERATION = 150.0 / (FPS * FPS);
const float PLAYER_BRAKING = -75.0 / (FPS * FPS);
const float PLAYER_TURN_RATE = M_PI / FPS;
const float SPEED_LIMIT = 500.0 / FPS;
const float PROJECTILE_SPEED = 500.0 / FPS;

// Delays (in ticks).
const int SHOT_COOLDOWN = 0.5 * FPS;
const int PROJECTILE_LIFETIME = 1.5 * FPS;
const int EXPLOSION_LIFETIME = 5 * FPS;
const int PLAYER_RESPAWN_DELAY = 1 * FPS;

Vec players;
Vec projectiles;
Vec explosions;

int curr_tick = 0;
SPlayerId next_player_id = 0;


/// Math utilities.

float normalize_angle(float angle) {
	// Normalize an angle in radians to [0, 2 * M_PI).
	float result = fmod(angle, 2 * M_PI);
	if (result < 0)
		result += 2 * M_PI;
	return result;
}

VectorFloat vector_from_polar(float angle, float length) {
	VectorFloat result;
	result.x = cos(angle) * length;
	result.y = -sin(angle) * length;
	return result;
}

VectorFloat vector_scale(VectorFloat vector, float multiplier) {
	VectorFloat result;
	result.x = vector.x * multiplier;
	result.y = vector.y * multiplier;
	return result;
}

VectorFloat vector_add(VectorFloat a, VectorFloat b) {
	VectorFloat result;
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	return result;
}

VectorFloat vector_subtract(VectorFloat a, VectorFloat b) {
	VectorFloat result;
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	return result;
}

float vector_dot(VectorFloat a, VectorFloat b) {
	return a.x * b.x + a.y * b.y;
}

float vector_length(VectorFloat vector) {
	return sqrt(vector_dot(vector, vector));
}

float vector_distance(VectorFloat a, VectorFloat b) {
	return vector_length(vector_subtract(a, b));
}

VectorFloat vector_velocity_add(VectorFloat u, VectorFloat v) {
	// Pseudo-relativistic velocity addition (so that players don't accelerate to ridiculous speeds).
	float sqr_limit = pow(SPEED_LIMIT, 2);
	return vector_scale(
		vector_add(u, v),
		sqr_limit / (sqr_limit + fmax(0, vector_dot(u, v))));
}

VectorFloat vector_wrap_position(VectorFloat position) {
	if (position.x < 0)
		position.x += LEVEL_SIZE.x;
	else if (position.x > LEVEL_SIZE.x)
		position.x -= LEVEL_SIZE.x;

	if (position.y < 0)
		position.y += LEVEL_SIZE.y;
	else if (position.y > LEVEL_SIZE.y)
		position.y -= LEVEL_SIZE.y;

	return position;
}


/// Physics and other game logic.

Player *player_by_id(SPlayerId id) {
	for (size_t i_player = 0; i_player < players.n_elems; i_player++) {
		Player *player = vec_get(&players, i_player);
		if (player->id == id)
			return player;
	}

	return NULL;
}

VectorFloat find_spacious_position() {
	// Return a position that is approximately the farthest away from screen edges and collidable objects.

	static const float STEP_SIZE = 20;
	VectorFloat curr_position = { STEP_SIZE, STEP_SIZE };

	VectorFloat best_position;
	float best_distance = 0;

	do {
		float curr_distance = INFINITY;

		curr_distance = fmin(curr_distance, curr_position.x);
		curr_distance = fmin(curr_distance, LEVEL_SIZE.x - curr_position.x);
		curr_distance = fmin(curr_distance, curr_position.y);
		curr_distance = fmin(curr_distance, LEVEL_SIZE.y - curr_position.y);

		for (size_t i_player = 0; i_player < players.n_elems; i_player++) {
			Player *player = vec_get(&players, i_player);
			if (player->alive) {
				curr_distance = fmin(
					curr_distance,
					vector_distance(curr_position, player->position));
			}
		}

		for (size_t i_projectile = 0; i_projectile < projectiles.n_elems;
		     i_projectile++) {
			Projectile *projectile = vec_get(&projectiles, i_projectile);
			curr_distance = fmin(
				curr_distance,
				vector_distance(curr_position, projectile->position));
		}

		if (curr_distance > best_distance) {
			best_position = curr_position;
			best_distance = curr_distance;
		}

		curr_position.x += STEP_SIZE;
		if (LEVEL_SIZE.x - curr_position.x <= STEP_SIZE) {
			curr_position.x = STEP_SIZE;
			curr_position.y += STEP_SIZE;
		}
	} while (LEVEL_SIZE.y - curr_position.y > STEP_SIZE);

	return best_position;
}

void player_spawn(Player *player) {
	player->alive = true;

	player->heading = M_PI / 2;
	player->velocity.x = 0;
	player->velocity.y = 0;
	player->position = find_spacious_position();

	player->last_shot_tick = curr_tick;
}

Color next_player_color(void) {
	for (int i_color = 0;; i_color++) {
		Color color = color_distinct(i_color);

		bool color_used = false;
		for (size_t i_player = 0; i_player < players.n_elems; i_player++) {
			Player *player = vec_get(&players, i_player);
			if (color_equal(player->color, color)) {
				color_used = true;
				break;
			}
		}

		if (!color_used)
			return color;
	}
}

void add_player(struct sockaddr_storage address) {
	// Add a player. (Doesn't initialize input or input_sequence_num.)

	// Log connection event.
	char addr_str[CPSOCK_IP_TO_STRING_LEN];
	cpsock_ip_to_string((struct sockaddr *) &address,
	                    addr_str, sizeof(addr_str));
	printf("Player connected: %s, port %d.\n", addr_str,
		   cpsock_ip_port((struct sockaddr *) &address));

	Player new_player;
	new_player.id = next_player_id++;
	new_player.address = address;
	new_player.score = 0;
	new_player.color = next_player_color();
	player_spawn(&new_player);
	vec_push(&players, &new_player);
}

void player_shoot(Player *player) {
	player->last_shot_tick = curr_tick;

	Projectile projectile;
	projectile.shooter_id = player->id;
	projectile.creation_tick = curr_tick;
	projectile.position = vector_wrap_position(
		vector_add(player->position,
		           vector_from_polar(player->heading, PLAYER_RADIUS)));
	projectile.heading = player->heading;
	projectile.velocity = vector_from_polar(
		projectile.heading, PROJECTILE_SPEED);
	vec_push(&projectiles, &projectile);
}

void player_die(Player *player) {
	player->alive = false;
	player->ticks_until_respawn = PLAYER_RESPAWN_DELAY;

	Explosion new_explosion;
	new_explosion.position = player->position;
	new_explosion.creation_tick = curr_tick;
	vec_push(&explosions, &new_explosion);
}

void tick_simulation(void) {
	curr_tick++;

	// Tick explosions (i.e. delete the ones whose lifetime has elapsed).
	for (size_t i_explosion = 0; i_explosion < explosions.n_elems;) {
		Explosion *explosion = vec_get(&explosions, i_explosion);
		if (curr_tick - explosion->creation_tick > EXPLOSION_LIFETIME)
			vec_delete(&explosions, i_explosion);
		else
			i_explosion++;
	}

	// Tick players.
	for (size_t i_player = 0; i_player < players.n_elems; i_player++) {
		Player *player = vec_get(&players, i_player);

		if (!player->alive) {
			player->ticks_until_respawn--;
			if (player->ticks_until_respawn <= 0)
				player_spawn(player);
			continue;
		}

		// Rotation.
		switch (player->input.rotate) {
		case S_PR_LEFT:
			player->heading += PLAYER_TURN_RATE;
			break;
		case S_PR_RIGHT:
			player->heading -= PLAYER_TURN_RATE;
			break;
		default:
			break;
		}
		player->heading = normalize_angle(player->heading);

		// Acceleration.
		float acceleration;
		switch (player->input.accelerate) {
		case S_PA_FORWARD:
			acceleration = PLAYER_ACCELERATION;
			break;
		case S_PA_REVERSE:
			acceleration = PLAYER_BRAKING;
			break;
		default:
			acceleration = 0;
			break;
		}
		player->velocity = vector_velocity_add(
			player->velocity,
			vector_from_polar(player->heading, acceleration));

		// Position.
		player->position = vector_wrap_position(
			vector_add(player->position, player->velocity));

		// Shooting.
		if (player->input.shoot
		    && curr_tick >= player->last_shot_tick + SHOT_COOLDOWN) {
			player_shoot(player);
		}
	}

	// Tick projectiles.
	for (size_t i_projectile = 0; i_projectile < projectiles.n_elems;) {
		Projectile *projectile = vec_get(&projectiles, i_projectile);

		// Position.
		projectile->position = vector_wrap_position(
			vector_add(projectile->position, projectile->velocity));

		// Delete the projectile if its lifetime has elapsed.
		if (curr_tick - projectile->creation_tick > PROJECTILE_LIFETIME)
			vec_delete(&projectiles, i_projectile);
		else
			i_projectile++;
	}

	// Collision detection.
	for (size_t i_player = 0; i_player < players.n_elems; i_player++) {
		Player *player = vec_get(&players, i_player);
		if (!player->alive)
			continue;

		bool player_dies = false;

		// Collisions with other players.
		for (size_t i_other = i_player + 1; i_other < players.n_elems; i_other++) {
			Player *other = vec_get(&players, i_other);
			if (!other->alive)
				continue;

			float distance = vector_distance(player->position, other->position);
			if (distance < PLAYER_RADIUS * 2) {
				player_dies = true;
				player_die(other);
			}
		}

		// Collisions with projectiles.
		for (size_t i_projectile = 0; i_projectile < projectiles.n_elems;) {
			Projectile *projectile = vec_get(&projectiles, i_projectile);

			float distance = vector_distance(
				player->position, projectile->position);
			if (distance < PLAYER_RADIUS) {
				Player *shooter = player_by_id(projectile->shooter_id);
				if (shooter == player)
					shooter->score--;
				else if (shooter != NULL)
					shooter->score++;

				player_dies = true;
				vec_delete(&projectiles, i_projectile);
			} else {
				i_projectile++;
			}
		}

		if (player_dies)
			player_die(player);
	}
}


/// Network.

void clean_up_disconnected_players(void) {
	Cptime time = cptime_time();

	for (size_t i_player = 0; i_player < players.n_elems;) {
		Player *player = vec_get(&players, i_player);
		if (cptime_elapsed(&player->last_input_time, &time) > PLAYER_TIMEOUT) {
			// Log disconnection event.
			char addr_str[CPSOCK_IP_TO_STRING_LEN];
			cpsock_ip_to_string((struct sockaddr *) &player->address,
			                    addr_str, sizeof(addr_str));
			printf("Player disconnected: %s, port %d.\n", addr_str,
			       cpsock_ip_port((struct sockaddr *) &player->address));

			vec_delete(&players, i_player);
		} else {
			i_player++;
		}
	}
}

void on_player_input_packet(struct sockaddr_storage address,
                            SPlayerInputPacket *packet) {
	Player *player = NULL;
	for (size_t i_player = 0; i_player < players.n_elems; i_player++) {
		Player *this_player = vec_get(&players, i_player);
		if (cpsock_ip_equal((struct sockaddr *) &this_player->address,
		                    (struct sockaddr *) &address)) {
			player = this_player;
			break;
		}
	}

	if (player == NULL) {
		add_player(address);
		player = vec_get(&players, players.n_elems - 1);
	} else {
		// Ignore stale input.
		if (packet->sequence_num < player->input_sequence_num)
			return;
	}

	player->input = packet->input;
	player->input_sequence_num = packet->sequence_num;
	player->last_input_time = cptime_time();
}

void receive_packets(int handle) {
	while (true) {
		enum { MAX_PACKET_SIZE = 65515 }; // Max UDP packet size (RFC 768).
		unsigned char packet_data[MAX_PACKET_SIZE];

		struct sockaddr_storage from;
		socklen_t from_size = sizeof(from);
		ssize_t packet_size =
			recvfrom(handle, (char *) packet_data, MAX_PACKET_SIZE,
					 0, (struct sockaddr *) &from, &from_size);

		if (packet_size < 0) // No more packets to process.
			break;

		// Ignore packets with bad size, protocol, version or type.
		if ((unsigned) packet_size < sizeof(SPacketHeader))
			continue;
		SPacketHeader *header = (SPacketHeader *) packet_data;
		if (header->protocol_id != S_PROTOCOL_ID)
			continue;
		SVersion version = header->protocol_version;
		if (version.major != S_PROTOCOL_VERSION.major) {
			fprintf(stderr,
			        "WARNING: received a packet with incompatible version"
			        " %d.%d (mine is %d.%d).\n",
			        version.major, version.minor,
			        S_PROTOCOL_VERSION.major, S_PROTOCOL_VERSION.minor);
			continue;
		}
		if (header->type != S_PT_PLAYER_INPUT) {
			printf("WARNING: Ignoring a packet of unexpected type.\n");
			continue;
		}
		if ((unsigned) packet_size
		    < sizeof(SPacketHeader) + sizeof(SPlayerInputPacket)) {
			fprintf(stderr,
			        "WARNING: received a too small player input packet.\n");
			continue;
		}

		// Process player input packet.
		SPlayerInputPacket *packet = (SPlayerInputPacket *)
			(packet_data + sizeof(SPacketHeader));
		on_player_input_packet(from, packet);
	}
}

void send_sim_tick_packet(int handle, int i_dest_player) {
	Player *dest_player = vec_get(&players, i_dest_player);

	size_t packet_size =
		sizeof(SPacketHeader) +
		sizeof(SSimulationTickPacket) +
		players.n_elems * sizeof(SPlayer) +
		explosions.n_elems * sizeof(SExplosion) +
		projectiles.n_elems * sizeof(SProjectile);

	// Buffer for packets, extended if necessary.
	static void *packet_buffer = NULL;
	static size_t packet_buffer_size = 0;
	if (packet_size > packet_buffer_size) {
		packet_buffer_size = packet_size;
		free(packet_buffer);
		packet_buffer = malloc(packet_buffer_size);
	}

	void *packet_begin = packet_buffer;
	char *packet_end = packet_begin; // char* instead of void* to simplify pointer arithmetic.

	// Header.
	SPacketHeader *header = (SPacketHeader *) packet_end;
	packet_end += sizeof(*header);
	s_packet_header_init(header, S_PT_SIMULATION_TICK);

	// Game settings and other non-array data.
	SSimulationTickPacket *tick_packet = (SSimulationTickPacket *) packet_end;
	packet_end += sizeof(*tick_packet);
	tick_packet->sequence_num = curr_tick;
	tick_packet->ack_input_sequence_num = dest_player->input_sequence_num;
	tick_packet->your_player_id = dest_player->id;
	tick_packet->game_settings.player_timeout = PLAYER_TIMEOUT;
	tick_packet->game_settings.level_size = LEVEL_SIZE;
	tick_packet->game_settings.fps = FPS;
	tick_packet->game_settings.projectile_lifetime = PROJECTILE_LIFETIME;

	// Array headers.
	void *players_array = (char *) tick_packet +
		offsetof(SSimulationTickPacket, players);
	void *explosions_array = (char *) tick_packet +
		offsetof(SSimulationTickPacket, explosions);
	void *projectiles_array = (char *) tick_packet +
		offsetof(SSimulationTickPacket, projectiles);

	// Players.
	s_array_init(players_array, packet_end, players.n_elems);
	for (size_t i_player = 0; i_player < players.n_elems; i_player++) {
		Player *player = vec_get(&players, i_player);
		SPlayer *s_player = (SPlayer *) packet_end;
		packet_end += sizeof(*s_player);
		s_player->id = player->id;
		s_player->alive = !!player->alive;
		s_player->position = player->position;
		s_player->heading = player->heading;
		s_player->score = player->score;
		s_player->color.red = player->color.red;
		s_player->color.green = player->color.green;
		s_player->color.blue = player->color.blue;
	}

	// Explosions.
	s_array_init(explosions_array, packet_end, explosions.n_elems);
	for (size_t i_expl = 0; i_expl < explosions.n_elems; i_expl++) {
		Explosion *explosion = vec_get(&explosions, i_expl);
		SExplosion *s_explosion = (SExplosion *) packet_end;
		packet_end += sizeof(*s_explosion);
		s_explosion->position = explosion->position;
		s_explosion->n_ticks_since_creation =
			curr_tick - explosion->creation_tick;
	}

	// Projectiles.
	s_array_init(projectiles_array, packet_end, projectiles.n_elems);
	for (size_t i_proj = 0; i_proj < projectiles.n_elems; i_proj++) {
		Projectile *projectile = vec_get(&projectiles, i_proj);
		SProjectile *s_projectile = (SProjectile *) packet_end;
		packet_end += sizeof(*s_projectile);
		s_projectile->position = projectile->position;
		s_projectile->heading = projectile->heading;
		s_projectile->n_ticks_since_creation =
			curr_tick - projectile->creation_tick;
	}

	assert(packet_end == (char *) packet_begin + packet_size);

	ssize_t n_sent_bytes =
		sendto(handle, (const char*) packet_begin, packet_size, 0,
		       (struct sockaddr *) &dest_player->address,
		       sizeof(dest_player->address));

	if (n_sent_bytes < 0 || (unsigned) n_sent_bytes != packet_size) {
		perror("ERROR: Failed to send packet");
		exit(EXIT_FAILURE);
	}
}


/// Main.

void main_loop(int handle) {
	vec_init(&players, sizeof(Player));
	vec_init(&explosions, sizeof(Explosion));
	vec_init(&projectiles, sizeof(Projectile));

	const double tick_interval = 1.0 / FPS;
	double sleep_time = 0;
	Cptime last_iter_time = cptime_time();

	while (true) {
		receive_packets(handle);
		clean_up_disconnected_players();
		tick_simulation();
		for (size_t i_player = 0; i_player < players.n_elems; i_player++)
			send_sim_tick_packet(handle, i_player);

		// Self-adjusting sleep that makes the loop contents execute every TICK_INTERVAL seconds.
		Cptime this_iter_time = cptime_time();
		double time_since_last_iter =
			cptime_elapsed(&last_iter_time, &this_iter_time);
		last_iter_time = this_iter_time;
		sleep_time += tick_interval - time_since_last_iter;
		if (sleep_time > 0)
			cptime_sleep(sleep_time);
	}
}

int main() {
	srand(time(NULL));
	cpsock_initialize();

	int handle = socket((USE_IPV6 ? AF_INET6 : AF_INET),
	                    SOCK_DGRAM, IPPROTO_UDP);
	if (handle <= 0) {
		perror("ERROR: Failed to create socket");
		exit(EXIT_FAILURE);
	}

	if (!cpsock_set_nonblocking(handle)) {
		perror("ERROR: Failed to set socket to non-blocking mode");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_storage address;
	memset(&address, 0, sizeof(address));
	if (USE_IPV6) {
		struct sockaddr_in6 *addr = (struct sockaddr_in6 *) &address;
		addr->sin6_family = AF_INET6;
		addr->sin6_addr = in6addr_any;
		addr->sin6_port = htons(LISTEN_PORT);
	} else {
		struct sockaddr_in *addr = (struct sockaddr_in *) &address;
		addr->sin_family = AF_INET;
		addr->sin_addr.s_addr = INADDR_ANY;
		addr->sin_port = htons(LISTEN_PORT);
	}

	if (bind(handle, (const struct sockaddr *) &address, sizeof(address)) < 0) {
		perror("ERROR: Failed to bind socket");
		exit(EXIT_FAILURE);
	}

	char address_str[CPSOCK_IP_TO_STRING_LEN];
	cpsock_ip_to_string((struct sockaddr *) &address,
	                    address_str, sizeof(address_str));
	printf("Listening on %s, port %d.\n", address_str,
	       cpsock_ip_port((struct sockaddr *) &address));

	main_loop(handle);

	cpsock_close(handle);

	cpsock_shutdown();

	return EXIT_SUCCESS;
}
