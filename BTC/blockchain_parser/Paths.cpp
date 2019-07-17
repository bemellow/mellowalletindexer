#include "Paths.h"
#include <boost/filesystem.hpp>
#include <sqlitepp/sqlitepp.h>

int load_state(const Paths &paths){
	using namespace sqlite3pp;
	if (!boost::filesystem::exists(paths.db_path))
		return 0;
	DB db(paths.db_path.c_str());
	int ret;
	db << "select state from state;" << Step() >> ret;
	return ret;
}

void save_state(const Paths &paths, int value){
	using namespace sqlite3pp;
	DB db(paths.db_path.c_str());
	Transaction t(db);
	db.exec("delete from state;");
	db << "insert into state (state) values (?);" << value << Step();
}
