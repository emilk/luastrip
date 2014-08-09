#pragma once

#include <cstdlib> // malloc/free
#include <vector>
#include <utility> // max
#include <cassert>

// ----------------------------------------

typedef uint8_t byte;

#define ALIGNMENT 8

inline size_t align(size_t v)
{
	return v/ALIGNMENT*ALIGNMENT + ALIGNMENT;
}


class Allocator
{
public:
	// Pick a chunk_size close to the expected final memory usage
	explicit Allocator(size_t chunk_size) : m_chunk_size(std::max<size_t>(1024, align(chunk_size))) {}
	
	~Allocator() {
#if 1
		for (byte* chunk : m_chunks) {
			free(chunk);
		}
#endif
	}
	
	inline byte* allocate(size_t num_bytes)
	{
#if 0
		return (byte*)malloc(num_bytes);
#elif 1
		num_bytes = align(num_bytes);
		byte* next_current = m_current + num_bytes;
		if (next_current < m_current_end) {
			byte* ret = m_current;
			m_current = next_current;
			return ret;
		} else {
			assert(num_bytes <= m_chunk_size);
			byte* ret     = (byte*)malloc(m_chunk_size);
			m_chunks.push_back(ret);
			m_current     = ret + num_bytes;
			m_current_end = ret + m_chunk_size;
			return ret;
		}
#else
		num_bytes = align(num_bytes);
		if (num_bytes > m_left) {
			assert(num_bytes <= m_chunk_size);
			//m_chunk_size = std::max(m_chunk_size, num_bytes);
			m_left       = m_chunk_size;
			m_current    = (byte*)malloc(m_chunk_size);
			//memset(m_current, 0, m_chunk_size);
			m_chunks.push_back(m_current);
		}
		byte* ret = m_current;
		m_current += num_bytes;
		m_left    -= num_bytes;
		return ret;
#endif
	}
	
	// Helper:
	template<typename T, typename... Args>
	inline T* make(Args&&... args)
	{
		T* ptr = (T*)allocate(sizeof(T));
		new(ptr) T(args...);
		return ptr;
	}
	
	// Num bytes used
	size_t size() const {
		//return m_chunks.size() * m_chunk_size - m_left;
		return m_chunks.size() * m_chunk_size - (m_current_end - m_current);
	}
	
private:
	Allocator(Allocator&);
	Allocator(Allocator&&);
	Allocator& operator=(Allocator&);
	Allocator& operator=(Allocator&&);
	
	size_t             m_chunk_size;
	std::vector<byte*> m_chunks;  // All
	byte*              m_current = nullptr; // Currently being filled
	byte*              m_current_end = nullptr; // m_current_end - m_current = bytes left in chunk
	//size_t             m_left = 0;  // In m_current;
};


// ----------------------------------------


// Binder for STL:
template<class T>
class STLAllocator
{
public:
	typedef T value_type;
	typedef value_type* pointer;
	typedef const value_type* const_pointer;
	typedef value_type& reference;
	typedef const value_type& const_reference;
	typedef std::size_t size_type;
	typedef std::ptrdiff_t difference_type;
	
	// Does not own the allocator - just refers to it!
	inline STLAllocator(Allocator& allocator) : m_allocator(allocator) {}
	
	inline STLAllocator(const STLAllocator& other) : m_allocator(other.m_allocator) {}
	
	// Used by STL:
	template<typename U>
	inline STLAllocator(const STLAllocator<U>& other) : m_allocator(other.m_allocator)
	{}

	inline ~STLAllocator() {
	}
	
	
	inline pointer       address(reference r)       { return &r; }
	inline const_pointer address(const_reference r) { return &r; }
	
	inline pointer allocate(size_type cnt, typename std::allocator<void>::const_pointer = 0)
	{
		size_t num_bytes = cnt * sizeof (T);
		byte* ptr = m_allocator.allocate(num_bytes);
		return (pointer)ptr;
	}
	
	inline void deallocate(pointer p, size_type)
	{
	}
	
	inline size_type max_size() const
	{
		return std::numeric_limits<size_type>::max() / sizeof(T);
	}
	
	inline void construct(pointer p, const T& t)
	{
		new(p) T(t);
	}
	
	inline void destroy(pointer p)
	{
		p->~T();
	}
	
	inline bool operator==(STLAllocator const&) { return true; }
	inline bool operator!=(STLAllocator const& a) { return !operator==(a); }
	
	// Convert an allocator<T> to allocator<U>
	template<typename U>
	struct rebind {
		typedef STLAllocator<U> other;
	};
	
private:
	Allocator& m_allocator;
};

