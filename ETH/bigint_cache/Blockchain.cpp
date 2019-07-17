#include "Blockchain.h"
#include <sstream>
#include <iostream>

const auto max64 = std::numeric_limits<u64>::max();

struct BlocksTemp{
	std::vector<HeadCandidate *> blocks;
	std::vector<HeadCandidate> block_map;
};

bool HeadCandidate_cmp(const HeadCandidate &a, const HeadCandidate &b){
	return a.hash < b.hash;
}

template<class It, class F>
It find_first_true(It begin, It end, const F &f){
	if (begin >= end)
		return end;
	if (f(*begin))
		return begin;
	auto diff = end - begin;
	while (diff > 1){
		auto pivot = begin + diff / 2;
		if (!f(*pivot))
			begin = pivot;
		else
			end = pivot;
		diff = end - begin;
	}
	return end;
}

BlocksTemp build_blocks(get_block_data_callback c1){
	BlocksTemp ret;
	while (true){
		u64 id;
		SHA256 hash, previous_hash;
		if (!c1(&id, &hash, &previous_hash))
			break;

		ret.block_map.push_back({id, 0, hash, previous_hash, nullptr, false});
	}

	std::sort(ret.block_map.begin(), ret.block_map.end(), HeadCandidate_cmp);
	ret.blocks.reserve(ret.block_map.size());
	for (auto &block : ret.block_map)
		ret.blocks.push_back(&block);

	auto b = ret.block_map.begin(),
		e = ret.block_map.end();

	for (auto &block : ret.blocks){
		auto previous_hash = block->previous_hash;
		auto it = find_first_true(b, e, [&previous_hash](const HeadCandidate &hc){ return hc.hash >= previous_hash; });
		if (it == e || it->hash != previous_hash)
			continue;
		block->previous = &*it;
		block->previous->has_any_children = true;
	}
	return ret;
}

const HeadCandidate *select_head(get_blockchain_head_callback c2, const BlocksTemp &blocks, const Blockchain::head_selector_t &head_selector){
	std::vector<const HeadCandidate *> heads;
	for (auto &block : blocks.blocks)
		if (!block->has_any_children)
			heads.push_back(block);
	if (!heads.size())
		throw std::runtime_error("Invalid blockchain. Circular topologies are invalid.");
	std::sort(heads.begin(), heads.end(), [](const HeadCandidate *a, const HeadCandidate *b){ return a->id < b->id; });
	const HeadCandidate *ret;
	if (heads.size() != 1){
		boost::optional<SHA256> hash;
		{
			SHA256 h;
			if (c2(&h))
				hash = h;
		}
		if (!hash.has_value()){
			if (!head_selector){
				for (auto &head : heads)
					std::cerr << "HEAD: " << head->id << ", " << (std::string)head->hash << std::endl;
				throw std::runtime_error("Can't read blockchain head from DB.");
			}
			hash = heads[head_selector(heads)]->hash;
		}
		
		auto b = blocks.block_map.begin(),
			e = blocks.block_map.end();
		auto previous_hash = *hash;
		auto it = find_first_true(b, e, [&previous_hash](const HeadCandidate &hc){ return hc.hash >= previous_hash; });
		if (it == e || it->hash != previous_hash)
			throw std::runtime_error("DB states that the blockchain head is " + (std::string)*hash + ", but no such block exists in the DB.");
		ret = &*it;
	}else
		ret = heads.front();
	return ret;
}

std::vector<HeadCandidate> assemble_blockchain(get_block_data_callback c1, get_blockchain_head_callback c2, const Blockchain::head_selector_t &head_selector){
	std::vector<HeadCandidate> ret;
	auto blocks = build_blocks(c1);
	if (!blocks.blocks.size())
		return ret;
	ret.reserve(blocks.blocks.size());
	for (auto current = select_head(c2, blocks, head_selector); current; current = current->previous){
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

Blockchain::Blockchain(get_block_data_callback c1, get_blockchain_head_callback c2, const head_selector_t &head_selector){
	auto blocks = assemble_blockchain(c1, c2, head_selector);
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
#if 0
	//Sanity check
	for (size_t i = 0, n = this->blockchain.size(); i < n; i++){
		auto &block = this->blockchain[i];
		auto it = this->block_map.find(block.hash);
		if (it == this->block_map.end())
			__debugbreak(); //Sanity check failed.
		if (it->second != i)
			__debugbreak(); //Sanity check failed.
	}
#endif
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
		return max64;
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
	return block.height;
}

Blockchain::Block::operator std::string() const{
	std::stringstream stream;
	stream
		<< (std::string)this->hash << ' '
		<< (std::string)this->previous_hash << ' '
		<< this->height << ' '
		<< this->db_id << ' ';
	if (this->previous_block_id == std::numeric_limits<u64>::max())
		stream << "-1";
	else
		stream << this->previous_block_id;
	return stream.str();
}

ChainReorganization::operator std::string() const{
	std::stringstream stream;
	stream << (int)this->block_required.has_value() << ' ';
	if (this->block_required.has_value())
		stream << (std::string)*this->block_required << ' ';
	stream << this->blocks_to_revert.size();
	for (auto &block : this->blocks_to_revert)
		stream
			<< ' ' << (std::string)block.hash
			<< ' ' << block.db_id
			<< ' ' << block.height;
	return stream.str();
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
	return RevertBlockResult::Success;
}
