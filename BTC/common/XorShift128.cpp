#include "XorShift128.h"
#include <random>

std::uint32_t BasicXorShift128::gen(){
	auto &state = this->state.data;
	auto x = state[3];
	x ^= x << 11;
	x ^= x >> 8;
	state[3] = state[2];
	state[2] = state[1];
	state[1] = state[0];
	x ^= state[0];
	x ^= state[0] >> 19;
	state[0] = x;
	return x;
}

xorshift128_state get_seed(){
	xorshift128_state ret;
	std::random_device rnd;
	for (auto &i : ret.data)
		i = rnd();
	return ret;
}

BasicXorShift128::BasicXorShift128(){
	this->state = get_seed();
}
