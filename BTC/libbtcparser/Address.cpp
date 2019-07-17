#include "Address.h"
#include <libhash/hash.h>
#include <common/bech32/segwit_addr.h>
#include <common/base58.h>

Address::Address(): type(AddressType::Unset){
	memset(this->buffer, 0, sizeof(this->buffer));
}

Address::Address(AddressType type, const void *src, bool testnet)
		: type(type)
		, testnet(testnet){
	const auto s = Hashes::Digests::RIPEMD160::size;
	switch (this->type){
		case AddressType::P2pk:
			memcpy(this->buffer, src, s);
			break;
		case AddressType::P2sh:
			memcpy(this->buffer, src, s);
			break;
		case AddressType::P2wpkh20:
			memcpy(this->buffer, src, 20);
			break;
		case AddressType::P2wpkh32:
			memcpy(this->buffer, src, 32);
			break;
		default:
			throw std::exception();
	}
}

Address::operator std::string() const{
	size_t size;

	auto type = (u8)this->type;
	if (this->testnet){
		switch (this->type){
			case AddressType::P2pk:
				type = 111;
				break;
			case AddressType::P2sh:
				type = 196;
				break;
		}
	}

	switch (this->type){
		case AddressType::P2pk:
		case AddressType::P2sh:
			{
				const auto s = Hashes::Digests::RIPEMD160::size;
				u8 temp[s + 1];
				temp[0] = type;
				memcpy(temp + 1, this->buffer, s);
				return binary_to_base58_check(temp, sizeof(temp));
			}
		case AddressType::P2wpkh20:
			size = 20;
			break;
		case AddressType::P2wpkh32:
			size = 32;
			break;
		default:
			throw std::exception();
	}
	return segwit_addr::encode(this->buffer, size);
}

size_t Address::size() const{
	switch (this->type){
		case AddressType::P2pk:
		case AddressType::P2sh:
		case AddressType::P2wpkh20:
			return 20;
		case AddressType::P2wpkh32:
			return 32;
		default:
			throw std::exception();
	}
}

bool Address::operator<(const Address &other) const{
	size_t ts = this->size();
	size_t os = other.size();
	int x = memcmp(this->buffer, other.buffer, std::min(ts, os));
	if (x < 0)
		return true;
	if (x > 0)
		return false;
	return ts < os;
}

bool Address::operator==(const Address &other) const{
	size_t ts = this->size();
	size_t os = other.size();
	if (ts != os)
		return false;
	return !memcmp(this->buffer, other.buffer, ts);
}
