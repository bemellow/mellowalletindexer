#include "Indexer.h"
#include <common/serialization.h>

//In main.cpp there's a call to sqlite3_config() that you should uncomment if you implement
//reader-writer locks!
#define LOCK_READER LOCK_MUTEX(this->mutex)
#define LOCK_WRITER LOCK_MUTEX(this->mutex)

using namespace sqlite3pp;

template <typename T>
void set_union(std::set<T> &dst, const std::set<T> &src){
	dst.insert(src.begin(), src.end());
}

static void get_id_list(std::vector<u64> &dst, Statement &stmt, u64 input){
	dst.clear();
	stmt << Reset() << input;
	while (stmt.step() == SQLITE_ROW){
		u64 id;
		stmt >> id;
		dst.push_back(id);
	}
}

static void get_id_list(std::set<u64> &dst, Statement &stmt, u64 input){
	dst.clear();
	stmt << Reset() << input;
	while (stmt.step() == SQLITE_ROW){
		u64 id;
		stmt >> id;
		dst.insert(id);
	}
}

static void use_id_list(Statement &stmt, std::vector<u64> &ids){
	for (auto id : ids)
		stmt << Reset() << id << Step();
}

Indexer::Indexer(const char *db_path, bool testnet)
	: testnet(testnet)
	, db_path(db_path)
	, db(this->db_path.c_str())
	, is(this->db)
	, get_address_id_stmt(this->db << "select id from addresses where address = ?;")
	, read_outputs_stmt(this->db << "select outputs_id from addresses_outputs where addresses_id = ?;")
	, read_txs_stmt(this->db << "select txs_id from addresses_txs where addresses_id = ?;")
	, read_utxo_stmt(this->db << "select txs_id, txo_index, value, required_spenders from outputs where outputs.id = ? and outputs.spent_by is null;")
	, get_tx_hash(this->db << "select hash from txs where id = ?;")
	, get_block_transactions(this->db << "select first_transaction_id, transaction_count from blocks where id = ?;")
	, get_block_transactions_and_timestamp(this->db << "select first_transaction_id, transaction_count, timestamp from blocks where id = ?;")
	, get_deleted_outputs(this->db << "select id from outputs where txs_id = ?;")
	, delete_outputs_from_tx(this->db << "delete from outputs where txs_id = ?;")
	, delete_outputs_relations(this->db << "delete from addresses_outputs where outputs_id = ?;")
	, get_spent_outputs_from_tx(this->db << "select outputs_id from inputs where txs_id = ?;")
	, delete_inputs_from_tx(this->db << "delete from inputs where txs_id = ?;")
	, unspend_output(this->db << "update outputs set spent_by = null where id = ?;")
	, delete_tx_relations(this->db << "delete from addresses_txs where txs_id = ?;")
	, delete_txs_from_block(this->db << "delete from txs where blocks_id = ?;")
	, delete_block(this->db << "delete from blocks where id = ?;")
	, get_cached_balance_stmt(this->db << "select balance from cached_balances where id = ?;")
	, set_cached_balance_stmt(this->db << "update cached_balances set balance = ? where id = ?;")
	, delete_cached_balance_stmt(this->db << "delete from cached_balances where id = ?;")
	, get_outputs_total_for_block_stmt(this->db << "select sum(outputs.value) from txs inner join outputs on outputs.txs_id = txs.id where txs.blocks_id = ? and txs.index_in_block > 0;")
	, get_inputs_total_for_block_stmt(this->db << "select sum(outputs.value) from txs inner join inputs on inputs.txs_id = txs.id inner join outputs on outputs.id = inputs.outputs_id where txs.blocks_id = ? and txs.index_in_block > 0;")
	, set_fee_for_block_stmt(this->db << "insert into block_fees (id, average_fee_per_kb) values (?1, ?2) on conflict (id) do update set average_fee_per_kb = ?2 where id = ?1;")
	, get_average_fee_for_block_stmt(this-> db << "select average_fee_per_kb from block_fees where id = ?;")
	, timestamp_index(this->db)
	, blockchain(this->db)
	, tx_fetcher(this->db, this->blockchain)
{}

