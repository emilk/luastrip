#pragma once

#include <vector>
#include <stdexcept>

typedef unsigned char byte;
typedef std::vector<byte> ByteVec;

ByteVec read_file(const char* path) throw(std::runtime_error);
void    write_file(const char* path, const void* data, size_t num_bytes) throw(std::runtime_error);
