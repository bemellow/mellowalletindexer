#pragma once

#include "TimestampIndex.h"
#include <libbtcparser/Block.h>
#include <libbtcparser/Blockchain.h>
#include <sqlitepp/sqlitepp.h>
#include <mutex>
#include <thread>
#include <set>
#include <nlohmann/json.hpp>

class TxFetcher{
	using DB = sqlite3pp::DB;
	using Statement = sqlite3pp::Statement;
	DB &db;
	Blockchain &blockchain;
	Statement get_tx_by_id;
	Statement get_inputs_by_tx;
	Statement get_outputs_by_tx;
	Statement get_addresses_by_output;
	nlohmann::json get_inputs(u64 txid, double &memory_limit, size_t reserve = 0);
	nlohmann::json get_outputs(u64 txid, double &memory_limit, size_t reserve = 0);
	std::vector<std::string> get_addresses_for_output(u64 output_id);
public:
	TxFetcher(DB &, Blockchain &);
	TxFetcher(const TxFetcher &) = delete;
	TxFetcher(TxFetcher &&) = delete;
	const TxFetcher &operator=(const TxFetcher &) = delete;
	const TxFetcher &operator=(TxFetcher &&) = delete;

	nlohmann::json get_tx(u64 id, double &memory_limit);
};

class Indexer{
	using DB = sqlite3pp::DB;
	using Statement = sqlite3pp::Statement;
	bool testnet;
	std::string db_path;
	DB db;
	InsertState is;
	Statement get_address_id_stmt;
	Statement read_outputs_stmt;
	Statement read_txs_stmt;
	Statement read_utxo_stmt;
	Statement get_tx_hash;
	Statement read_utxo_value_stmt;
	Statement get_block_transactions;
	Statement get_block_transactions_and_timestamp;
	Statement get_deleted_outputs;
	Statement delete_outputs_from_tx;
	Statement delete_outputs_relations;
	Statement get_spent_outputs_from_tx;
	Statement delete_inputs_from_tx;
	Statement unspend_output;
	Statement delete_tx_relations;
	Statement delete_txs_from_block;
	Statement delete_block;
	Statement get_cached_balance_stmt;
	Statement set_cached_balance_stmt;
	Statement delete_cached_balance_stmt;
	Statement get_outputs_total_for_block_stmt;
	Statement get_inputs_total_for_block_stmt;
	Statement set_fee_for_block_stmt;
	Statement get_average_fee_for_block_stmt;
	std::map<std::thread::id, std::string> returned_strings;
	TimestampIndex timestamp_index;
	Blockchain blockchain;
	TxFetcher tx_fetcher;
	mutable std::recursive_mutex mutex;

	boost::optional<u64> low_fee, normal_fee, high_fee;

	using SHA256 = Hashes::Digests::SHA256;

	struct BlockchainElement{
		bool is_new;
		u64 id;
		SHA256 hash;
		u64 height;
	};

	struct NewBlock{
		u64 block_id;
		u64 first_transaction_id;
		u64 transaction_count;
		u64 timestamp;
	};

	struct Utxo{
		u64 tx_id;
		u32 output_index;
		u64 value;
		u32 required_spenders;
		bool operator<(const Utxo &other) const{
			if (this->tx_id < other.tx_id)
				return true;
			if (this->tx_id > other.tx_id)
				return false;
			return this->output_index < other.output_index;
		}
	};

	std::set<u64> read_outputs(u64 address_id);
	std::vector<u64> read_txs(u64 address);
	nlohmann::json read_utxo(u64 id);
	NewBlock insert_new_block(Block &block, const std::vector<ChainReorganizationBlock> &, std::set<u64> &updated_balances);
	const char *return_string(std::string &&);
	void revert_block(u64 id);
	void revert_tx(u64 id, std::vector<u64> &ids);
	std::map<std::string, std::set<Utxo>> get_utxo_internal(const char *addresses_string);
	template <typename F>
	void enumerate_utxo_internal_single_address(u64 id, const F &f){
		auto outputs = this->read_outputs(id);
		for (auto outputs_id : outputs){
			this->read_utxo_stmt << sqlite3pp::Reset() << outputs_id;
			if (this->read_utxo_stmt.step() != SQLITE_ROW)
				continue;
			Utxo utxo;
			this->read_utxo_stmt >> utxo.tx_id >> utxo.output_index >> utxo.value >> utxo.required_spenders;
			f(std::move(utxo));
		}
	}
	std::set<Utxo> get_utxo_internal_single_address(u64 id);
	u64 get_balance(u64 id);
	boost::optional<u64> get_cached_balance(u64 id);
	void set_cached_balance(u64 id, u64 balance);
	void invalidate_balance_cache(u64 id);
	u64 get_outputs_total_for_block(u64 block_id);
	u64 get_inputs_total_for_block(u64 block_id);
	void set_fee_for_block(u64 block_id, u64 tx_size);
	boost::optional<u64> get_average_fee_for_block(u64 block_id);
public:
	Indexer(const char *db_path, bool testnet);
	const char *get_utxo(const char *addresses);
	const char *get_utxo_insight(const char *addresses);
	const char *get_balance(const char *addresses);
	const char *get_balances(const char *addresses);
	const char *get_history(const char *addresses);
	const char *get_fees();
	u64 get_blockchain_height() const;
	const char *push_new_block(const void *data, size_t size);
};
