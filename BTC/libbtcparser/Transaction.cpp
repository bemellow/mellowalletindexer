#include "Transaction.h"
#include "Block.h"
#include <common/serialization.h>
#include <sstream>

Transaction::Transaction(SerializedBuffer &buffer, bool testnet){
	using Hashes::Algorithms::SHA256;

	SHA256 txid_sha;

	auto first_offset = buffer.get_offset();
	this->version = buffer.read_u32();
	if (buffer.get_offset() + 2 > buffer.get_size())
		throw std::runtime_error("Invalid block.");
	auto segwit_check = (u8 *)buffer.get_buffer();
	this->segwit = segwit_check[0] == 0 && segwit_check[1] == 1;

	txid_sha.update(buffer.get_absolute_buffer(first_offset), buffer.get_offset() - first_offset);
	
	if (this->segwit)
		buffer.read_u16(); //ignore segwit marker

	auto second_offset = buffer.get_offset();
	buffer.read_sized_vector(this->inputs, this->input_count);
	buffer.read_sized_vector(this->outputs, this->output_count, *this, testnet);

	txid_sha.update(buffer.get_absolute_buffer(second_offset), buffer.get_offset() - second_offset);

	if (this->segwit)
		this->read_witness_data(buffer);

	auto third_offset = buffer.get_offset();

	this->lock_time = buffer.read_u32();

	txid_sha.update(buffer.get_absolute_buffer(third_offset), buffer.get_offset() - third_offset);
	
	auto txid_digest = txid_sha.final();

	this->hash = SHA256::compute(txid_digest);
	this->size = buffer.get_offset() - first_offset;
	this->whash = SHA256::compute(buffer.get_absolute_buffer(first_offset), this->size, 2);
}

u64 Transaction::estimate_memory_cost() const{
	return sizeof(*this) + this->inputs.size() * sizeof(TxInput) + this->outputs.size() * sizeof(TxOutput);
}

void Transaction::compute_addresses(){
	int i = 0;
	try{
		for (auto &out : this->outputs){
			out.compute_output_addresses();
			i++;
		}
	}catch (std::exception &e){
		std::stringstream stream;
		stream << "Error while parsing transaction " << this->hash << ", out " << i << ": " << e.what();
		throw std::runtime_error(stream.str());
	}
}

void Transaction::read_witness_data(SerializedBuffer &buffer){
	for (auto &input : this->inputs)
		input.read_witnesses(buffer);
}

void Transaction::insert(u64 block_id, u32 tx_index, InsertState &nis, std::set<u64> &updated_balances){
	try{
		auto tx_id = nis.insert_tx(this->hash, this->whash, this->lock_time, block_id, tx_index, (u32)this->inputs.size(), (u32)this->outputs.size());
		u32 txi_index = 0;
		std::set<u64> addresses;
		for (auto &in : this->inputs){
			auto previous_output = in.insert(tx_id, txi_index++, nis);
			if (previous_output != std::numeric_limits<u64>::max()){
				auto temp = nis.get_addresses_for_output(previous_output);
				for (auto &id : temp)
					addresses.insert(id);
			}
		}
		u32 txo_index = 0;
		for (auto &out : this->outputs){
			auto temp = out.insert(tx_id, txo_index++, nis);
			for (auto &id : temp)
				addresses.insert(id);
		}
		nis.add_addresses_tx_relations(tx_id, addresses);
		for (auto id : addresses)
			updated_balances.insert(id);
	}catch (std::exception &e){
		std::stringstream stream;
		stream << "Error while processing transaction " << this->hash << ": " << e.what();
		throw std::runtime_error(stream.str());
	}
}
