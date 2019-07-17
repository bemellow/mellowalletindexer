#pragma once
#ifdef _MSC_VER
#define SPP_USE_SPP_ALLOC 1
#endif
#include "sparsepp/spp.h"
#include <cstring>
#include <cstdint>
#include <iostream>
#include <vector>
#include <utility>
#include <algorithm>
#include <map>

typedef std::uint64_t u64;
typedef std::int64_t s64;
typedef std::uint32_t u32;

struct bigint{
	u32 data[5];
	bool operator==(const bigint &other) const{
		if (this->data[0] != other.data[0]) return false;
		if (this->data[1] != other.data[1]) return false;
		if (this->data[2] != other.data[2]) return false;
		if (this->data[3] != other.data[3]) return false;
		if (this->data[4] != other.data[4]) return false;
		return true;
	}
	bool operator!=(const bigint &other) const{
		return !(*this == other);
	}
	bool operator<(const bigint &other) const{
		if (this->data[4] < other.data[4]) return true;
		if (this->data[4] > other.data[4]) return false;
		if (this->data[3] < other.data[3]) return true;
		if (this->data[3] > other.data[3]) return false;
		if (this->data[2] < other.data[2]) return true;
		if (this->data[2] > other.data[2]) return false;
		if (this->data[1] < other.data[1]) return true;
		if (this->data[1] > other.data[1]) return false;
		if (this->data[0] < other.data[0]) return true;
		return false;
	}
	bool operator>=(const bigint &other) const{
		return !(*this < other);
	}
};

struct bigint_hash{
	u64 operator()(const bigint &bi) const{
		u64 ret = bi.data[0] | ((u64)bi.data[1] << 32);
		ret ^= bi.data[2] | ((u64)bi.data[3] << 32);
		ret ^= bi.data[4] | ((u64)bi.data[4] << 32);
		return ret;
	}
};

struct id_hash{
	u64 operator()(u64 id) const{
		return id;
	}
};

struct bigint_id{
	bigint bi;
	u64 id;
	bool operator<(const bigint_id &other) const{
		return this->bi < other.bi;
	}
};

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

class BigintCache{
#if 0
	typedef MAP<bigint, u64, bigint_hash> map_t;
#else
	typedef std::map<bigint, u64> map_t;
#endif

	bool preloading = true;
	u64 next_id = 0;
	std::vector<bigint_id> vector;
	map_t map;
	std::vector<bigint_id> reverse_map;
	static bool reverse_cmp(const bigint_id &a, const bigint_id &b){
		return a.id < b.id;
	}

	void reduce(){
		if (this->map.size() < 1000000)
			return;
		auto n = this->vector.size();
		this->vector.reserve(n + this->map.size());
		for (auto &kv : this->map){
			bigint_id bid;
			bid.bi = kv.first;
			bid.id = kv.second;
			this->vector.push_back(bid);
		}
		this->map.clear();
		std::sort(this->vector.begin(), this->vector.end());
	}
public:
	BigintCache(size_t preload_size){
		this->vector.reserve(preload_size);
		this->reverse_map.reserve(preload_size * 2);
	}
	u64 encode(const bigint &bi, bool readonly, bool &new_id){
		if (this->preloading){
			new_id = true;
			auto ret = this->next_id++;
			bigint_id bid;
			bid.bi = bi;
			bid.id = ret;
			this->vector.push_back(bid);
			this->reverse_map.emplace_back(bid);
			if (!(this->vector.size() % 1000000))
				std::cout << "Cache size: " << this->vector.size() * 1e-6 << "/" << this->vector.capacity() * 1e-6 << std::endl;
			return ret;
		}

		new_id = false;
		{
			auto it1 = find_first_true(this->vector.begin(), this->vector.end(), [&](const bigint_id &x){ return x.bi >= bi; });
			if (it1 != this->vector.end() && it1->bi == bi)
				return it1->id;
		}
		{
			auto it2 = this->map.find(bi);
			if (it2 != this->map.end())
				return it2->second;
		}
		new_id = true;
		if (readonly)
			return 0;
		auto ret = this->next_id++;
		this->map[bi] = ret;
		{
			bigint_id bid;
			bid.bi = bi;
			bid.id = ret;
			this->reverse_map.emplace_back(bid);
		}
		this->reduce();
		auto n = this->map.size() + this->vector.size();
		if (!(n % 100000))
			std::cout << "Cache size: " << n * 1e-6 << std::endl;
		return ret;
	}
	bigint decode(u64 id, bool &valid) const{
		auto e = this->reverse_map.end();
		auto it = find_first_true(this->reverse_map.begin(), e, [id](const bigint_id &x){ return x.id >= id; });
		if (it == e || it->id != id){
			valid = false;
			return {};
		}
		valid = true;
		return it->bi;
	}
	void finish_preloading(){
		std::cout << "Sorting...\n";
		std::sort(this->vector.begin(), this->vector.end());
		std::sort(this->reverse_map.begin(), this->reverse_map.end(), reverse_cmp);
		this->preloading = false;
	}
};
