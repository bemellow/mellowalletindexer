#pragma once

#include "ParallelBlockProcessor.h"
#include <sqlitepp/sqlitepp.h>

class BlockAdder : public ParallelBlockProcessor{
	std::mutex output_mutex;
	sqlite3pp::DB *db;
	sqlite3pp::Statement insert;

	void on_block(std::unique_ptr<Block> &&block, void *tls) override;
public:
	BlockAdder(sqlite3pp::DB &db, const Paths &paths, bool testnet);
};
