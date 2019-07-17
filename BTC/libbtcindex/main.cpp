#include "Indexer.h"
#include <common/types.h>
#include <common/misc.h>
#include <exception>
#include <iostream>
#include <string>

#ifdef _MSC_VER
#define API extern "C" _declspec(dllexport)
#else
#define API extern "C"
#endif

API Indexer *initialize_index(const char *db_path, bool testnet){
	try{
		//Remember to uncomment this if you ever implement reader-writer locks!
		//sqlite3_config(SQLITE_CONFIG_SERIALIZED);
		return new Indexer(db_path, testnet);
	}catch (std::exception &e){
		std::cerr << e.what() << std::endl;
	}catch (...){
	}
	return nullptr;
}

API void destroy_index(Indexer *index){
	delete index;
}

API const char *index_get_utxo(Indexer *index, const char *addresses){
	try{
		return index->get_utxo(addresses);
	}catch (std::exception &e){
		std::cerr << e.what() << std::endl;
	}catch (...){
	}
	return nullptr;
}

API const char *index_get_utxo_insight(Indexer *index, const char *addresses){
	try{
		return index->get_utxo_insight(addresses);
	}catch (std::exception &e){
		std::cerr << e.what() << std::endl;
	}catch (...){
	}
	return nullptr;
}

API const char *index_get_balance(Indexer *index, const char *addresses){
	try{
		return index->get_balance(addresses);
	}catch (std::exception &e){
		std::cerr << e.what() << std::endl;
	}catch (...){
	}
	return nullptr;
}

API const char *index_get_balances(Indexer *index, const char *addresses){
	try{
		return index->get_balances(addresses);
	}catch (std::exception &e){
		std::cerr << e.what() << std::endl;
	}catch (...){
	}
	return nullptr;
}

API const char *index_get_history(Indexer *index, const char *addresses){
	try{
		return index->get_history(addresses);
	}catch (std::exception &e){
		std::cerr << e.what() << std::endl;
	}catch (...){
	}
	return nullptr;
}

API s64 index_get_blockchain_height(Indexer *index){
	try{
		return index->get_blockchain_height();
	}catch (std::exception &e){
		std::cerr << e.what() << std::endl;
	}catch (...){
	}
	return -1;
}

API const char *index_push_new_block(Indexer *index, const void *data, size_t size){
	try{
		return index->push_new_block(data, size);
	}catch (std::exception &e){
		std::cerr << e.what() << std::endl;
	}catch (...){
	}
	return nullptr;
}

API const char *index_get_fees(Indexer *index){
	try{
		return index->get_fees();
	}catch (std::exception &e){
		std::cerr << e.what() << std::endl;
	}catch (...){
	}
	return nullptr;
}
