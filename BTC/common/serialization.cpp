#include "serialization.h"

template <typename T>
T read_uint(const u8 *buffer, size_t &offset, size_t size){
	const size_t n = sizeof(T);
	if (offset + n > size)
		throw std::runtime_error("Invalid block.");
	T ret = 0;
	for (size_t i = 0; i < n; i++)
		ret |= (T)buffer[offset++] << (T)(8 * i);
	return ret;
}

u8 SerializedBuffer::read_u8(){
	if (this->offset + 1 > this->buffer_size)
		throw std::runtime_error("Invalid block.");
	return this->buffer[this->offset++];
}

u16 SerializedBuffer::read_u16(){
	return read_uint<u16>(this->buffer, this->offset, this->buffer_size);
}

u32 SerializedBuffer::read_u32(){
	return read_uint<u32>(this->buffer, this->offset, this->buffer_size);
}

u64 SerializedBuffer::read_u64(){
	return read_uint<u64>(this->buffer, this->offset, this->buffer_size);
}

u64 SerializedBuffer::read_varint(){
	auto first = this->read_u8();
	if (first < 253)
		return first;
	if (first == 253)
		return this->read_u16();
	if (first == 254)
		return this->read_u32();
	return this->read_u64();
}

Hashes::Digests::SHA256 SerializedBuffer::read_sha256(){
	const auto s = Hashes::Digests::SHA256::size;
	if (offset + s > this->buffer_size)
		throw std::runtime_error("Invalid block.");
	Hashes::Digests::SHA256::digest_t ret;
	for (int i = 0; i < s; i++)
		ret[i] = this->buffer[this->offset++];
	return ret;
}

std::vector<u8> SerializedBuffer::read_sized_buffer(){
	auto n = this->read_varint();
	if (this->buffer_size < this->offset + n)
		throw std::runtime_error("Invalid block.");
	std::vector<u8> ret(n);
	if (n){
		memcpy(&ret[0], this->buffer + this->offset, n);
		this->offset += n;
	}
	return ret;
}

void SerializedBuffer::ignore_sized_buffer(){
	auto n = this->read_varint();
	if (this->buffer_size < this->offset + n)
		throw std::runtime_error("Invalid block.");
	this->offset += n;
}
