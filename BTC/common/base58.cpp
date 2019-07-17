#include "base58.h"

#include <libhash/hash.h>
#include <cassert>

// Copyright (c) 2014-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/** All alphanumeric characters except for "0", "I", "O", and "l" */
static const char* pszBase58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
static const int8_t mapBase58[256] = {
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1, 0, 1, 2, 3, 4, 5, 6,  7, 8,-1,-1,-1,-1,-1,-1,
    -1, 9,10,11,12,13,14,15, 16,-1,17,18,19,20,21,-1,
    22,23,24,25,26,27,28,29, 30,31,32,-1,-1,-1,-1,-1,
    -1,33,34,35,36,37,38,39, 40,41,42,43,-1,44,45,46,
    47,48,49,50,51,52,53,54, 55,56,57,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
};

constexpr inline bool IsSpace(char c) noexcept {
    return c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v';
}

LIBMISC_API std::unique_ptr<std::vector<u8>> base58_to_binary(const char *psz){
    // Skip leading spaces.
    while (*psz && IsSpace(*psz))
        psz++;
    // Skip and count leading '1's.
    int zeroes = 0;
    int length = 0;
    while (*psz == '1') {
        zeroes++;
        psz++;
    }
    // Allocate enough space in big-endian base256 representation.
    auto size = strlen(psz) * 733 /1000 + 1; // log(58) / log(256), rounded up.
    std::vector<unsigned char> b256(size);
    // Process the characters.
    static_assert(sizeof(mapBase58)/sizeof(mapBase58[0]) == 256, "mapBase58.size() should be 256"); // guarantee not out of range
    while (*psz && !IsSpace(*psz)) {
        // Decode base58 character
        int carry = mapBase58[(uint8_t)*psz];
        if (carry == -1)  // Invalid b58 character
            return nullptr;
        int i = 0;
        for (std::vector<unsigned char>::reverse_iterator it = b256.rbegin(); (carry != 0 || i < length) && (it != b256.rend()); ++it, ++i) {
            carry += 58 * (*it);
            *it = carry % 256;
            carry /= 256;
        }
        assert(carry == 0);
        length = i;
        psz++;
    }
    // Skip trailing spaces.
    while (IsSpace(*psz))
        psz++;
    if (*psz != 0)
        return nullptr;
    // Skip leading zeroes in b256.
    std::vector<unsigned char>::iterator it = b256.begin() + (size - length);
    while (it != b256.end() && *it == 0)
        it++;
	std::vector<u8> ret;
    // Copy result into output vector.
    ret.reserve(zeroes + (b256.end() - it));
    ret.assign(zeroes, 0x00);
    while (it != b256.end())
        ret.push_back(*(it++));
    return std::make_unique<std::vector<u8>>(std::move(ret));
}

LIBMISC_API std::unique_ptr<std::vector<u8>> base58_to_binary(const std::string &str){
    return base58_to_binary(str.c_str());
}

LIBMISC_API std::string binary_to_base58(const void *buffer, size_t buffer_size){
	auto pbegin = (const unsigned char *)buffer;
	auto pend = pbegin + buffer_size;
    
	// Skip & count leading zeroes.
    int zeroes = 0;
    int length = 0;
    while (pbegin != pend && *pbegin == 0) {
        pbegin++;
        zeroes++;
    }
    // Allocate enough space in big-endian base58 representation.
    auto size = (pend - pbegin) * 138 / 100 + 1; // log(256) / log(58), rounded up.
    std::vector<u8> b58(size);
    // Process the bytes.
    while (pbegin != pend) {
        int carry = *pbegin;
        int i = 0;
        // Apply "b58 = b58 * 256 + ch".
        for (auto it = b58.rbegin(); (carry != 0 || i < length) && (it != b58.rend()); it++, i++) {
            carry += 256 * (*it);
            *it = carry % 58;
            carry /= 58;
        }

        assert(carry == 0);
        length = i;
        pbegin++;
    }
    // Skip leading zeroes in base58 result.
    auto it = b58.begin() + (size - length);
    while (it != b58.end() && *it == 0)
        it++;
    // Translate the result into a string.
    std::string str;
    str.reserve(zeroes + (b58.end() - it));
    str.assign(zeroes, '1');
    while (it != b58.end())
        str += pszBase58[*(it++)];
    return str;
}

LIBMISC_API std::string binary_to_base58(const std::vector<unsigned char> &vch){
	char temp;
	if (!vch.size())
		return binary_to_base58(&temp, 0);
    return binary_to_base58(&vch[0], vch.size());
}

LIBMISC_API Hashes::Digests::SHA256::digest_t compute_double_sha256(const void *buffer, size_t size){
	return Hashes::Algorithms::SHA256::compute(buffer, size, 2).to_array();
}

LIBMISC_API std::string binary_to_base58_check(const void *buffer, size_t size){
	std::vector<u8> temp((const u8 *)buffer, (const u8 *)buffer + size);
	
    auto hash = compute_double_sha256(&temp[0], temp.size());
    temp.insert(temp.end(), hash.data(), hash.data() + 4);
    return binary_to_base58(temp);
}

LIBMISC_API std::string binary_to_base58_check(const std::vector<u8> &input){
    std::vector<u8> temp(input);
    auto hash = compute_double_sha256(&temp[0], temp.size());
    temp.insert(temp.end(), hash.data(), hash.data() + 4);
    return binary_to_base58(temp);
}

LIBMISC_API std::unique_ptr<std::vector<u8>> base58_to_binary_check(const char *psz){
	auto ret = base58_to_binary(psz);
    if (!ret || ret->size() < 4)
        return nullptr;
    
	auto p = &(*ret)[0];
	// re-calculate the checksum, ensure it matches the included 4-byte checksum
    auto hash = compute_double_sha256(p, ret->size() - 4);
    if (memcmp(hash.data(), p + ret->size() - 4, 4) != 0)
        return nullptr;
    ret->resize(ret->size() - 4);
    return ret;
}

LIBMISC_API std::unique_ptr<std::vector<u8>> base58_to_binary_check(const std::string &str){
    return base58_to_binary_check(str.c_str());
}