std::set<u64> Indexer::read_outputs(u64 address_id){
	std::set<u64> ret;
	get_id_list(ret, this->read_outputs_stmt, address_id);
	return ret;
}

std::vector<u64> Indexer::read_txs(u64 address){
	this->read_txs_stmt << Reset() << address;
	std::vector<u64> ret;
	ret.reserve(1024);
	while (this->read_txs_stmt.step() == SQLITE_ROW){
		u64 txid;
		this->read_txs_stmt >> txid;
		ret.push_back(txid);
	}
	std::sort(ret.begin(), ret.end());
	return ret;
}

nlohmann::json Indexer::read_utxo(u64 id){
	nlohmann::json ret = nlohmann::json::object_t();
	this->read_utxo_stmt << Reset() << id;
	if (this->read_utxo_stmt.step() == SQLITE_ROW){
		u64 value;
		std::string txid;
		u64 output_index;
		u64 min_sigs;
		this->read_utxo_stmt >> value >> txid >> output_index >> min_sigs;
		ret["value"] = std::to_string(value);
		ret["txid"] = txid;
		ret["output_index"] = output_index;
		ret["min_sigs"] = min_sigs;
	}
	return ret;
}

template <typename T>
struct ptrcmp{
	bool operator()(T *a, T *b) const{
		return *a < *b;
	}
};

template <typename T>
boost::optional<u64> map_address(Statement &stmt, const T &x){
	stmt << Reset() << x;
	if (stmt.step() != SQLITE_ROW)
		return {};
	u64 ret;
	stmt >> ret;
	return ret;
}

template <typename T>
std::map<std::string, u64> map_addresses(Statement &stmt, const T &xs){
	std::map<std::string, u64> ret;
	for (auto &x : xs){
		auto s = x.template get<std::string>();
		auto id = map_address(stmt, s);
		if (!id)
			continue;
		ret[s] = *id;
	}
	return ret;
}

std::set<Indexer::Utxo> Indexer::get_utxo_internal_single_address(u64 id){
	std::set<Utxo> set;
	this->enumerate_utxo_internal_single_address(id, [&set](const Utxo &utxo){ set.insert(utxo); });
	return set;
}

std::map<std::string, std::set<Indexer::Utxo>> Indexer::get_utxo_internal(const char *addresses_string){
	auto addresses = map_addresses(this->get_address_id_stmt, nlohmann::json::parse(addresses_string));

	std::map<std::string, std::set<u64>> outputs_by_address;
	for (auto &address : addresses)
		outputs_by_address[address.first] = this->read_outputs(address.second);
	
	std::map<std::string, std::set<Utxo>> ret;
	for (auto &kv : outputs_by_address){
		std::set<Utxo> set;
		for (auto outputs_id : kv.second){
			this->read_utxo_stmt << Reset() << outputs_id;
			if (this->read_utxo_stmt.step() != SQLITE_ROW)
				continue;
			Utxo utxo;
			this->read_utxo_stmt >> utxo.tx_id >> utxo.output_index >> utxo.value >> utxo.required_spenders;
			set.insert(utxo);
		}
		ret[kv.first] = std::move(set);
	}
	return ret;
}

const char *Indexer::get_utxo(const char *addresses){
	LOCK_READER;
	auto utxos_by_address = this->get_utxo_internal(addresses);
	
	nlohmann::json ret = nlohmann::json::object_t();
	for (auto &kv : utxos_by_address){
		auto addr = nlohmann::json::array();
		for (auto &utxo : kv.second){
			this->get_tx_hash << Reset() << utxo.tx_id;
			if (this->get_tx_hash.step() != SQLITE_ROW){
				//The DB is probably corrupted if we reach here.
				assert(false);
				continue;
			}
			std::string hash;
			this->get_tx_hash >> hash;
			nlohmann::json utxo_json;
			utxo_json["value"] = std::to_string(utxo.value);
			utxo_json["txid"] = hash;
			utxo_json["output_index"] = utxo.output_index;
			utxo_json["min_sigs"] = utxo.required_spenders;
			addr.emplace_back(std::move(utxo_json));
		}
		ret[kv.first] = std::move(addr);
	}
	return this->return_string(ret.dump());
}

