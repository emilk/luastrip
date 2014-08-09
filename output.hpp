#pragma once

#include <cstdlib>
#include <cstring>  // memcpy
#include <algorithm>

//#include <vector>
//typedef std::vector<char> CharVec;
class CharVec
{
public:
	CharVec() {}
	
	CharVec(CharVec&& other)
	{
		m_ptr  = other.m_ptr;
		m_size = other.m_size;
		m_cap  = other.m_cap;
		other.m_ptr  = nullptr;
		other.m_size = 0;
		other.m_cap  = 0;
	}
	
	~CharVec()
	{
		free(m_ptr);
	}
	
	void reserve(size_t new_cap)
	{
		char* buff = (char*)malloc(new_cap);
		memcpy(buff, m_ptr, m_size);
		free(m_ptr);
		m_ptr = buff;
		m_cap = new_cap;
	}
	
	void append(const char* begin, const char* end)
	{
		size_t num_bytes = end-begin;
		size_t new_size  = m_size + num_bytes;
		
		if (new_size > m_cap) {
			reserve( std::max(2*m_cap, 2*num_bytes) );
		}
		
		memcpy(m_ptr + m_size, begin, num_bytes);
		m_size = new_size;
	}
	
	void append(char c)
	{
		append(&c, &c + 1);
	}
	
	const char* data() const { return m_ptr;  }
	size_t      size() const { return m_size; }
	
private:
	CharVec(CharVec&) = delete;
	CharVec& operator=(CharVec&) = delete;
	CharVec& operator=(CharVec&&) = delete;
	
	char*  m_ptr  = nullptr;
	size_t m_size = 0;
	size_t m_cap  = 0;
};


class Tokens;
class Parser;

CharVec output(const Tokens& tok, const Parser& p);
