#pragma once
#include "types.h"
#include <vector>
#include <mutex>
#include <deque>
#include <set>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#include <libmisc/declspec.h>

#define LOCK_MUTEX(x) std::lock_guard<decltype(x)> lg_##__COUNTER__(x);

static const char hex_digits[] = "0123456789abcdef";

inline void full_assert_impl(bool condition, const char *description){
	if (!condition)
		throw std::runtime_error((std::string)"Assertion failed: " + description);
}

#define full_assert(x) full_assert_impl(x, #x)

template<class It, class F>
It find_first_true(It begin, It end, const F &f){
	if (begin >= end)
		return end;
	if (f(*begin))
		return begin;
	auto diff = end - begin;
	while (diff > 1){
		auto pivot = begin + diff / 2;
		if (!f(*pivot))
			begin = pivot;
		else
			end = pivot;
		diff = end - begin;
	}
	return end;
}

#if 0
//#define SPP_USE_SPP_ALLOC 1
#include "sparsepp/spp.h"
typedef spp::sparse_hash_map<std::string, u64> address_map_t;
class AddressEncoder{
	std::vector<std::pair<std::string, u64>> old;
	address_map_t map;
	u64 next_id = 1;
	std::vector<std::pair<std::string, u64>> new_pairs;
	mutable boost::shared_mutex mutex;
	//std::shared_mutex mutex;
public:
	AddressEncoder() = default;
	AddressEncoder(size_t count, const std::function<std::pair<std::string, u64>()> &read){
		this->next_id = 0;
		for (auto i = count; i--;){
			auto kv = read();
			this->next_id = kv.second;
			this->old.emplace_back(std::move(kv));
		}
		std::sort(this->old.begin(), this->old.end());
		this->next_id++;
	}
	u64 encode(const std::string &address){
		{
			auto b = this->old.begin();
			auto e = this->old.end();
			auto it = find_first_true(b, e, [&](const std::pair<std::string, u64> &p){ return p.first >= address; });
			if (it != e && it->first == address)
				return it->second;
		}
		{
			boost::upgrade_lock<boost::shared_mutex> l(this->mutex);
			auto it = this->map.find(address);
			if (it != this->map.end())
				return it->second;
			boost::upgrade_to_unique_lock<boost::shared_mutex> ul(l);
			auto ret = this->next_id++;
			this->map[address] = ret;
			this->new_pairs.emplace_back(address, ret);
			return ret;
		}
	}
	std::vector<std::pair<std::string, u64>> get_new_pairs(){
		std::vector<std::pair<std::string, u64>> ret;
		{
			boost::unique_lock<boost::shared_mutex> ul(this->mutex);
			ret = std::move(this->new_pairs);
		}
		return ret;
	}
};
#endif

template <typename T>
void write_natural_to_vector(std::vector<char> &dst, T n){
	if (!n)
		dst.push_back('0');
	else{
		char array[sizeof(T) * 8];
		char *p = array + sizeof(array);
		*--p = 0;
		while (n){
			*--p = hex_digits[n % 10];
			n /= 10;
		}
		for (; *p; p++)
			dst.push_back(*p);
	}
}

template <typename T>
void write_integer_to_vector(std::vector<char> &dst, T n){
	if (n < 0){
		dst.push_back('-');
		n = -n;
	}
	write_natural_to_vector(dst, n);
}

template <typename T>
typename std::enable_if<std::is_unsigned<T>::value, void>::type serialize(std::vector<u8> &dst, T n){
	static_assert(CHAR_BIT == 8, "Only 8-bit byte platforms supported!");

	if (!n){
		dst.push_back(0);
		return;
	}

	const unsigned shift = 7;
	const size_t capacity = (sizeof(n) * 8 + 6) / 7 * 2;
	u8 buffer[capacity]; //times 2 for safety
	const u8 mask = 0x7F;

	size_t buffer_size = 0;

	while (n > 0){
		u8 m = n & mask;
		n >>= shift;
		m &= mask;
		m |= ~mask;
		buffer[capacity - 1 - buffer_size++] = m;
	}
	buffer[capacity - 1] &= mask;
	auto offset = dst.size();
	dst.resize(offset + buffer_size);
	memcpy(&dst[offset], buffer + (capacity - buffer_size), buffer_size);
}

template <typename T>
typename std::enable_if<std::is_unsigned<T>::value, T>::type deserialize(const std::vector<u8> &src, size_t &offset){
	static_assert(CHAR_BIT == 8, "Only 8-bit byte platforms supported!");

	const unsigned shift = 7;

	T ret = 0;

	const u8 more_mask = 0x80;
	const u8 mask = (u8)~more_mask;
	u8 byte;
	do {
		if (offset >= src.size())
			throw std::out_of_range("Incorrect deserialize() call.");
		byte = src[offset++];
		ret <<= shift;
		ret |= byte & mask;
	}while ((byte & more_mask) == more_mask);
	return ret;
}

template <typename T>
std::vector<u8> serialize_set(const std::set<T> &set){
	std::vector<u8> ret;
	if (!set.size())
		return ret;
	auto beg = set.begin();
	auto end = set.end();
	T last = *beg;
	serialize(ret, last);
	while (++beg != end){
		auto x = *beg;
		serialize(ret, x - last);
		last = x;
	}
	return ret;
}

