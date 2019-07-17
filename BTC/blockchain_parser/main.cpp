#include "Paths.h"
#include "add_all_blocks.h"
#include <libbtcparser/Block.h>
#include <libbtcparser/Blockchain.h>
#include <sqlitepp/sqlitepp.h>
#include <common/serialization.h>
#include <csignal>
#include <fstream>
#include <boost/filesystem.hpp>
#include "ProgressDisplay.h"

void find_longest_chain(const Paths &paths);

void initialize_db(const Paths &paths){
	using namespace sqlite3pp;
	static const char * const commands[] = {
		"create table blocks(\n"
		"    id integer primary key,\n"
		"    hash text collate nocase,\n"
		"    previous_hash text collate nocase,\n"
		"    previous_blocks_id integer,\n"
		"    timestamp integer,\n"
		"    first_transaction_id integer,\n"
		"    transaction_count integer,\n"
		"    file_name text,\n"
		"    file_offset integer,\n"
		"    size_in_file integer\n"
		");",

		"create index blocks_by_hash on blocks (hash);",

		"create table txs(\n"
		"    id integer primary key,\n"
		"    hash text collate nocase,\n"
		"    whash text collate nocase,\n"
		"    locktime integer,\n"
		"    blocks_id integer,\n"
		"    index_in_block integer,\n"
		"    input_count integer,\n"
		"    output_count integer\n"
		");",

		"create index txs_by_hash on txs (hash);",
		"create index txs_by_blocks_id on txs (blocks_id);",

		"create table inputs(\n"
		"    id integer primary key,\n"
		"    previous_tx_id integer,\n"
		"    txo_index integer,\n"
		"    outputs_id integer,\n"
		"    txs_id integer,\n"
		"    txi_index integer\n"
		");",

		//"create index inputs_by_previous_tx_id on inputs(previous_tx_id);",
		//"create index inputs_by_txs_id on inputs(txs_id);",
		//"create index inputs_by_outputs_id on inputs(outputs_id);",

		"create table outputs(\n"
		"    id integer primary key,\n"
		"    txs_id integer,\n"
		"    txo_index integer,\n"
		"    value integer,\n"
		"    required_spenders integer,\n"
		"    script blob,\n"
		"    spent_by integer\n"
		");",

		"create index outputs_by_txs_id on outputs (txs_id);",
		"create index outputs_by_txs_id_txo_index on outputs (txs_id, txo_index);",

		"create table addresses (\n"
		"    id integer primary key,\n"
		"    address text,\n"
		"    cached_balance integer\n"
		/*"    address text,\n"
		"    txs_count integer,\n"
		"    txs blob\n"*/
		");",

		"create index addresses_by_address on addresses (address);",

		"create table addresses_outputs (\n"
		"    addresses_id integer,\n"
		"    outputs_id integer\n"
		");",

		//"create index addresses_outputs_by_addresses_id on addresses_outputs (addresses_id);",
		"create index addresses_outputs_by_outputs_id on addresses_outputs (outputs_id);",

		"create table addresses_txs(\n"
		"    addresses_id integer,\n"
		"    txs_id integer\n"
		");",

		//"create index addresses_txs_by_addresses_id on addresses_txs (addresses_id);",
		//"create index addresses_txs_by_txs_id on addresses_txs (txs_id);",

		"create table blockchain_head (hash string);",

		"create table cached_balances (id integer primary key, balance integer);",

		"create table block_fees (id integer primary key, average_fee_per_kb integer);",
	};

	DB db(paths.db_path.c_str());
	for (auto &cmd : commands)
		db.exec(cmd);
}

size_t head_selector(const std::vector<const HeadCandidate *> &heads){
	size_t ret;
	do{
		std::cout << "Multiple possible heads. Please select one:\n";
		int i = 1;
		for (auto head : heads)
			std::cout << i++ << ". " << (std::string)head->hash << std::endl;
		std::cin >> ret;
		ret--;
	}while (ret < 0 || ret >= heads.size());
	return ret;
}

