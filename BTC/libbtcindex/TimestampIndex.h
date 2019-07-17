#pragma once

#include <common/types.h>
#include <sqlitepp/sqlitepp.h>
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

class TimestampIndex{
	struct TimestampItem{
		u64 tx_begin, tx_end;
		u64 timestamp;
		bool operator<(const TimestampItem &other) const{
			return this->tx_end < other.tx_end;
		}
	};
	std::vector<TimestampItem> data;
public:
	TimestampIndex(sqlite3pp::DB &db);
	TransactionOrder get_timestamp(u64 tx_id) const;
	void reload_data(sqlite3pp::DB &db);
	void add_block(u64 first_transaction_id, u64 transaction_count, u64 timestamp);
};
