#pragma once

#include <common/types.h>
#include <sqlitepp/sqlitepp.h>
#include <set>

class InsertState{
	sqlite3pp::DB &db;
	sqlite3pp::Statement insert_block_stmt;
	sqlite3pp::Statement insert_tx_stmt;
	sqlite3pp::Statement find_tx;
	sqlite3pp::Statement find_output;
	sqlite3pp::Statement insert_input_stmt;
	sqlite3pp::Statement update_output_stmt;
	sqlite3pp::Statement insert_output_stmt;
	sqlite3pp::Statement select_address_stmt;
	sqlite3pp::Statement insert_address_stmt;
	sqlite3pp::Statement insert_relation1_stmt;
	sqlite3pp::Statement select_output_addresses_stmt;
	sqlite3pp::Statement insert_relation2_stmt;
	u64 next_transaction_id;
public:
	InsertState(sqlite3pp::DB &db);
	u64 insert_block(const std::string &hash, const std::string &prev_hash, u32 timestamp, u32 transaction_count);
	u64 insert_tx(const std::string &hash, const std::string &whash, u32 locktime, u64 block_id, u32 index_in_block, u32 input_count, u32 output_count);
	u64 insert_input(const std::string &previous_tx, u32 txo_index, u64 current_txs_id, u32 txi_index, u64 &previous_output_id);
	u64 insert_output(u64 tx, u32 txo_index, u64 value, u32 required_spenders, const std::vector<u8> &script);
	u64 insert_address_if_it_doesnt_exist(const std::string &address);
	void add_addresses_outputs_relations(u64 output_id, const std::set<u64> &address_ids);
	std::set<u64> get_addresses_for_output(u64 output_id);
	void add_addresses_tx_relations(u64 tx_id, const std::set<u64> &addresses);
};
