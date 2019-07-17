#pragma once

#include "BigintCache.h"
#include <cstdint>
#include <vector>

struct TransactionOrder{
	u64 block_timestamp;
	u32 tx_index;
	bool operator<(const TransactionOrder &other) const{
		if (this->block_timestamp < other.block_timestamp)
			return true;
		if (this->block_timestamp > other.block_timestamp)
			return false;
		return this->tx_index < other.tx_index;
	}
};

struct TimestampItem{
	u64 tx_begin, tx_end;
	u64 timestamp;
	bool operator<(const TimestampItem &other) const{
		return this->tx_end < other.tx_end;
	}
};

class TimestampIndex{
	std::vector<TimestampItem> data;
public:
	TimestampIndex(const TimestampItem *array, size_t n);
	TransactionOrder get_timestamp(u64 tx_id) const;
	void reload_data(const TimestampItem *array, size_t n);
	void add_block(const TimestampItem &);
};
