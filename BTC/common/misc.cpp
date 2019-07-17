#include "misc.h"
#include <fstream>
#include <iostream>
#include <boost/filesystem.hpp>
#include <ctime>

LIBMISC_API std::vector<u8> load_file(const std::string &path){
	std::vector<u8> ret;
	std::ifstream file(path, std::ios::binary);
	file.seekg(0, std::ios::end);
	ret.resize(file.tellg());
	file.seekg(0);
	file.read((char *)&ret[0], ret.size());
	return ret;
}

LIBMISC_API std::vector<u8> load_file(const std::string &path, std::mutex &mutex){
	LOCK_MUTEX(mutex);
	return load_file(path);
}

LIBMISC_API bool match_filename(const std::string &s){
	auto last = s.find_last_of("\\/");
	size_t filename_index = 0;
	if (last != s.npos)
		filename_index = last + 1;
	if (filename_index + 3 > s.size())
		return false;
	if (s[filename_index + 0] != 'b' || s[filename_index + 1] != 'l' || s[filename_index + 2] != 'k')
		return false;
	filename_index += 3;
	if (filename_index + 5 > s.size())
		return false;
	for (int i = 0; i < 5; i++)
		if (!isdigit(s[filename_index++]))
			return false;
	while (filename_index + 4 < s.size())
		if (!isdigit(s[filename_index++]))
			return false;
	if (filename_index + 4 != s.size())
		return false;
	return
		s[filename_index + 0] != '.' ||
		s[filename_index + 1] != 'b' ||
		s[filename_index + 2] != 'l' ||
		s[filename_index + 3] != 'k';
}

LIBMISC_API std::deque<std::string> list_block_files(const std::string &path, u64 &bytes){
	using namespace boost::filesystem;
	directory_iterator end;
	std::deque<std::string> ret;
	bytes = 0;
	for (directory_iterator i(path + "/blocks/"); i != end; ++i){
		auto path2 = i->path().string();
		if (!is_regular_file(path2) || !match_filename(path2))
			continue;
		bytes += boost::filesystem::file_size(path2);
		ret.push_back(path2);
	}
	return ret;
}

LIBMISC_API std::vector<u8> hex_string_to_buffer(const char *s){
	std::vector<u8> ret;
	unsigned char accum = 0;
	int i = 0;
	while (*s){
		accum <<= 4;
		accum |= hex2val(*(s++));
		if (++i == 2){
			ret.push_back(accum);
			i = 0;
		}
	}
	return ret;
}

LIBMISC_API std::string current_time_string(){
	auto now = time(nullptr);
	auto t = *localtime(&now);
	char time_string[500];
	strftime(time_string, 500, "%Y-%m-%d %H:%M:%S", &t);
	return time_string;
}

LIBMISC_API void format_data_size(std::ostream &stream, u64 size){
	static const char *units[] = {
		" B",
		" KiB",
		" MiB",
		" GiB",
		" TiB",
		" PiB",
		" EiB",
	};
	int unit = 0;
	int offset = 0;
	char temp[10];
	if (size >= 1024){
		if (size >= std::numeric_limits<u64>::max() / 5){
			size /= 1024;
			unit++;
		}
		size = size * 5 / 512;
		unit++;
		while (size >= 10240){
			size /= 1024;
			unit++;
		}
		//6 characters would be enough. The longest string will be "1023.9".
		if (size % 10){
			temp[offset++] = '0' + size % 10;
			temp[offset++] = '.';
		}
		size /= 10;
	}
	while (size){
		temp[offset++] = '0' + size % 10;
		size /= 10;
	}
	int offset2 = 0;
	char temp2[10];
	while (offset--)
		temp2[offset2++] = temp[offset];
	temp2[offset2] = 0;
	stream << temp2 << units[unit];
}