template <typename It, typename F>
std::vector<u8> serialize_transformed(It begin, It end, const F &transform){
	std::vector<u8> ret;
	if (begin == end)
		return ret;
	auto last = transform(*begin);
	serialize(ret, last);
	while (++begin != end){
		auto x = transform(*begin);
		serialize(ret, x - last);
		last = x;
	}
	return ret;
}

template <typename T>
std::set<T> deserialize_set(const std::vector<u8> &v){
	std::set<T> ret;
	if (!v.size())
		return ret;
	size_t offset = 0;
	auto last = deserialize<T>(v, offset);
	ret.insert(last);
	while (offset < v.size()){
		auto x = deserialize<T>(v, offset) + last;
		ret.insert(x);
		last = x;
	}
	return ret;
}

template <typename T>
std::vector<T> deserialize_set_as_vector(const std::vector<u8> &v, size_t reserve = 0){
	std::vector<T> ret;
	if (!v.size())
		return ret;
	if (reserve)
		ret.reserve(reserve);
	size_t offset = 0;
	auto last = deserialize<T>(v, offset);
	ret.push_back(last);
	while (offset < v.size()){
		auto x = deserialize<T>(v, offset) + last;
		ret.push_back(x);
		last = x;
	}
	return ret;
}

template <typename T>
std::vector<T> set_union(const std::vector<T> &a, const std::vector<T> &b){
	std::vector<T> ret;
	ret.resize(a.size() + b.size());
	auto end = std::set_union(a.begin(), a.end(), b.begin(), b.end(), ret.begin());
	auto size = end - ret.begin();
	ret.resize(size);
	return ret;
}

LIBMISC_API std::vector<u8> load_file(const std::string &path);
LIBMISC_API std::vector<u8> load_file(const std::string &path, std::mutex &mutex);
LIBMISC_API bool match_filename(const std::string &s);
LIBMISC_API std::deque<std::string> list_block_files(const std::string &path, u64 &bytes);
LIBMISC_API std::vector<u8> hex_string_to_buffer(const char *s);
LIBMISC_API std::string current_time_string();
LIBMISC_API void format_data_size(std::ostream &, u64 size);

inline u8 hex2val(char c){
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	throw std::runtime_error("Invalid hex.");
}

class SizeFormatter{
public:
	u64 size;
	SizeFormatter(u64 size): size(size){}
};

inline std::ostream &operator<<(std::ostream &stream, const SizeFormatter &sf){
	format_data_size(stream, sf.size);
	return stream;
}

class OutputSequencerTemp{
	std::ostream *dst;
	std::mutex *mutex;
	std::stringstream stream;
public:
	OutputSequencerTemp(): dst(nullptr), mutex(nullptr){}
	OutputSequencerTemp(std::ostream &dst, std::mutex &m): dst(&dst), mutex(&m){}
	OutputSequencerTemp(OutputSequencerTemp &&other):
			dst(other.dst),
			mutex(other.mutex),
			stream(std::move(other.stream)){
		other.dst = nullptr;
		other.mutex = nullptr;
	}
	const OutputSequencerTemp &operator=(OutputSequencerTemp &&other){
		this->dst = other.dst;
		other.dst = nullptr;
		this->mutex = other.mutex;
		other.mutex = nullptr;
		this->stream = std::move(other.stream);
		return *this;
	}
	OutputSequencerTemp(const OutputSequencerTemp &) = delete;
	OutputSequencerTemp &operator=(const OutputSequencerTemp &) = delete;
	~OutputSequencerTemp(){
		if (!this->dst)
			return;
		LOCK_MUTEX(*this->mutex);
		*this->dst << this->stream.str();
	}
	template <typename T>
	OutputSequencerTemp operator<<(const T &x){
		this->stream << x;
		return std::move(*this);
	}
	OutputSequencerTemp operator<<(std::ostream &(*f)(std::ostream &)){
		this->stream << f;
		return std::move(*this);
	}
	OutputSequencerTemp operator<<(std::ios &(*f)(std::ios &)){
		this->stream << f;
		return std::move(*this);
	}
	OutputSequencerTemp operator<<(std::ios_base &(*f)(std::ios_base &)){
		this->stream << f;
		return std::move(*this);
	}
};

class OutputSequencer{
	std::ostream &stream;
	std::mutex &mutex;
public:
	OutputSequencer(std::ostream &stream, std::mutex &mutex): stream(stream), mutex(mutex){}
	OutputSequencer(const OutputSequencer &) = delete;
	OutputSequencer &operator=(const OutputSequencer &) = delete;
	OutputSequencer(OutputSequencer &&) = delete;
	OutputSequencer &operator=(OutputSequencer &&) = delete;
	template <typename T>
	OutputSequencerTemp operator<<(const T &x){
		return OutputSequencerTemp(this->stream, this->mutex) << x;
	}
	OutputSequencerTemp operator<<(std::ostream &(*f)(std::ostream &)){
		return OutputSequencerTemp(this->stream, this->mutex) << f;
	}
};
