#pragma once

#include "SHA256.h"
#include <boost/optional.hpp>
#include <functional>
#include <string>
#include <map>

struct ChainReorganizationBlock{
	SHA256 hash;
	u64 db_id;
	u64 height;
};

struct ChainReorganization{
	boost::optional<SHA256> block_required;
	std::vector<ChainReorganizationBlock> blocks_to_revert;
	operator std::string() const;
};

struct HeadCandidate{
	u64 id;
	u64 previous_id;
	SHA256 hash;
	SHA256 previous_hash;
	HeadCandidate *previous;
	bool has_any_children;
};

typedef bool (*get_block_data_callback)(u64 *id, SHA256 *hash, SHA256 *previous_hash);
typedef bool (*get_blockchain_head_callback)(SHA256 *hash);

enum class RevertBlockResult{
	Success = 0,
	EmptyBlockchain,
	InvalidRevert,
};

class Blockchain{
public:
	struct Block{
		SHA256 hash;
		SHA256 previous_hash;
		u64 height;
		u64 db_id;
		u64 previous_block_id;
		operator std::string() const;
	};
	typedef std::function<size_t(const std::vector<const HeadCandidate *> &)> head_selector_t;
private:
	std::vector<Block> blockchain;
	std::map<SHA256, size_t> block_map;
public:
	Blockchain(get_block_data_callback c1, get_blockchain_head_callback c2, const head_selector_t &head_selector = {});
	Blockchain(const Blockchain &) = delete;
	Blockchain(Blockchain &&) = delete;
	const Blockchain &operator=(const Blockchain &) = delete;
	const Blockchain &operator=(Blockchain &&) = delete;
	ChainReorganization try_add_new_block(const SHA256 &previous_hash);
	u64 add_new_block(const SHA256 &hash, const SHA256 &previous_block, u64 db_id);
	boost::optional<Block> get_block_by_height(u64 height) const{
		if (height >= this->blockchain.size())
			return {};
		return this->blockchain[height];
	}
	boost::optional<Block> get_block_by_hash(const SHA256 &hash) const{
		auto it = this->block_map.find(hash);
		if (it == this->block_map.end())
			return {};
		return this->blockchain[it->second];
	}
	u64 get_height() const{
		return this->blockchain.size() - 1;
	}
	boost::optional<SHA256> get_head() const{
		if (!this->blockchain.size())
			return {};
		return this->blockchain.back().hash;
	}
	RevertBlockResult revert_block(const SHA256 &hash);
};
