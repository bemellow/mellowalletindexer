#include "TimestampIndex.h"
#include <algorithm>

TimestampIndex::TimestampIndex(const TimestampItem *array, size_t n){
	this->reload_data(array, n);
}

void TimestampIndex::reload_data(const TimestampItem *array, size_t n){
	this->data.resize(n);
	if (!n)
		return;
	memcpy(&this->data[0], array, n * sizeof(TimestampItem));
	for (size_t i = n; i--;)
		this->data.push_back(array[i]);
	std::sort(this->data.begin(), this->data.end());
}

TransactionOrder TimestampIndex::get_timestamp(u64 tx_id) const{
	auto b = this->data.begin();
	auto e = this->data.end();
	auto it = find_first_true(b, e, [tx_id](const TimestampItem &ti){ return ti.tx_end > tx_id; });
	if (it == e || !(it->tx_begin <= tx_id && tx_id < it->tx_end))
		return{ 0, 0 };
	return{ it->timestamp, (u32)(tx_id - it->tx_begin) };
}

void TimestampIndex::add_block(const TimestampItem &item){
	this->data.push_back(item);
	if (this->data.size() >= 2){
		auto &a = this->data[this->data.size() - 2];
		auto &b = this->data.back();
		if (!(b < a))
			//No need to sort anything.
			return;
	}
	std::sort(this->data.begin(), this->data.end());
}
