#include "hash.h"
#include "sha256.h"
#include "ripemd160.h"

namespace Hashes{
namespace Digests{

SHA256::SHA256(const std::string &digest): SHA256(digest.c_str(), digest.size()){}

SHA256::SHA256(const char *digest, size_t size){
	if (size != this->size * 2)
		throw std::runtime_error("Invalid hex.");
	int j = 0;
	for (int i = this->size; i--;){
		u8 b = hex2val(digest[j++]);
		b <<= 4;
		b |= hex2val(digest[j++]);
		this->digest[i] = b;
	}
}

int SHA256::cmp(const SHA256 &other) const{
	return memcmp(this->digest.data(), other.digest.data(), this->digest.size());
}

SHA256::operator std::string() const{
	char temp[string_size];
	this->write_to_char_array(temp);
	return std::string(temp, size * 2);
}

void SHA256::write_to_char_array(char (&array)[string_size]) const{
	array[string_size - 1] = 0;
	for (size_t i = 0; i < size; i++){
		array[i * 2 + 0] = hex_digits[this->digest[size - 1 - i] >> 4];
		array[i * 2 + 1] = hex_digits[this->digest[size - 1 - i] & 0x0F];
	}
}

void SHA256::write_to_char_vector(std::vector<char> &s) const{
	char array[string_size];
	this->write_to_char_array(array);
	for (int i = 0; i < sizeof(array) - 1; i++)
		s.push_back(array[i]);
}

int RIPEMD160::cmp(const RIPEMD160 &other) const{
	return memcmp(this->digest.data(), other.digest.data(), this->digest.size());
}

} //Digests

namespace Algorithms{
	
SHA256::digest_t SHA256::compute(const void *buffer, size_t size, unsigned iterations){
	digest_t ret;
	if (!iterations){
		auto &array = ret.to_array();
		memset(array.data(), 0, array.size());
		return ret;
	}
	
	iterations--;
	{
		SHA256 temp;
		temp.update(buffer, size);
		ret = temp.final();
	}
	while (iterations--)
		ret = compute(ret.to_array());
	return ret;
}

SHA256::digest_t SHA256::compute(const std::vector<u8> &buffer, unsigned iterations){
	if (!iterations)
		return {};
	if (!buffer.size()){
		SHA256 temp;
		return temp.final();
	}
	return compute(&buffer[0], buffer.size(), iterations);
}

void SHA256::reset(){
	sha256_init(&this->state);
}

void SHA256::update(const void *buffer, size_t size){
	sha256_update(&this->state, (const u8 *)buffer, size);
}

SHA256::digest_t SHA256::final(){
	digest_t::digest_t ret;
	sha256_final(&this->state, ret.data());
	return ret;
}

RIPEMD160::digest_t RIPEMD160::compute(const void *buffer, size_t size){
	RIPEMD160 temp;
	temp.update(buffer, size);
	return temp.final();
}

RIPEMD160::digest_t RIPEMD160::compute(const std::vector<u8> &buffer){
	if (!buffer.size()){
		RIPEMD160 temp;
		return temp.final();
	}
	return compute(&buffer[0], buffer.size());
}

void RIPEMD160::reset(){
	ripemd160_init(&this->state);
}

void RIPEMD160::update(const void *buffer, size_t size){
	ripemd160_process(&this->state, buffer, size);
}

Digests::RIPEMD160 RIPEMD160::final(){
	digest_t::digest_t ret;
	ripemd160_done(&this->state, ret.data());
	return ret;
}

} //Algorithms

} //Hashes
