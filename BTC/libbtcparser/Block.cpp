#include "Block.h"
#include <common/serialization.h>
#include <libhash/hash.h>
#include <common/misc.h>
#include <boost/filesystem/path.hpp>

const u32 magic_number = 0xD9B4BEF9;
const u32 magic_number_testnet = 0x0709110B;

Block::Block(SerializedBuffer &buffer, bool testnet, const block_filter &f, const std::string &path){
	this->parse_file_block(buffer, testnet, f, path);
}

Block::Block(SerializedBuffer &buffer, bool testnet, const std::string &path){
	this->parse_file_block(buffer, testnet, {}, path);
}

Block::Block(SerializedBuffer &buffer, bool testnet, const BlockFromRpc &){
	this->magic_number = !testnet ? ::magic_number : ::magic_number_testnet;
	this->proper_offset = this->offset = 0;
	if (buffer.get_size() > (size_t)std::numeric_limits<decltype(this->block_length)>::max())
		throw std::runtime_error("Block size is larger than 2^32. This should have never happened.");
	this->block_length = (u32)buffer.get_size();
	this->parse_rpc_block(buffer, testnet);
}

void Block::parse_file_block(SerializedBuffer &buffer, bool testnet, const block_filter &f, const std::string &path){
	size_t first_offset;
	auto size = buffer.get_size();
	auto mn = !testnet ? ::magic_number : ::magic_number_testnet;
	while (true){
		first_offset = buffer.get_offset();
		if (buffer.get_offset() > size - 4)
			throw NoMoreBlocks();
		this->proper_offset = buffer.get_offset();
		this->magic_number = buffer.read_u32();
		if (this->magic_number != mn){
			buffer.set_offset(first_offset + 1);
			continue;
		}
		break;
	}
	full_assert(this->magic_number == mn);
	this->path = path;
	this->offset = first_offset;
	this->block_length = buffer.read_u32();

	this->parse_rpc_block(buffer, testnet, f);
}

void Block::parse_rpc_block(SerializedBuffer &buffer, bool testnet, const block_filter &f){
	auto block_start = buffer.get_offset();

	this->version_number = buffer.read_u32();
	this->previous_block_hash = buffer.read_sha256();
	this->merkle_root = buffer.read_sha256();
	this->timestamp = buffer.read_u32();
	this->difficulty = buffer.read_u32();
	this->nonce = buffer.read_u32();

	full_assert(buffer.get_offset() - block_start == BlockHeader_size);

	auto hash = Hashes::Algorithms::SHA256::compute(buffer.get_absolute_buffer(block_start), BlockHeader_size, 2);
	if (!f || f(hash)){
		try{
			buffer.read_sized_vector(this->transactions, this->transaction_count, testnet);
		}catch (std::exception &e){
			throw std::runtime_error("Error parsing block " + (std::string)hash + ": " + e.what());
		}
		int index = 0;
		for (auto &tx : this->transactions){
			tx.transaction_block_index = index++;
#ifdef FIND_UNKNOWN_SCRIPTS
			tx.compute_addresses();
#endif
		}
		this->hash = hash;
	}
	full_assert(buffer.get_offset() <= block_start + this->block_length);
	buffer.set_offset(block_start + this->block_length);
	this->proper_length = buffer.get_offset() - this->proper_offset;
}

u64 Block::estimate_memory_cost() const{
	u64 ret = sizeof(*this);
	for (auto &tx : this->transactions)
		ret += tx.estimate_memory_cost();
	return ret;
}

u64 Block::insert(InsertState &nis, std::set<u64> &updated_balances){
	try{
		auto block_id = nis.insert_block(this->hash, this->previous_block_hash, this->timestamp, (u32)this->transactions.size());
		u32 tx_index = 0;
		for (auto &tx : this->transactions){
			tx.insert(block_id, tx_index++, nis, updated_balances);
		}
		return block_id;
	}catch (std::exception &e){
		std::stringstream stream;
		stream << "Error while processing block " << this->hash << ": " << e.what();
		throw std::runtime_error(stream.str());
	}
}


void AbstractBlockFileParser::parse(){
	SerializedBuffer buffer(this->get_data(), this->get_data_size());
	std::vector<std::unique_ptr<Block>> ret;
	auto path = this->get_path();
	while (buffer.remaining_bytes() && this->continue_running()){
		auto old = buffer.get_offset();
		std::unique_ptr<Block> block;
		try{
			block = std::make_unique<Block>(buffer, this->testnet, path);
		}catch (NoMoreBlocks &){
			this->report_progress(buffer.get_offset() - old);
			break;
		}
		this->on_block(std::move(block));
		this->report_progress(buffer.get_offset() - old);
	}
}

u64 Block::get_average_transaction_size() const{
	u64 ret = 0;
	const auto n = this->transactions.size();
	if (n == 1)
		return 0;
	for (size_t i = 1; i < n; i++)
		ret += this->transactions[i].get_size();
	return ret / (n - 1);
}
