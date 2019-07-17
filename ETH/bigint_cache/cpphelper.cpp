#include "BigintCache.h"
#include "Blockchain.h"
#include "TimestampIndex.h"
#include <limits>
#include <sstream>
#include <cstring>

#ifdef WIN32
#define EXPORT extern "C" __declspec(dllexport)
#else
#define EXPORT extern "C"
#endif

EXPORT BigintCache *create_cache(u64 size){
	return new BigintCache(size);
}

EXPORT void release_cache(BigintCache *cache){
	delete cache;
}

template <typename T, size_t N>
constexpr size_t array_size(const T (&)[N]){
	return N;
}

EXPORT s64 encode_bigint(BigintCache *cache, const char *string, bool readonly, bool *new_id){
	static const s64 error = -1;
	auto length = strlen(string);
	if (length < 2 || length % 2 || string[0] != '0' || tolower(string[1]) != 'x')
		return error;
	string += 2;
	length -= 2;
	if (length > 40)
		return error;
	char temp_string[40];
	memset(temp_string, '0', 40 - length);
	memcpy(temp_string + (40 - length), string, length);
	string = temp_string;
	length = 40;
	bigint bi;
	memset(bi.data, 0, sizeof(bi.data));
	for (size_t i = array_size(bi.data); i-- && length;){
		for (int j = sizeof(*bi.data) * 2; j-- && length;){
			bi.data[i] <<= 4;
			bi.data[i] |= hex2val(*string);
			string++;
			length--;
		}
	}
	return cache->encode(bi, readonly, *new_id);
}

EXPORT bool decode_id(BigintCache *cache, char *dst, u64 id){
	bool ret;
	auto bi = cache->decode(id, ret);
	if (!ret)
		return false;
	*(dst++) = '0';
	*(dst++) = 'x';
	for (size_t i = array_size(bi.data); i--;){
		for (int j = sizeof(*bi.data); j--;){
			*(dst++) = hex_digits[bi.data[i] >> 28];
			bi.data[i] <<= 4;
			*(dst++) = hex_digits[bi.data[i] >> 28];
			bi.data[i] <<= 4;
		}
	}
	*dst = 0;
	return true;
}

EXPORT void finish_preloading(BigintCache *cache){
	cache->finish_preloading();
}

//------------------------------------------------------------------------------

EXPORT Blockchain *create_blockchain(get_block_data_callback c1, get_blockchain_head_callback c2){
	try{
		return new Blockchain(c1, c2);
	}catch (std::exception &e){
		std::cerr << "Exception: " << e.what() << std::endl;
		return nullptr;
	}
}

EXPORT void destroy_blockchain(Blockchain *blockchain){
	delete blockchain;
}

char *return_string(const std::string &s){
	auto n = s.size();
	auto ret = new char[n + 1];
	memcpy(ret, &s[0], n);
	ret[n] = 0;
	return ret;
}

EXPORT char *try_add_new_block(Blockchain *blockchain, const char *previous_hash){
	return return_string(blockchain->try_add_new_block((SHA256)previous_hash));
}

EXPORT void release_string(Blockchain *blockchain, char *s){
	delete[] s;
}

EXPORT u64 add_new_block(Blockchain *blockchain, const char *hash, const char *previous_hash, u64 db_id){
	return blockchain->add_new_block((SHA256)hash, (SHA256)previous_hash, db_id);
}

EXPORT char *get_block_by_height(Blockchain *blockchain, u64 height){
	auto block = blockchain->get_block_by_height(height);
	if (!block)
		return nullptr;
	return return_string(*block);
}

EXPORT char *get_block_by_hash(Blockchain *blockchain, const char *hash){
	auto block = blockchain->get_block_by_hash((SHA256)hash);
	if (!block)
		return nullptr;
	return return_string(*block);
}

EXPORT u64 get_height(Blockchain *blockchain){
	return blockchain->get_height();
}

EXPORT char *get_blockchain_head(Blockchain *blockchain){
	auto head = blockchain->get_head();
	if (!head)
		return nullptr;
	return return_string(*head);
}

EXPORT int revert_block(Blockchain *blockchain, const char *hash){
	return (int)blockchain->revert_block((SHA256)hash);
}

//------------------------------------------------------------------------------

EXPORT TimestampIndex *create_timestamp_index(const TimestampItem *array, size_t n){
	try{
		return new TimestampIndex(array, n);
	}catch (std::exception &){
		return nullptr;
	}
}

EXPORT void destroy_timestamp_index(TimestampIndex *index){
	delete index;
}

EXPORT TransactionOrder timestamp_index_get_timestamp(TimestampIndex *index, u64 tx_id){
	return index->get_timestamp(tx_id);
}

EXPORT void timestamp_index_reload_data(TimestampIndex *index, const TimestampItem *array, size_t n){
	index->reload_data(array, n);
}

EXPORT void timestamp_index_add_block(TimestampIndex *index, const TimestampItem &item){
	index->add_block(item);
}
