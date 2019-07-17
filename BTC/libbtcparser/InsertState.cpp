#include "InsertState.h"
#include <sstream>
#include <limits>

InsertState::InsertState(sqlite3pp::DB &db)
	: db(db)
	, insert_block_stmt(db << "insert into blocks (hash, previous_hash, timestamp, first_transaction_id, transaction_count) values (?, ?, ?, ?, ?);")
	, insert_tx_stmt(db << "insert into txs (id, hash, whash, locktime, blocks_id, index_in_block, input_count, output_count) values (?, ?, ?, ?, ?, ?, ?, ?);")
	, find_tx(db << "select id from txs where hash = ?;")
	, find_output(db << "select id from outputs where txs_id = ? and txo_index = ?;")
	, insert_input_stmt(db << "insert into inputs (previous_tx_id, txo_index, outputs_id, txs_id, txi_index) values (?, ?, ?, ?, ?);")
	, update_output_stmt(db << "update outputs set spent_by = ? where id = ?;")
	, insert_output_stmt(db << "insert into outputs (txs_id, txo_index, value, required_spenders, script) values (?, ?, ?, ?, ?);")
	, select_address_stmt(db << "select id from addresses where address = ?;")
	, insert_address_stmt(db << "insert into addresses (address) values (?);")
	, select_output_addresses_stmt(db << "select addresses_id from addresses_outputs where outputs_id = ?;")
	, insert_relation1_stmt(db << "insert into addresses_outputs (addresses_id, outputs_id) values (?, ?);")
	, insert_relation2_stmt(db << "insert into addresses_txs (addresses_id, txs_id) values (?, ?);"){
		
	using namespace sqlite3pp;

	u64 count;
	this->db << "select count(*) from (select * from txs limit 1);" << Step() >> count;
	if (!count)
		this->next_transaction_id = 1;
	else{
		this->db << "select max(id) from txs;" << Step() >> this->next_transaction_id;
		this->next_transaction_id++;
	}
}

u64 InsertState::insert_block(const std::string &hash, const std::string &prev_hash, u32 timestamp, u32 transaction_count){
	using namespace sqlite3pp;
	this->insert_block_stmt << Reset() << hash << prev_hash << timestamp << this->next_transaction_id << transaction_count << Step();
	return this->db.last_insert_rowid();
}

u64 InsertState::insert_tx(const std::string &hash, const std::string &whash, u32 locktime, u64 block_id, u32 index_in_block, u32 input_count, u32 output_count){
	using namespace sqlite3pp;
	this->insert_tx_stmt << Reset() << this->next_transaction_id++ << hash;
	if (whash != hash)
		this->insert_tx_stmt << whash;
	else
		this->insert_tx_stmt << Null();
	this->insert_tx_stmt << locktime << block_id << index_in_block << input_count << output_count << Step();
	return this->db.last_insert_rowid();
}

u64 InsertState::insert_input(const std::string &previous_tx, u32 txo_index, u64 current_txs_id, u32 txi_index, u64 &previous_output_id){
	using namespace sqlite3pp;

	bool all_zeroes = true;
	for (auto c : previous_tx)
		if (c != '0')
			all_zeroes = false;

	if (all_zeroes){
		this->insert_input_stmt << Reset() << Null() << Null() << Null() << current_txs_id << txi_index << Step();
		previous_output_id = std::numeric_limits<u64>::max();
		return this->db.last_insert_rowid();
	}

	this->find_tx << Reset() << previous_tx;
	if (this->find_tx.step() != SQLITE_ROW){
		std::stringstream stream;
		stream << "Error while adding input index " << txi_index << ": input references unknown tx " << previous_tx;
		throw std::runtime_error(stream.str());
	}
	u64 tx_id;
	this->find_tx >> tx_id;

	this->find_output << Reset() << tx_id << txo_index;
	if (this->find_output.step() != SQLITE_ROW){
		std::stringstream stream;
		stream << "Error while adding input index " << txi_index << ": input references unknown txo " << previous_tx << ", " << txo_index;
		throw std::runtime_error(stream.str());
	}
	u64 txo_id;
	this->find_output >> txo_id;
	previous_output_id = txo_id;

	this->insert_input_stmt << Reset() << tx_id << txo_index << txo_id << current_txs_id << txi_index << Step();
	u64 ret = this->db.last_insert_rowid();

	this->update_output_stmt << Reset() << ret << txo_id << Step();

	return ret;
}

u64 InsertState::insert_output(u64 tx, u32 txo_index, u64 value, u32 required_spenders, const std::vector<u8> &script){
	using namespace sqlite3pp;
	this->insert_output_stmt << Reset() << tx << txo_index << value << required_spenders << script << Step();
	return this->db.last_insert_rowid();
}

u64 InsertState::insert_address_if_it_doesnt_exist(const std::string &address){
	using namespace sqlite3pp;
	this->select_address_stmt << Reset() << address;
	if (this->select_address_stmt.step() == SQLITE_ROW){
		u64 ret;
		this->select_address_stmt >> ret;
		return ret;
	}
	this->insert_address_stmt << Reset() << address << Step();
	return this->db.last_insert_rowid();
}

void InsertState::add_addresses_outputs_relations(u64 output_id, const std::set<u64> &address_ids){
	using namespace sqlite3pp;
	for (auto &addr : address_ids)
		this->insert_relation1_stmt << Reset() << addr << output_id << Step();
}

std::set<u64> InsertState::get_addresses_for_output(u64 output_id){
	using namespace sqlite3pp;
	this->select_output_addresses_stmt << Reset() << output_id;
	std::set<u64> ret;
	while (this->select_output_addresses_stmt.step() == SQLITE_ROW){
		u64 id;
		this->select_output_addresses_stmt >> id;
		ret.insert(id);
	}
	return ret;
}

void InsertState::add_addresses_tx_relations(u64 tx_id, const std::set<u64> &addresses){
	using namespace sqlite3pp;
	for (auto addr : addresses)
		this->insert_relation2_stmt << Reset() << addr << tx_id << Step();
}
