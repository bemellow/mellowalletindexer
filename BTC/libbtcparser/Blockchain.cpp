#include "Blockchain.h"

using Hashes::Digests::SHA256;
using namespace sqlite3pp;

const auto max64 = std::numeric_limits<u64>::max();

struct BlocksTemp{
	std::vector<HeadCandidate *> blocks;
	std::map<SHA256, HeadCandidate> block_map;
};

BlocksTemp build_blocks(DB &db){
	auto stmt = db << "select id, hash, previous_hash from blocks;";
	BlocksTemp ret;
	while (stmt.step() == SQLITE_ROW){
		u64 id;
		std::string hash, previous_hash;
		stmt >> id >> hash >> previous_hash;
		SHA256 sha(hash);
		auto &block = ret.block_map[sha];
		block = {id, 0, sha, previous_hash, nullptr, false};
		ret.blocks.push_back(&block);
	}
	for (auto &block : ret.blocks){
		auto previous = ret.block_map.find(block->previous_hash);
		if (previous == ret.block_map.end())
			continue;
		block->previous = &previous->second;
		block->previous->has_any_children = true;
	}
	return ret;
}

const HeadCandidate *select_head(DB &db, const BlocksTemp &blocks, const Blockchain::head_selector_t &head_selector){
	std::vector<const HeadCandidate *> heads;
	for (auto &block : blocks.blocks)
		if (!block->has_any_children)
			heads.push_back(block);
	if (!heads.size())
		throw std::runtime_error("Invalid blockchain. Circular topologies are invalid.");
	const HeadCandidate *ret;
	if (heads.size() != 1){
		std::string hash;
		try{
			auto stmt = db << "select hash from blockchain_head limit 1;";
			if (stmt.step() == SQLITE_ROW)
				stmt >> hash;
		}catch (std::exception &){}
		if (!hash.size()){
			if (!head_selector)
				throw std::runtime_error("Can't read blockchain head from DB.");
			hash = heads[head_selector(heads)]->hash;
		}
		auto it = blocks.block_map.find(hash);
		if (it == blocks.block_map.end())
			throw std::runtime_error("DB states that the blockchain head is " + hash + ", but no such block exists in the DB.");
		ret = &it->second;
	}else
		ret = heads.front();
	return ret;
}

std::vector<HeadCandidate> assemble_blockchain(DB &db, const Blockchain::head_selector_t &head_selector){
	std::vector<HeadCandidate> ret;
	auto blocks = build_blocks(db);
	if (!blocks.blocks.size())
		return ret;
	ret.reserve(blocks.blocks.size());
	for (auto current = select_head(db, blocks, head_selector); current; current = current->previous){
		ret.push_back(*current);
		auto &back = ret.back();
		if (current->previous)
			back.previous_id = current->previous->id;
		else
			back.previous_id = std::numeric_limits<decltype(back.previous_id)>::max();
		back.previous = nullptr;
	}
	std::reverse(ret.begin(), ret.end());
	return ret;
}

Blockchain::Blockchain(DB &db, const head_selector_t &head_selector): db(db){
	auto blocks = assemble_blockchain(this->db, head_selector);
	this->blockchain.reserve(blocks.size());
	for (auto &block : blocks){
		auto height = this->blockchain.size();
		Block block2;
		block2.hash = block.hash;
		block2.previous_hash = block.previous_hash;
		block2.height = height;
		block2.db_id = block.id;
		block2.previous_block_id = block.previous_id;
		this->block_map[block2.hash] = height;
		this->blockchain.push_back(block2);
	}
	this->update_db();
}

void Blockchain::update_db(){
	this->db.exec("delete from blockchain_head;");
	if (!this->blockchain.size())
		return;
	this->db << "insert into blockchain_head (hash) values (?);" << (std::string)this->blockchain.back().hash << Step();
}

ChainReorganization Blockchain::try_add_new_block(const SHA256 &previous_hash){
	ChainReorganization ret;
	if (!this->blockchain.size())
		return ret;
	auto it = this->block_map.find(previous_hash);
	if (it == this->block_map.end()){
		ret.block_required = previous_hash;
		return ret;
	}
	auto first_reverted = it->second + 1;
	ret.blocks_to_revert.reserve(this->blockchain.size() - first_reverted);
	for (auto i = first_reverted; i < this->blockchain.size(); i++){
		auto &block = this->blockchain[i];
		ChainReorganizationBlock crb;
		crb.hash = block.hash;
		crb.height = block.height;
		crb.db_id = block.db_id;
		ret.blocks_to_revert.push_back(crb);
	}
	return ret;
}

u64 Blockchain::add_new_block(const SHA256 &hash, const SHA256 &previous_hash, u64 db_id){
	auto cr = this->try_add_new_block(previous_hash);
	if (cr.block_required.has_value())
		throw std::runtime_error("Incorrect usage. Call Blockchain::try_add_new_block() first and add blocks in the correct order.");
	u64 first_reverted = this->blockchain.size();
	if (cr.blocks_to_revert.size())
		first_reverted = cr.blocks_to_revert.front().height;
	for (auto &crb : cr.blocks_to_revert){
		auto block = *this->get_block_by_height(crb.height);
		this->block_map.erase(block.hash);
	}
	Block block;
	block.hash = hash;
	block.previous_hash = previous_hash;
	block.height = first_reverted;
	block.db_id = db_id;
	block.previous_block_id = std::numeric_limits<u64>::max();
	if (block.height > 0)
		block.previous_block_id = this->blockchain[block.height - 1].db_id;
	this->blockchain.resize(block.height + 1);
	this->blockchain[block.height] = block;
	this->block_map[block.hash] = block.height;
	this->update_db();
	return block.height;
}

RevertBlockResult Blockchain::revert_block(const SHA256 &hash){
	if (this->blockchain.size() < 1)
		return RevertBlockResult::EmptyBlockchain;
	if (this->blockchain.back().hash != hash)
		return RevertBlockResult::InvalidRevert;
	auto it = this->block_map.find(hash);
	if (it == this->block_map.end())
		return RevertBlockResult::InvalidRevert;
	this->block_map.erase(it);
	this->blockchain.pop_back();
	this->update_db();
	return RevertBlockResult::Success;
}
