#include "add_all_blocks.h"
#include <boost/filesystem/path.hpp>

using namespace sqlite3pp;

BlockAdder::BlockAdder(sqlite3pp::DB &db, const Paths &paths, bool testnet):
	ParallelBlockProcessor("Adding all blocks", paths, testnet),
	db(&db),
	insert(db << "insert into blocks (hash, previous_hash, timestamp, transaction_count, file_name, file_offset, size_in_file) values (?, ?, ?, ?, ?, ?, ?);"){
}

void BlockAdder::on_block(std::unique_ptr<Block> &&block, void *){
	LOCK_MUTEX(this->output_mutex);
	insert
		<< Reset()
		<< (std::string)block->get_hash()
		<< (std::string)block->get_previous_hash()
		<< block->get_timestamp()
		<< block->get_transactions().size()
		<< boost::filesystem::path(block->get_path()).leaf().string()
		<< block->get_proper_offset()
		<< block->get_proper_length()
		<< Step();
}
