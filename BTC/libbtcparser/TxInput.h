#pragma once
#include "InsertState.h"
#include "declspec.h"
#include <libhash/hash.h>
#include <vector>

class SerializedBuffer;
class InsertState;

class LIBBTCPARSER_API TxInput{
	Hashes::Digests::SHA256 previous_tx;
	u32 transaction_index;
	std::vector<std::vector<u8>> witnesses;

public:
	TxInput(SerializedBuffer &buffer);
	u64 insert(u64 txid, u32 txi_index, InsertState &nis) const;
	const Hashes::Digests::SHA256 &get_previous_tx() const{
		return this->previous_tx;
	}
	const u32 &get_transaction_index() const{
		return this->transaction_index;
	}
	void read_witnesses(SerializedBuffer &buffer);
};
