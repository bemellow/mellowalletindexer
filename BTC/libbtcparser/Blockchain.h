#pragma once

#include <libhash/hash.h>
#include <sqlitepp/sqlitepp.h>
#include <boost/optional.hpp>
#include <functional>

struct ChainReorganizationBlock{
	using SHA256 = Hashes::Digests::SHA256;
	SHA256 hash;
	u64 db_id;
	u64 height;
};

struct ChainReorganization{
	using SHA256 = Hashes::Digests::SHA256;
	boost::optional<SHA256> block_required;
	std::vector<ChainReorganizationBlock> blocks_to_revert;
};

struct HeadCandidate{
	using SHA256 = Hashes::Digests::SHA256;
	u64 id;
	u64 previous_id;
	SHA256 hash;
	SHA256 previous_hash;
	HeadCandidate *previous;
	bool has_any_children;
};

enum class RevertBlockResult{
	Success = 0,
	EmptyBlockchain,
	InvalidRevert,
};

class Blockchain{
public:
	using SHA256 = Hashes::Digests::SHA256;
	struct Block{
		SHA256 hash;
		SHA256 previous_hash;
		u64 height;
		u64 db_id;
		u64 previous_block_id;
	};
	typedef std::function<size_t(const std::vector<const HeadCandidate *> &)> head_selector_t;
private:
	sqlite3pp::DB &db;
	std::vector<Block> blockchain;
	std::map<SHA256, size_t> block_map;

	void update_db();
public:
	Blockchain(sqlite3pp::DB &db, const head_selector_t &head_selector = {});
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
	boost::optional<Block> get_block_by_hash(const Hashes::Digests::SHA256 &hash) const{
		auto it = this->block_map.find(hash);
		if (it == this->block_map.end())
			return {};
		return this->blockchain[it->second];
	}
	u64 get_height() const{
		return this->blockchain.size() - 1;
	}
	RevertBlockResult revert_block(const SHA256 &hash);
};