const char *Indexer::get_utxo_insight(const char *addresses){
	LOCK_READER;
	auto utxos_by_address = this->get_utxo_internal(addresses);
	
	auto ret = nlohmann::json::array();
	for (auto &kv : utxos_by_address){
		for (auto &utxo : kv.second){
			if (utxo.required_spenders > 1)
				continue;
			this->get_tx_hash << Reset() << utxo.tx_id;
			if (this->get_tx_hash.step() != SQLITE_ROW){
				//The DB is probably corrupted if we reach here.
				assert(false);
				continue;
			}
			std::string hash;
			this->get_tx_hash >> hash;
			nlohmann::json utxo_json;
			utxo_json["address"] = kv.first;
			utxo_json["satoshis"] = std::to_string(utxo.value);
			utxo_json["txid"] = hash;
			utxo_json["vout"] = utxo.output_index;
			ret.emplace_back(std::move(utxo_json));
		}
	}
	return this->return_string(ret.dump());
}

u64 Indexer::get_balance(u64 id){
	auto balance = this->get_cached_balance(id);
	if (balance)
		return *balance;

	u64 ret = 0;
	this->enumerate_utxo_internal_single_address(
		id,
		[&ret](const Utxo &utxo){ ret += utxo.value; }
	);
	this->set_cached_balance(id, ret);
	return ret;
}

const char *Indexer::get_balance(const char *addresses){
	LOCK_READER;
	auto mapped = map_addresses(this->get_address_id_stmt, nlohmann::json::parse(addresses));
	u64 sum = 0;
	for (auto &kv : mapped)
		sum += this->get_balance(kv.second);
	std::string ret(1, '"');
	ret += std::to_string(sum);
	ret += '\"';
	return this->return_string(std::move(ret));
}

const char *Indexer::get_balances(const char *addresses){
	LOCK_READER;
	auto mapped = map_addresses(this->get_address_id_stmt, nlohmann::json::parse(addresses));
	nlohmann::json ret = nlohmann::json::object_t();
	for (auto &kv : mapped)
		ret[kv.first] = std::to_string(this->get_balance(kv.second));
	return this->return_string(ret.dump());
}

template <typename T>
void check_limit(double &l, T c){
	l -= (double)c;
	if (l < 0)
		throw std::runtime_error("Request exceeds memory limit.");
}

const char *Indexer::get_history(const char *params_string){
	LOCK_READER;
	auto params = nlohmann::json::parse(params_string);
	auto addresses = map_addresses(this->get_address_id_stmt, params["addresses"]);
	auto max_txs = params["max_txs"].get<u64>();

	std::vector<u64> txs;
	for (auto &kv : addresses)
		txs = set_union(txs, this->read_txs(kv.second));
	
	std::vector<std::pair<u64, TransactionOrder>> txs_timestamps;
	txs_timestamps.reserve(txs.size());
	for (auto id : txs)
		txs_timestamps.emplace_back(id, this->timestamp_index.get_timestamp(id));

	std::sort(txs_timestamps.begin(), txs_timestamps.end(), [](auto &a, auto &b){ return b.second < a.second; });

	if (txs_timestamps.size() > max_txs)
		txs_timestamps.resize((size_t)max_txs);

	double memory_limit = 1024 * 1024 * 1024; // 1 GiB
	auto ret = nlohmann::json::array();
	for (auto &tx : txs_timestamps){
		check_limit(memory_limit, (9 + 8 * 2) + 8);
		auto json_tx = this->tx_fetcher.get_tx(tx.first, memory_limit);
		json_tx["timestamp"] = tx.second.block_timestamp;
		ret.emplace_back(std::move(json_tx));
	}

	return this->return_string(ret.dump());
}

u64 Indexer::get_blockchain_height() const{
	LOCK_READER;
	return this->blockchain.get_height();
}

u64 Indexer::get_outputs_total_for_block(u64 block_id){
	u64 ret;
	this->get_outputs_total_for_block_stmt << Reset() << block_id << Step() >> ret;
	return ret;
}

