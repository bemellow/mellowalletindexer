#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include <string>
#include <cstring>

typedef std::uint8_t u8;
typedef std::uint64_t u64;

extern const char hex_digits[];

inline unsigned hex2val(char c){
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	return c - 'A' + 10;
}

class SHA256{
public:
	static const size_t size = 32;
	typedef std::array<u8, size> digest_t;
	static const size_t string_size =  2 + size * 2 + 1;
private:
	digest_t digest;
public:
	SHA256(){
		memset(this->digest.data(), 0, size);
	}
	SHA256(const std::string &digest);
	SHA256(const char *digest, size_t size = 0);
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
