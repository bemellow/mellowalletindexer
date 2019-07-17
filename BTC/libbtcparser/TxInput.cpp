#include "TxInput.h"
#include "Block.h"
#include <common/serialization.h>

TxInput::TxInput(SerializedBuffer &buffer){
	this->previous_tx = buffer.read_sha256();
	this->transaction_index = buffer.read_u32();
	buffer.ignore_sized_buffer();
	auto sequence = buffer.read_u32();
}

void TxInput::read_witnesses(SerializedBuffer &buffer){
	for (auto i = buffer.read_varint(); i--;)
		this->witnesses.emplace_back(buffer.read_sized_buffer());
}

u64 TxInput::insert(u64 txid, u32 txi_index, InsertState &nis) const{
	u64 ret;
	nis.insert_input(this->previous_tx, this->transaction_index, txid, txi_index, ret);
	return ret;
}
