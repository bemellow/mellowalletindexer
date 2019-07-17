#pragma once
#include <common/types.h>
#include "declspec.h"
#include <string>

enum class AddressType{
	Unspendable = -3,
	Unknown = -2,
	Unset = -1,
	P2pk = 0,
	P2pkh = 0,
	P2sh = 5,
	P2wpkh20 = 256,
	P2wpkh32 = 257,
};

struct LIBBTCPARSER_API Address{
	AddressType type;
	bool testnet = false;
	u8 buffer[32];
	Address();
	Address(AddressType type, const void *src, bool testnet);
	operator std::string() const;
	size_t size() const;
	bool operator<(const Address &other) const;
	bool operator==(const Address &other) const;
	bool operator!=(const Address &other) const{
		return !(*this == other);
	}
};
