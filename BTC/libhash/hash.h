#pragma once

#include <common/misc.h>
#include "sha256_state.h"
#include "ripemd160_state.h"
#include "declspec.h"
#include <array>

namespace Hashes{

namespace Digests{

class LIBHASH_API SHA256{
public:
	static const size_t size = 32;
	typedef std::array<u8, size> digest_t;
	static const size_t string_size = size * 2 + 1;
private:
	digest_t digest;
public:
	SHA256(){
		memset(this->digest.data(), 0, size);
	}
	SHA256(const std::string &digest);
	SHA256(const char *digest, size_t size);
	SHA256(const digest_t &digest): digest(digest){}
	SHA256(const SHA256 &other) = default;
	int cmp(const SHA256 &other) const;
	bool operator==(const SHA256 &other) const{
		return !this->cmp(other);
	}
	bool operator!=(const SHA256 &other) const{
		return !!this->cmp(other);
	}
	bool operator<(const SHA256 &other) const{
		return this->cmp(other) < 0;
	}
	bool operator>(const SHA256 &other) const{
		return this->cmp(other) > 0;
	}
	bool operator<=(const SHA256 &other) const{
		return this->cmp(other) <= 0;
	}
	bool operator>=(const SHA256 &other) const{
		return this->cmp(other) >= 0;
	}
	bool operator!() const{
		for (auto b : this->digest)
			if (b)
				return false;
		return true;
	}
	operator std::string() const;
	void write_to_char_array(char (&array)[string_size]) const;
	void write_to_char_vector(std::vector<char> &) const;
	const digest_t &to_array() const{
		return this->digest;
	}
	digest_t &to_array(){
		return this->digest;
	}
};

class LIBHASH_API RIPEMD160{
public:
	static const size_t size = 20;
	typedef std::array<u8, size> digest_t;
private:
	digest_t digest;
public:
	RIPEMD160(){
		memset(this->digest.data(), 0, size);
	}
	RIPEMD160(const digest_t &digest): digest(digest){}
	RIPEMD160(const RIPEMD160 &other) = default;
	int cmp(const RIPEMD160 &other) const;
	bool operator==(const RIPEMD160 &other) const{
		return !this->cmp(other);
	}
	bool operator!=(const RIPEMD160 &other) const{
		return !!this->cmp(other);
	}
	bool operator<(const RIPEMD160 &other) const{
		return this->cmp(other) < 0;
	}
	bool operator>(const RIPEMD160 &other) const{
		return this->cmp(other) > 0;
	}
	bool operator<=(const RIPEMD160 &other) const{
		return this->cmp(other) <= 0;
	}
	bool operator>=(const RIPEMD160 &other) const{
		return this->cmp(other) >= 0;
	}
	bool operator!() const{
		for (auto b : this->digest)
			if (b)
				return false;
		return true;
	}
	const digest_t &to_array() const{
		return this->digest;
	}
};

} //Digests

namespace Algorithms{

class LIBHASH_API SHA256{
	SHA256_CTX state;
public:
	typedef Digests::SHA256 digest_t;

	SHA256(){
		this->reset();
	}
	void reset();
	void update(const void *buffer, size_t size);
	digest_t final();

	static digest_t compute(const void *buffer, size_t size, unsigned iterations = 1);
	static digest_t compute(const std::vector<u8> &buffer, unsigned iterations = 1);
	template <size_t N>
	static digest_t compute(const std::array<u8, N> &array, unsigned iterations = 1){
		return compute(array.data(), array.size(), iterations);
	}
	static digest_t compute(const digest_t &data){
		return compute(data.to_array());
	}
};

class LIBHASH_API RIPEMD160{
	ripemd160_state state;
public:
	typedef Digests::RIPEMD160 digest_t;

	RIPEMD160(){
		this->reset();
	}
	void reset();
	void update(const void *buffer, size_t size);
	Digests::RIPEMD160 final();

	static digest_t compute(const void *buffer, size_t size);
	static digest_t compute(const std::vector<u8> &buffer);
	template <size_t N>
	static digest_t compute(const std::array<u8, N> &array){
		return compute(array.data(), array.size());
	}
	static digest_t compute(const digest_t &data){
		return compute(data.to_array());
	}
};

} //Algorithms

} //Hashes

inline std::ostream &operator<<(std::ostream &stream, const Hashes::Digests::SHA256 &digest){
	return stream << (std::string)digest;
}
