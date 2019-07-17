#include "TimestampIndex.h"
#include <common/misc.h>

TimestampIndex::TimestampIndex(sqlite3pp::DB &db){
	this->reload_data(db);
}

void TimestampIndex::reload_data(sqlite3pp::DB &db){
	this->data.clear();
#if 1 //Disabled during debug. Remember to turn back on!
	auto stmt = db << "select first_transaction_id, transaction_count, timestamp from blocks;";
	while (stmt.step() == SQLITE_ROW){
		TimestampItem ti;
		stmt >> ti.tx_begin >> ti.tx_end >> ti.timestamp;
		ti.tx_end += ti.tx_begin;
		this->data.push_back(ti);
	}
	std::sort(this->data.begin(), this->data.end());
#endif
}

TransactionOrder TimestampIndex::get_timestamp(u64 tx_id) const{
	auto b = this->data.begin();
	auto e = this->data.end();
	auto it = find_first_true(b, e, [tx_id](const TimestampItem &ti){ return ti.tx_end > tx_id; });
	if (it == e || !(it->tx_begin <= tx_id && tx_id < it->tx_end))
		return { 0, 0 };
	return { it->timestamp, (u32)(tx_id - it->tx_begin) };
}

void TimestampIndex::add_block(u64 first_transaction_id, u64 transaction_count, u64 timestamp){
	TimestampItem ti;
	ti.tx_begin = first_transaction_id;
	ti.tx_end = first_transaction_id + transaction_count;
	ti.timestamp = timestamp;
	this->data.push_back(ti);
	std::sort(this->data.begin(), this->data.end());
}
