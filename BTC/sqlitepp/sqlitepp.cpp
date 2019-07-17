#include "sqlite3.h"
#include "sqlitepp.h"
#include <cstring>

namespace sqlite3pp{

static void throw_sqlite_error(int error, sqlite3 *db){
	if (error==SQLITE_OK)
		return;
	std::string error_msg = sqlite3_errmsg(db);
	const char *string;
	switch (error){
		case SQLITE_ERROR:
			string = "Generic SQLite error: ";
			break;
		case SQLITE_INTERNAL:
			string = "Internal SQLite error: ";
			break;
		case SQLITE_PERM:
			string = "Permission denied. Error string: ";
			break;
		case SQLITE_ABORT:
			string = "Abort requested. Error string: ";
			break;
		case SQLITE_BUSY:
			string = "Database is locked. Error string: ";
			break;
		case SQLITE_LOCKED:
			string = "Table is locked. Error string: ";
			break;
		case SQLITE_NOMEM:
			string = "malloc() failed. Error string: ";
			break;
		case SQLITE_READONLY:
			string = "Database is read-only. Error string: ";
			break;
		case SQLITE_INTERRUPT:
			string = "sqlite3_interrupt() called. Error string: ";
			break;
		case SQLITE_IOERR:
			string = "I/O error: ";
			break;
		case SQLITE_CORRUPT:
			string = "Database is corrupt. Error string: ";
			break;
		case SQLITE_FULL:
			string = "Database is full. Error string: ";
			break;
		case SQLITE_CANTOPEN:
			string = "Can't open database. Error string: ";
			break;
		case SQLITE_EMPTY:
			string = "Database is empty. Error string: ";
			break;
		case SQLITE_SCHEMA:
			string = "Database schema changed. Error string: ";
			break;
		case SQLITE_TOOBIG:
			string = "String or blob above size limit. Error string: ";
			break;
		case SQLITE_CONSTRAINT:
			string = "Constraint violation. Error string: ";
			break;
		case SQLITE_MISMATCH:
			string = "Data type mismatch. Error string: ";
			break;
		case SQLITE_MISUSE:
			string = "SQLite used incorrectly. Error string: ";
			break;
		case SQLITE_NOLFS:
			string = "OS features not supported by host. Error string: ";
			break;
		case SQLITE_AUTH:
			string = "Authorization denied. Error string: ";
			break;
		case SQLITE_FORMAT:
			string = "Auxiliary database format error. Error string: ";
			break;
		case SQLITE_RANGE:
			string = "Range error. Error string: ";
			break;
		case SQLITE_NOTADB:
			string = "File is not a database. Error string: ";
			break;
		default:
			return;
	}
	throw std::runtime_error(string + error_msg);
}

DB::DB(const char *path,bool Throw): lock_count(0){
	int error = sqlite3_open(path, &this->db);
	if (Throw)
		throw_sqlite_error(error, this->db);
	else if (error != SQLITE_OK){
		sqlite3_close(this->db);
		this->db = nullptr;
	}
}

void DB::exec(const char *s){
	if (!this->good())
		return;
	int error = sqlite3_exec(this->db, s, nullptr, nullptr, nullptr);
	throw_sqlite_error(error, this->db);
}
	
sqlite3_int64 DB::last_insert_rowid(){
	return sqlite3_last_insert_rowid(this->db);
}


Statement DB::operator<<(const char *s){
	return Statement(this->db, s);
}

Statement::Statement(sqlite3 *db,const char *s){
	this->db = db;
	int error = sqlite3_prepare_v2(db, s, -1, &this->statement, nullptr);
	throw_sqlite_error(error, db);
	this->reset();
}

Statement::Statement(Statement &&s){
	this->db = s.db;
	this->statement = s.statement;
	this->reset();
	s.statement = nullptr;
}

void Statement::uninit(bool throws){
	if (!this->good())
		return;
	int error = sqlite3_finalize(this->statement);
	if (throws)
		throw_sqlite_error(error, this->db);
}

Statement::~Statement(){
	this->uninit(false);
}

const Statement &Statement::operator=(Statement &&s){
	this->uninit();
	this->db = s.db;
	this->statement = s.statement;
	s.statement = nullptr;
	return *this;
}

void Statement::reset(){
	if (!*this)
		return;
	this->bind_index = 1;
	int error;
	error = sqlite3_reset(this->statement);
	throw_sqlite_error(error, this->db);
	error = sqlite3_clear_bindings(this->statement);
	throw_sqlite_error(error, this->db);
}

Statement &Statement::operator<<(const Null &){
	if (!*this)
		return *this;
	int error = sqlite3_bind_null(this->statement, this->bind_index++);
	throw_sqlite_error(error, this->db);
	return *this;
}

Statement &Statement::operator<<(const Step &){
	if (!*this)
		return *this;
	this->step();
	return *this;
}

Statement &Statement::operator<<(const Reset &){
	if (!*this)
		return *this;
	this->reset();
	return *this;
}

Statement &Statement::operator<<(int i){
	if (!*this)
		return *this;
	int error = sqlite3_bind_int(this->statement, this->bind_index++, i);
	throw_sqlite_error(error, this->db);
	return *this;
}

Statement &Statement::operator<<(std::int64_t i){
	if (!*this)
		return *this;
	int error = sqlite3_bind_int64(this->statement, this->bind_index++, (sqlite3_int64)i);
	throw_sqlite_error(error, this->db);
	return *this;
}

Statement &Statement::operator<<(std::uint64_t i){
	return *this << (std::int64_t)i;
}

Statement &Statement::operator<<(double d){
	if (!*this)
		return *this;
	int error = sqlite3_bind_double(this->statement, this->bind_index++, d);
	throw_sqlite_error(error, this->db);
	return *this;
}

Statement &Statement::operator<<(const std::string &s){
	if (!*this)
		return *this;
	int error = sqlite3_bind_text(this->statement, this->bind_index++, s.c_str(), (int)s.size(), SQLITE_TRANSIENT);
	throw_sqlite_error(error, this->db);
	return *this;
}

Statement &Statement::operator<<(const std::vector<unsigned char> &v){
	if (!*this)
		return *this;
	char c;
	auto p = v.size() ? (const void *)&v[0] : (const void *)&c;
	int error = sqlite3_bind_blob(this->statement, this->bind_index++, p, (int)v.size(), SQLITE_TRANSIENT);
	throw_sqlite_error(error, this->db);
	return *this;
}

int Statement::step(){
	if (!*this)
		return SQLITE_ERROR;
	this->get_index = 0;
	int error = sqlite3_step(this->statement);
	throw_sqlite_error(error, this->db);
	return error;
}

Statement &Statement::operator>>(int &i){
	i = sqlite3_column_int(this->statement, this->get_index++);
	return *this;
}

Statement &Statement::operator>>(std::int64_t &i){
	i = sqlite3_column_int64(this->statement, this->get_index++);
	return *this;
}

Statement &Statement::operator>>(std::uint64_t &i){
	std::int64_t temp;
	*this >> temp;
	i = temp;
	return *this;
}

Statement &Statement::operator>>(double &d){
	d = sqlite3_column_double(this->statement, this->get_index++);
	return *this;
}

Statement &Statement::operator>>(std::string &s){
	sqlite3_column_text(this->statement, this->get_index);
	size_t size = sqlite3_column_bytes(this->statement, this->get_index);
	s.resize(size);
	const char *p = (const char *)sqlite3_column_text(this->statement, this->get_index++);
	std::copy(p, p + size, s.begin());
	return *this;
}

Statement &Statement::operator>>(std::vector<unsigned char> &v){
	sqlite3_column_blob(this->statement, this->get_index);
	v.resize(sqlite3_column_bytes(this->statement, this->get_index));
	if (v.size())
		memcpy(&v[0], sqlite3_column_blob(this->statement, this->get_index), v.size());
	this->get_index++;
	return *this;
}

}
