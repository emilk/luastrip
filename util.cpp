#include "util.hpp"
#include <string>


ByteVec read_file(const char* path) throw(std::runtime_error)
{
	const size_t CHUNK_SIZE = 32 * 1024; // 32kB ~= 1-2 kloc
	
	FILE* fp = fopen(path, "rb");
	
	if (!fp) {
		throw std::runtime_error("File not found: " + std::string(path));
	}
	
	ByteVec buff;
	size_t  num_read = 0;
	
	while (feof(fp) == 0)
	{
		buff.resize(num_read + CHUNK_SIZE);
		size_t n = fread(&buff[num_read], 1, CHUNK_SIZE, fp);
		num_read += n;
		buff.resize(num_read);
	}
	
	fclose(fp);
	
	return buff;
}

void write_file(const char* path, const void* data, size_t num_bytes) throw(std::runtime_error)
{
	FILE* fp = fopen(path, "wb");
	
	if (!fp) {
		throw std::runtime_error("File not found");
	}

	fwrite(data, 1, num_bytes, fp);
	
	fclose(fp);
}