std::unique_ptr<Blockchain> initialize_blockchain(sqlite3pp::DB &db, const Paths &paths, bool testnet){
	using namespace sqlite3pp;
	std::unique_ptr<Blockchain> blockchain;
	try{
		blockchain = std::make_unique<Blockchain>(db);
		if (blockchain->get_height() == std::numeric_limits<u64>::max())
			blockchain.reset();
	}catch (std::exception &){}

	if (!blockchain){
		sqlite3pp::Transaction t(db);
		{
			BlockAdder adder(db, paths, testnet);
			adder.process();
		}

		blockchain = std::make_unique<Blockchain>(db, head_selector);
		auto n = blockchain->get_height() + 1;
		auto update = db << "update blocks set previous_blocks_id = ? where id = ?;";
		for (u64 i = 0; i < n; i++){
			auto block = blockchain->get_block_by_height(i);
			update << Reset() << block->previous_block_id << block->db_id << Step();
		}
	}

	return blockchain;
}

size_t find_next_processing_block(sqlite3pp::DB &db, Blockchain &blockchain){
	using namespace sqlite3pp;
	size_t next_processing_block = 0;
	u64 count;
	db << "select count(*) from (select * from txs limit 1);" << Step() >> count;
	if (!count)
		return 0;
	u64 last_block_id;
	db << "select blocks_id from txs where id = (select max(id) from txs);" << Step() >> last_block_id;
	bool found = false;
	auto n = blockchain.get_height() + 1;
	for (size_t i = 0; i < n; i++){
		if (blockchain.get_block_by_height(i)->previous_block_id == last_block_id){
			found = true;
			return i;
		}
	}
	return 0;
}

int main(int argc, char **argv){
	using namespace sqlite3pp;
	using Hashes::Digests::SHA256;

	if (argc < 3){
		mstderr <<
			"Usage: blockchain_parser <config dir> <output dir> [<testnet>]\n"
			"\n"
			"The testnet argument must be a 0 or a 1. If not provided, it defaults to 0.\n";
		return -1;
	}

	const bool testnet = argc >= 4 && atoi(argv[3]);

	if (testnet)
		mstdout << "Using testnet.\n";

	continue_running = true;
	signal(SIGINT, [](auto){ continue_running = false; });
	signal(SIGTERM, [](auto){ continue_running = false; });
	try{
		Paths paths(argv);
		if (!boost::filesystem::exists(paths.db_path))
			initialize_db(paths);

		DB db(paths.db_path.c_str());

		auto blockchain = initialize_blockchain(db, paths, testnet);
		auto next_processing_block = find_next_processing_block(db, *blockchain);

		InsertState nis(db);
		auto stmt = db << "select file_name, file_offset, size_in_file from blocks where id = ?;";
		TaskProgress task("Parsing blocks...");
		auto n = blockchain->get_height() + 1;
		task.start(n - next_processing_block);
		for (auto i = next_processing_block; continue_running && i < n;){
			sqlite3pp::Transaction t(db);
			for (int j = 100; j-- && continue_running && i < n; i++){
				task.report_progress(1);
				std::string filename;
				u64 file_offset, size;
				auto blockchain_block = *blockchain->get_block_by_height(i);
				stmt << Reset() << blockchain_block.db_id << Step() >> filename >> file_offset >> size;
				auto path = paths.config_path + "/blocks/" + filename;
				std::ifstream file(path, std::ios::binary);
				if (!file)
					throw std::runtime_error("File not found: " + path);
				file.seekg(file_offset);
				std::unique_ptr<u8[]> buffer(new u8[size]);
				file.read((char *)buffer.get(), size);
				SerializedBuffer sb(buffer.get(), size);
				::Block block(sb, testnet);
				block.insert(nis);
			}
		}

	}catch (std::exception &e){
		mstderr << e.what() << std::endl;
		return -1;
	}
	return 0;
}