u64 Indexer::get_inputs_total_for_block(u64 block_id){
	u64 ret;
	this->get_inputs_total_for_block_stmt << Reset() << block_id << Step() >> ret;
	return ret;
}

void Indexer::set_fee_for_block(u64 block_id, u64 tx_size){
	//Yes, we do need to go to the DB to get the total fees. No, we can't just use the information
	//we parsed from the block.
	auto total_fees = this->get_inputs_total_for_block(block_id) - this->get_outputs_total_for_block(block_id);
	auto average = (total_fees && tx_size) ? (total_fees / tx_size) : 0;
	this->set_fee_for_block_stmt << Reset() << block_id << average << Step();
}

Indexer::NewBlock Indexer::insert_new_block(Block &block, const std::vector<ChainReorganizationBlock> &blocks_to_revert, std::set<u64> &updated_balances){
	sqlite3pp::Transaction transaction(this->db);
	for (size_t i = blocks_to_revert.size(); i--;){
		this->revert_block(blocks_to_revert[i].db_id);
		auto result = this->blockchain.revert_block(blocks_to_revert[i].hash);
		switch (result){
			case RevertBlockResult::Success:
				break;
			case RevertBlockResult::EmptyBlockchain:
				throw std::runtime_error("Attempting to revert a block on an empty blockchain (?!).");
			case RevertBlockResult::InvalidRevert:
				throw std::runtime_error("Attempting to revert a block other than the head.");
		}
	}
	NewBlock ret;
	ret.block_id = block.insert(this->is, updated_balances);
	this->set_fee_for_block(ret.block_id, block.get_average_transaction_size());
	this->get_block_transactions_and_timestamp << Reset() << ret.block_id << Step()
		>> ret.first_transaction_id
		>> ret.transaction_count
		>> ret.timestamp;
	return ret;
}

const char *Indexer::push_new_block(const void *data, size_t size){
	SerializedBuffer buffer(data, size);
	Block block(buffer, this->testnet, BlockFromRpc());
	LOCK_WRITER;
	auto reorg = this->blockchain.try_add_new_block(block.get_previous_hash());
	nlohmann::json ret = nlohmann::json::object_t();
	if (reorg.block_required.has_value()){
		ret["block_required"] = (std::string)*reorg.block_required;
	}else{
		std::set<u64> updated_balances;
		auto new_block = this->insert_new_block(block, reorg.blocks_to_revert, updated_balances);
		auto new_height = this->blockchain.add_new_block(block.get_hash(), block.get_previous_hash(), new_block.block_id);
		if (reorg.blocks_to_revert.size()){
			this->db.exec("delete from cached_balances;");
			this->timestamp_index.reload_data(this->db);
		}else{
			this->timestamp_index.add_block(new_block.first_transaction_id, new_block.transaction_count, new_block.timestamp);
			for (auto id : updated_balances)
				this->invalidate_balance_cache(id);
		}

		if (reorg.blocks_to_revert.size())
			ret["blocks_reverted"] = reorg.blocks_to_revert.size();
		ret["db_id"] = new_block.block_id;
		ret["new_height"] = new_height;
	}
	this->low_fee.reset();
	this->normal_fee.reset();
	this->high_fee.reset();
	return this->return_string(ret.dump());
}

void Indexer::revert_block(u64 block_id){
	u64 first_transaction_id, transaction_count;
	this->get_block_transactions << Reset() << block_id << Step() >> first_transaction_id >> transaction_count;
	std::vector<u64> ids;
	for (auto txid = first_transaction_id + transaction_count; txid-- > first_transaction_id;)
		this->revert_tx(txid, ids);
	this->delete_txs_from_block << Reset() << block_id << Step();
	this->delete_block << Reset() << block_id << Step();
}

void Indexer::revert_tx(u64 txid, std::vector<u64> &ids){
	get_id_list(ids, this->get_deleted_outputs, txid);
	this->delete_outputs_from_tx << Reset() << txid << Step();
	use_id_list(this->delete_outputs_relations, ids);

	get_id_list(ids, this->get_spent_outputs_from_tx, txid);
	this->delete_inputs_from_tx << Reset() << txid << Step();
	use_id_list(this->unspend_output, ids);

	this->delete_tx_relations << Reset() << txid << Step();
}

