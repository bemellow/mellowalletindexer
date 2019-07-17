#ifdef _MSC_VER
#pragma once
#endif

#ifndef SQLITEPP_H
#define SQLITEPP_H
#include <string>
#include <vector>
#include <memory>
#include <boost/optional.hpp>
#include "sqlite3.h"

#ifdef USE_BOOST
#include <boost/filesystem.hpp>
#endif
#ifdef USE_QT
#include <QString>
#endif

#ifdef _MSC_VER
 #ifdef BUILDING_SQLITEPP
  #ifdef _DLL
   #define SQLITEPP_API __declspec(dllexport)
  #else
   #define SQLITEPP_API
  #endif
 #else
  #ifndef SQLITEPP_STATIC
   #define SQLITEPP_API __declspec(dllimport)
  #else
   #define SQLITEPP_API
  #endif
 #endif
#else
 #define SQLITEPP_API
#endif

namespace sqlite3pp{

class Statement;

class SQLITEPP_API DB{
	sqlite3 *db;
	unsigned lock_count;
public:
	DB(const char *path, bool Throw = true);
	~DB(){
		if (this->good())
			sqlite3_close(this->db);
	}
	bool good() const{
		return !!this->db;
	}
	operator sqlite3 *() const{
		return this->db;
	}
	Statement operator<<(const char *s);
	void exec(const char *s);
	sqlite3_int64 last_insert_rowid();
	void begin_transaction(){
		if (!this->lock_count)
			this->exec("begin exclusive transaction;");
		this->lock_count++;
	}
	void commit(){
		if (this->lock_count)
			this->lock_count--;
		if (!this->lock_count)
			this->exec("commit;");
	}
	void rollback(){
		if (this->lock_count){
			this->exec("rollback;");
			this->lock_count=0;
		}
	}
};

class SQLITEPP_API Transaction{
	DB *db;
	Transaction(const Transaction &m){}
	void operator=(const Transaction &){}
	bool _commit;
public:
	Transaction(DB &db):db(&db),_commit(1){
		db.begin_transaction();
	}
	~Transaction(){
		if (!std::uncaught_exception() && this->_commit)
			this->db->commit();
	}
	void commit(){
		this->db->commit();
		this->db->begin_transaction();
	}
	void rollback(){
		this->db->rollback();
		this->_commit=0;
	}
};

class Null{};

class Step{};

class Reset{};

class SQLITEPP_API Statement{
	sqlite3 *db = nullptr;
	sqlite3_stmt *statement = nullptr;
	unsigned bind_index = 0,
		get_index = 0;

	friend class DB;
	Statement(sqlite3 *db,const char *s);
	void uninit(bool throws = true);
public:
	Statement(): statement(nullptr){}
	Statement(const Statement &) = delete;
	Statement(Statement &&);
	~Statement();
	const Statement &operator=(const Statement &) = delete;
	const Statement &operator=(Statement &&);
	bool good() const{
		return !!this->statement;
	}
	bool operator!() const{
		return !this->good();
	}
	void reset();
	Statement &operator<<(const Null &);
	Statement &operator<<(const Step &);
	Statement &operator<<(const Reset &);
	Statement &operator<<(bool b){
		return *this << (int)b;
	}
	Statement &operator<<(int);
	Statement &operator<<(unsigned u){
		return *this << (int)u;
	}
	Statement &operator<<(std::int64_t);
	Statement &operator<<(std::uint64_t);
	Statement &operator<<(double);
	Statement &operator<<(const char *s){
		return *this << (std::string)s;
	}
	Statement &operator<<(const std::string &);
	Statement &operator<<(const std::vector<unsigned char> &);
	/*
		How to use:
		while (stmt.step()==SQLITE_ROW){
			//Get more data.
		}
	*/
	int step();
	void set_get(unsigned i){
		this->get_index = i;
	}
	Statement &operator>>(bool &b){
		int i;
		*this >> i;
		b=!!i;
		return *this;
	}
	Statement &operator>>(int &i);
	Statement &operator>>(unsigned &u){
		int i;
		*this >> i;
		u = (unsigned)i;
		return *this;
	}
	Statement &operator>>(std::int64_t &i);
	Statement &operator>>(std::uint64_t &i);
	Statement &operator>>(double &d);
	Statement &operator>>(std::string &s);
	Statement &operator>>(std::vector<unsigned char> &v);
	template <typename T>
	Statement &operator>>(std::unique_ptr<T> &p){
		auto type = sqlite3_column_type(this->statement, this->get_index);
		if (type == SQLITE_NULL){
			p.reset();
			return *this;
		}
		p.reset(new T);
		return *this >> *p;
	}
	template <typename T>
	Statement &operator>>(std::shared_ptr<T> &p){
		auto type = sqlite3_column_type(this->statement, this->get_index);
		if (type == SQLITE_NULL){
			p.reset();
			return *this;
		}
		p.reset(new T);
		return *this >> *p;
	}
	template <typename T>
	Statement &operator>>(boost::optional<T> &p){
		auto type = sqlite3_column_type(this->statement, this->get_index);
		T temp;
		if (type == SQLITE_NULL){
			p.reset();
			return *this;
		}
		*this >> temp;
		p = temp;
		return *this;
	}
};

}
#endif
