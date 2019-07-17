#include <sqlitepp/sqlitepp.h>
#include <common/types.h>
#include <common/misc.h>
#include <iostream>

using namespace sqlite3pp;

int main(int argc, char **argv){
	if (argc < 2)
		return -1;
	
	try{
		DB db(argv[1]);

		std::vector<std::pair<u64, u64>> pairs;

		size_t total = 0;
		db << "select sum(utxo_count) from addresses;" << Step() >> total;
		pairs.reserve(total);

		for (auto stmt = db << "select id, utxo_count, utxos from addresses;"; stmt.step() == SQLITE_ROW;){
			u64 id, utxo_count;
			std::vector<u8> buffer;
			stmt >> id >> utxo_count >> buffer;
			auto deserialized = deserialize_set_as_vector<u64>(buffer, utxo_count);
			for (auto i : deserialized)
				pairs.emplace_back(id, i);
		}

		try{
			db.exec("drop table addresses_utxos;");
		}catch (std::exception &){
		}

		db.exec("create table addresses_utxos (addresses_id integer, utxos_id integer);");

		{
			Transaction transaction(db);
			auto stmt = db << "insert into addresses_utxos (addresses_id, utxos_id) values (?, ?);";
			for (auto &p : pairs)
				stmt << Reset() << p.first << p.second << Step();
		}

		db.exec("create index addresses_utxos_by_addresses_id on addresses_utxos (addresses_id);");
		db.exec("create index addresses_utxos_by_utxos_id on addresses_utxos (utxos_id);");
	}catch (std::exception &e){
		std::cerr << e.what() << std::endl;
		return -1;
	}
	return 0;
}
