#pragma once
#include <libmisc/declspec.h>
#include <cstdint>
#include <algorithm>

struct xorshift128_state{
	std::uint32_t data[4];
};

class LIBMISC_API BasicXorShift128{
protected:
	xorshift128_state state;
	std::uint32_t gen();
public:
	BasicXorShift128();
	BasicXorShift128(const xorshift128_state &seed): state(seed){}
	BasicXorShift128(xorshift128_state &&seed): state(std::move(seed)){}
};

class LIBMISC_API XorShift128_32 : public BasicXorShift128{
public:
	XorShift128_32() = default;
	XorShift128_32(const xorshift128_state &seed): BasicXorShift128(seed){}
	XorShift128_32(xorshift128_state &&seed): BasicXorShift128(std::move(seed)){}
	std::uint32_t operator()(){
		return this->gen();
	}
};

class LIBMISC_API XorShift128_64 : public BasicXorShift128{
public:
	XorShift128_64() = default;
	XorShift128_64(const xorshift128_state &seed): BasicXorShift128(seed){}
	XorShift128_64(xorshift128_state &&seed): BasicXorShift128(std::move(seed)){}
	std::uint64_t operator()(){
		return ((std::uint64_t)this->gen() << 32) | this->gen();
	}
};
