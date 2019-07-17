#include "SHA256.h"

extern const char hex_digits[] = "0123456789abcdef";

SHA256::SHA256(const std::string &digest): SHA256(digest.c_str(), digest.size()){}

SHA256::SHA256(const char *digest, size_t size){
	if (!size)
		size = strlen(digest);
	if (size < 2)
		throw std::runtime_error("Invalid hex.");
	if (digest[0] == '0' && tolower(digest[1]) == 'x'){
		digest += 2;
		size -= 2;
	}
	if (size != this->size * 2)
		throw std::runtime_error("Invalid hex.");
	int j = 0;
	for (int i = 0; i < this->size; i++){
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
	return std::string(temp, string_size - 1);
}

void SHA256::write_to_char_array(char (&array)[string_size]) const{
	array[0] = '0';
	array[1] = 'x';
	array[string_size - 1] = 0;
	for (size_t i = 0; i < size; i++){
		array[2 + i * 2 + 0] = hex_digits[this->digest[i] >> 4];
		array[2 + i * 2 + 1] = hex_digits[this->digest[i] & 0x0F];
	}
}

void SHA256::write_to_char_vector(std::vector<char> &s) const{
	char array[string_size];
	this->write_to_char_array(array);
	for (int i = 0; i < sizeof(array) - 1; i++)
		s.push_back(array[i]);
}
