#pragma once
#include "types.h"
#include <libhash/hash.h>
#include <stdexcept>
#include <libmisc/declspec.h>

class LIBMISC_API SerializedBuffer{
	const u8 *buffer;
	size_t buffer_size;
	size_t offset;
public:
	SerializedBuffer(const void *buffer, size_t size):
		buffer((const u8 *)buffer),
		buffer_size(size),
		offset(0){}
	u8 read_u8();
	u16 read_u16();
	u32 read_u32();
	u64 read_u64();
	u64 read_varint();
	Hashes::Digests::SHA256 read_sha256();
	std::vector<u8> read_sized_buffer();
	void ignore_sized_buffer();
	const void *get_absolute_buffer(size_t offset = 0) const{
		return this->buffer + offset;
	}
	const void *get_buffer(int offset = 0) const{
		return this->buffer + this->offset + offset;
	}
	size_t get_offset() const{
		return this->offset;
	}
	void set_offset(size_t offset){
		this->offset = offset;
	}
	size_t get_size() const{
		return this->buffer_size;
	}
	size_t remaining_bytes() const{
		if (this->offset >= this->buffer_size)
			return 0;
		return this->buffer_size - this->offset;
	}

	//template <typename T1, typename T2>
	//void read_sized_vector(std::vector<T1> &dst, T2 &dst_size){
	//	dst_size = this->read_varint();
	//	dst.clear();
	//	dst.reserve(dst_size);
	//	for (T2 i = 0; i < dst_size; i++)
	//		dst.emplace_back(*this);
	//}
	template <typename T1, typename T2, typename ... T3>
	void read_sized_vector(std::vector<T1> &dst, T2 &dst_size, T3 &&...t3){
		dst_size = this->read_varint();
		dst.clear();
		dst.reserve(dst_size);
		for (T2 i = 0; i < dst_size; i++)
			dst.emplace_back(*this, t3...);
	}
};
