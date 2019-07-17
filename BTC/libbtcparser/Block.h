#pragma once

#include "Transaction.h"
#include "declspec.h"
#include <common/types.h>
#include <libhash/hash.h>
#include <functional>
#include <string>
#include <set>

class NoMoreBlocks{};
class SerializedBuffer;
class BlockFromRpc{};

class LIBBTCPARSER_API Block{
public:
	typedef std::function<bool(const Hashes::Digests::SHA256 &)> block_filter;
private:
	u32 magic_number;
	u32 block_length;
	//Block header start
	u32 version_number; //must equal 1
	Hashes::Digests::SHA256 previous_block_hash;
	Hashes::Digests::SHA256 merkle_root;
	u32 timestamp;
	u32 difficulty;
	u32 nonce;
	//Block header end
	u64 transaction_count; //varint
	std::vector<Transaction> transactions;
	std::string path;
	u64 offset;
	u64 proper_offset, proper_length;

	Hashes::Digests::SHA256 hash;
	
	static const size_t BlockHeader_size = 80;
	typedef std::array<u8, BlockHeader_size> BlockHeader;

	void parse_file_block(SerializedBuffer &buffer, bool testnet, const block_filter &, const std::string &path);
	void parse_rpc_block(SerializedBuffer &buffer, bool testnet, const block_filter & = {});

public:
	Block(SerializedBuffer &buffer, bool testnet, const block_filter &, const std::string &path = {});
	Block(SerializedBuffer &buffer, bool testnet, const std::string &path = {});
	Block(SerializedBuffer &buffer, bool testnet, const BlockFromRpc &);
	const Hashes::Digests::SHA256 &get_hash() const{
		return this->hash;
	}
	const Hashes::Digests::SHA256 &get_previous_hash() const{
		return this->previous_block_hash;
	}
	u64 get_timestamp() const{
		return this->timestamp;
	}
	const std::vector<Transaction> &get_transactions() const{
		return this->transactions;
	}
	std::vector<Transaction> &get_transactions(){
		return this->transactions;
	}
	const std::string &get_path() const{
		return this->path;
	}
	u64 get_proper_offset() const{
		return this->proper_offset;
	}
	u64 get_proper_length() const{
		return this->proper_length;
	}
	u64 insert(InsertState &nis, std::set<u64> &updated_balances);
	u64 insert(InsertState &nis){
		std::set<u64> updated_balances;
		return this->insert(nis, updated_balances);
	}
	u64 estimate_memory_cost() const;
	u64 get_average_transaction_size() const;
};

class LIBBTCPARSER_API AbstractBlockFileParser{
protected:
	bool testnet;

	virtual const void *get_data() = 0;
	virtual size_t get_data_size() = 0;
	virtual std::string get_path(){
		return {};
	}
	virtual void on_block(std::unique_ptr<Block> &&) = 0;
	virtual void on_eof(){}
	virtual void report_progress(u64){}
	virtual bool continue_running(){
		return true;
	}
public:
	AbstractBlockFileParser(bool testnet): testnet(testnet){}
	virtual ~AbstractBlockFileParser(){}
	void parse();
};

/*
typedef std::function<void(std::unique_ptr<Block> &&, u64)> block_available_callback;
typedef std::function<bool()> continue_running_callback;

LIBBTCPARSER_API void parse_block_file(
	const std::vector<u8> &buffer,
	const std::string &path,
	const block_available_callback &bac,
	const continue_running_callback &crc = {}
);

inline void parse_block_file(
	const std::vector<u8> &buffer,
	const block_available_callback &bac,
	const continue_running_callback &crc = {}
){
	parse_block_file(buffer, std::string(), bac, crc);
}
*/