const char *Indexer::return_string(std::string &&s){
	LOCK_WRITER;
	auto tid = std::this_thread::get_id();
	auto &s2 = this->returned_strings[tid] = std::move(s);
	return s2.c_str();
}

boost::optional<u64> Indexer::get_cached_balance(u64 id){
	auto &stmt = this->get_cached_balance_stmt;
	stmt << Reset() << id;
	boost::optional<u64> ret;
	if (stmt.step() == SQLITE_ROW)
		stmt >> ret;
	return ret;
}

void Indexer::set_cached_balance(u64 id, u64 balance){
	this->set_cached_balance_stmt << Reset() << balance << id << Step();
}

void Indexer::invalidate_balance_cache(u64 id){
	this->delete_cached_balance_stmt << Reset() << id << Step();
}

const char *Indexer::get_fees(){
	LOCK_READER;
	nlohmann::json ret;
	if (!this->low_fee.has_value()){
		u64 first = 0;
		auto height = this->blockchain.get_height();
		if (height > 50)
			first = height - 50;
		
		u64 sum = 0;
		u64 count = 0;
		for (u64 i = first; i <= height; i++){
			auto block = this->blockchain.get_block_by_height(i);
			if (!block)
				continue;
			auto id = block->db_id;
			auto fee = this->get_average_fee_for_block(id);
			if (!fee)
				continue;
			sum += *fee;
			count++;
		}

		u64 normal = count ? sum / count : 0;
		this->low_fee = normal * 8 / 10;
		this->normal_fee = normal;
		this->high_fee = normal * 12 / 10;
	}
	ret["low"] = std::to_string(*this->low_fee);
	ret["normal"] = std::to_string(*this->normal_fee);
	ret["high"] = std::to_string(*this->high_fee);
	return this->return_string(ret.dump());
}

boost::optional<u64> Indexer::get_average_fee_for_block(u64 block_id){
	auto &stmt = this->get_average_fee_for_block_stmt;
	stmt << Reset() << block_id;
	if (stmt.step() != SQLITE_ROW)
		return {};
	u64 ret;
	stmt >> ret;
	return ret;
}

TxFetcher::TxFetcher(DB &db, Blockchain &blockchain)
	: db(db)
	, blockchain(blockchain)
	, get_tx_by_id(db << "select txs.hash, txs.whash, txs.locktime, blocks.hash, txs.index_in_block, txs.input_count, txs.output_count from txs inner join blocks on blocks.id = txs.blocks_id where txs.id = ?;")
	, get_inputs_by_tx(db << "select inputs.outputs_id, inputs.txi_index, outputs.value from inputs inner join outputs on outputs.id = inputs.outputs_id where inputs.txs_id = ?;")
	, get_outputs_by_tx(db << "select id, txo_index, value, required_spenders, spent_by from outputs where txs_id = ?;")
	, get_addresses_by_output(db << "select addresses.address from addresses inner join addresses_outputs on addresses_outputs.addresses_id = addresses.id where addresses_outputs.outputs_id = ?;")
{}

nlohmann::json TxFetcher::get_tx(u64 id, double &memory_limit){
	auto &stmt = this->get_tx_by_id;
	stmt << Reset() << id;
	nlohmann::json ret;
	if (stmt.step() != SQLITE_ROW)
		return ret;

	std::string hash, whash;
	u32 locktime;
	std::string block_hash;
	u32 block_index;
	size_t input_count;
	size_t output_count;
	stmt >> hash >> whash >> locktime >> block_hash >> block_index >> input_count >> output_count;

	auto block = this->blockchain.get_block_by_hash(block_hash);
	if (!block){
		std::stringstream stream;
		stream
			<< "Internal error (implementation bug?): TX " << hash << " (ID " << id
			<< ") reports that it belongs to block " << block_hash
			<< ", but this block is not part of the blockchain.";
		throw std::runtime_error(stream.str());
	}

	check_limit(memory_limit,
		(( 4 + 8 * 2) + (64 + 8 * 2)) +
		(( 5 + 8 * 2) + (64 + 8 * 2)) +
		(( 8 + 8 * 2) + 4) +
		((10 + 8 * 2) + (64 + 8 * 2)) +
		((12 + 8 * 2) + 8) +
		((11 + 8 * 2) + 4)
	);
	ret["hash"] = hash;
	ret["whash"] = whash;
	ret["locktime"] = locktime;
	ret["block_hash"] = block_hash;
	ret["block_height"] =  block->height;
	ret["block_index"] = block_index;

	check_limit(memory_limit, 6 + 8 * 2);
	ret["inputs"] = this->get_inputs(id, memory_limit);
	check_limit(memory_limit, 7 + 8 * 2);
	ret["outputs"] = this->get_outputs(id, memory_limit);

	return ret;
}

