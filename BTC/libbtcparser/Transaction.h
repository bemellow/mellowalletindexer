#pragma once

#include "TxInput.h"
#include "TxOutput.h"
#include "declspec.h"
#include <common/types.h>
#include <libhash/hash.h>
#include <vector>

class SerializedBuffer;
class InsertState;

class LIBBTCPARSER_API Transaction{
	friend class Block;
	friend class TxOutput;
	int transaction_block_index;
	u32 version;
	u64 input_count; //varint
	std::vector<TxInput> inputs;
	u64 output_count; //varint
	std::vector<TxOutput> outputs;
	u32 lock_time;
	bool segwit;
	Hashes::Digests::SHA256 hash;
	Hashes::Digests::SHA256 whash;
	u64 size;

	void read_witness_data(SerializedBuffer &buffer);
public:
	Transaction(SerializedBuffer &buffer, bool testnet);
	const Hashes::Digests::SHA256 &get_hash() const{
		return this->hash;
	}
	const Hashes::Digests::SHA256 &get_whash() const{
		return this->whash;
	}
	void insert(u64 block_id, u32 tx_index, InsertState &nis, std::set<u64> &updated_balances);
	u64 estimate_memory_cost() const;
	const std::vector<TxInput> &get_inputs() const{
		return this->inputs;
	}
	const std::vector<TxOutput> &get_outputs() const{
		return this->outputs;
	}
	std::vector<TxOutput> &get_outputs(){
		return this->outputs;
	}
	void compute_addresses();
	u64 get_size() const{
		return this->size;
	}
};
