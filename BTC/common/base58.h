#pragma once

#include "misc.h"
#include <libhash/hash.h>
#include <libmisc/declspec.h>
#include <memory>
#include <vector>
#include <string>

LIBMISC_API std::unique_ptr<std::vector<u8>> base58_to_binary(const char *psz);
LIBMISC_API std::unique_ptr<std::vector<u8>> base58_to_binary(const std::string &str);
LIBMISC_API std::string binary_to_base58(const void *buffer, size_t buffer_size);
LIBMISC_API std::string binary_to_base58(const std::vector<u8> &vch);
LIBMISC_API std::string binary_to_base58_check(const void *buffer, size_t size);
LIBMISC_API std::string binary_to_base58_check(const std::vector<u8> &input);
LIBMISC_API std::unique_ptr<std::vector<u8>> base58_to_binary_check(const char *psz);
LIBMISC_API std::unique_ptr<std::vector<u8>> base58_to_binary_check(const std::string &str);

inline std::string to_base58(const Hashes::Digests::RIPEMD160 &hash){
	auto &array = hash.to_array();
	return binary_to_base58_check(array.data(), array.size());
}

inline std::string to_base58(u8 version, const Hashes::Digests::RIPEMD160 &hash){
	const auto s = Hashes::Digests::RIPEMD160::size;
	u8 temp[s + 1];
	temp[0] = version;
	auto &array = hash.to_array();
	memcpy(temp + 1, array.data(), s);
	return binary_to_base58_check(temp, sizeof(temp));
}