nlohmann::json TxFetcher::get_inputs(u64 txid, double &memory_limit, size_t reserve){
	struct Input{
		u64 output_id;
		u32 txi_index;
		u64 value;
		std::vector<std::string> addresses;
		nlohmann::json to_json(double &memory_limit) const{
			check_limit(memory_limit,
				((9 + 8 * 2) + 4) +
				((5 + 8 * 2) + (19 + 8 * 2)) +
				((9 + 8 * 2) + this->addresses.size() * (64 + 8 * 2))
			);

			nlohmann::json ret;
			ret["txi_index"] = this->txi_index;
			ret["value"] = std::to_string(this->value);
			ret["addresses"] = this->addresses;
			return ret;
		}
	};

	std::vector<Input> inputs;
	inputs.reserve(reserve);
	{
		auto &stmt = this->get_inputs_by_tx;
		stmt << Reset() << txid;
		while (stmt.step() == SQLITE_ROW){
			Input input;
			stmt >> input.output_id >> input.txi_index >> input.value;
			inputs.push_back(input);
		}
	}
	auto ret = nlohmann::json::array();
	for (auto &input : inputs){
		input.addresses = this->get_addresses_for_output(input.output_id);
		ret.emplace_back(input.to_json(memory_limit));
	}
	return ret;
}

nlohmann::json TxFetcher::get_outputs(u64 txid, double &memory_limit, size_t reserve){
	struct Output{
		u64 id;
		u32 txo_index;
		u64 value;
		u32 required_spenders;
		boost::optional<u64> spent_by;
		std::vector<std::string> addresses;
		nlohmann::json to_json(double &memory_limit) const{
			check_limit(memory_limit,
				(( 9 + 8 * 2) + 4) +
				(( 5 + 8 * 2) + (19 + 8 * 2)) +
				((17 + 8 * 2) + 4) +
				(( 8 + 8 * 2) + (19 + 8 * 2)) +
				(( 9 + 8 * 2) + this->addresses.size() * (64 + 8 * 2))
			);

			nlohmann::json ret;
			ret["txo_index"] = this->txo_index;
			ret["value"] = std::to_string(this->value);
			ret["required_spenders"] = this->required_spenders;
			if (this->spent_by.has_value())
				ret["spent_by"] = std::to_string(*this->spent_by);
			else
				ret["spent_by"] = {};
			ret["addresses"] = this->addresses;
			return ret;
		}
	};

	std::vector<Output> outputs;
	outputs.reserve(reserve);
	{
		auto &stmt = this->get_outputs_by_tx;
		stmt << Reset() << txid;
		while (stmt.step() == SQLITE_ROW){
			Output output;
			stmt >> output.id >> output.txo_index >> output.value >> output.required_spenders >> output.spent_by;
			outputs.push_back(output);
		}
	}
	auto ret = nlohmann::json::array();
	for (auto &output : outputs){
		output.addresses = this->get_addresses_for_output(output.id);
		ret.emplace_back(output.to_json(memory_limit));
	}
	return ret;
}

std::vector<std::string> TxFetcher::get_addresses_for_output(u64 output_id){
	std::vector<std::string> ret;
	auto &stmt = this->get_addresses_by_output;
	stmt << Reset() << output_id;
	ret.reserve(32);
	while (stmt.step() == SQLITE_ROW){
		std::string address;
		stmt >> address;
		ret.emplace_back(std::move(address));
	}
	return ret;
}
