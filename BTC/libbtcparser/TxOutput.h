#pragma once

#include "Address.h"
#include "declspec.h"
#include "InsertState.h"
#include <vector>
#include <set>

class Transaction;
class SerializedBuffer;
class InsertState;

class LIBBTCPARSER_API TxOutput{
	Transaction *parent;
	bool testnet;
	u64 value;
	AddressType address_type = AddressType::Unset;
	std::vector<u8> script;
	std::vector<Address> addresses;
	int required_spenders = 1;
public:
	TxOutput(SerializedBuffer &buffer, Transaction &parent, bool testnet);
	std::set<u64> insert(u64 txid, u32 txo_index, InsertState &nis);
	void compute_output_addresses();
	u64 get_value() const{
		return this->value;
	}
	const std::vector<Address> &get_addresses() const{
		return this->addresses;
	}
	int get_required_spenders() const{
		return this->required_spenders;
	}
};
