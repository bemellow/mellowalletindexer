#pragma once
#include <common/types.h>
#include <string>

struct Paths{
	std::string config_path;
	std::string output_path;
	std::string db_path;

	Paths() = default;
	Paths(char **argv){
		this->config_path = argv[1];
		this->output_path = argv[2];
		this->db_path = this->output_path + "/btc.sqlite";
	}
};

u64 load_tx_count(const Paths &);
void save_tx_count(const Paths &, u64);
int load_state(const Paths &);
void save_state(const Paths &, int);
u64 load_executed_block(const Paths &);
void save_executed_block(const Paths &, u64);
