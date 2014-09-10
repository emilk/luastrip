#pragma once

#include <stdexcept>  // std::exception
#include <vector>
#include <string>
#include <forward_list>
#include "allocator.hpp"


enum class T : uint8_t
{
	None,
	
	Bof,              // Zero-width start sentinel.
	Eof,              // Zeor-width end sentinel.
	
	// For whitespace tokens:
	Shebang,          // #! usr/bin/lua
	Newline,          // Single \n or \r\n couplet
	Indentation,      // Leading tabs
	Whitespace,       // Spaces or non-leading tabs
	OneLineComment,   // --
	MultilineComment, // --[[ not necessarily multiline, but potentially. --]]
	
	// Non-whitespace:
	Identifier,       // Non-keyword name, /[a-z_][a-z_0-9]*/i
	Integer,          // Digits-only number (maybe hex)
	Number,           // Any number, e.g. 6.02e+23 (also: some hex, e.g. 0x1p­-2)
	String,           // "double quoted", 'single quoted' or [[multiline string]]
	
	
	SYMBOLS_START,
	
	Label,      // ::
	
	Ellipsis,   // ...
	Concat,     // ..
	Dot,        // .
	
	// Delims:
	BracketStart,  // [
	BracketEnd,    // ]
	CurlyStart,    // {
	CurlyEnd,      // }
	ParenStart,    // (
	ParenEnd,      // )
	
	// Comp:
	Eq,      // ==
	Neq,     // ~=
	Leq,     // <=
	Geq,     // >=
	Greater, // >
	Less,    // <
	
	// Assign ops, eg:  += ..= #=
	AddAssign,     // +=
	SubAssign,     // -=
	MulAssign,     // *=
	DivAssign,     // /=
	AppendAssign,  // #=
	ConcatAssign,  // ..=
	
	FunctionTypeArrow, // ->
	MapArrow,          // =>
	
	Add, Sub, Mul, Div, Pow, Mod,
	Comma, Semicolon, Hash, Colon,
	Assign, Questionmark,
	
	SYMBOLS_END,
	
		
	KEYWORDS_START,
	Do, If, In, Or,
	And, End, For, Nil, Not, Var,
	Else, Goto, Then, True, Void,
	Break, False, Local, Until, While, Class,
	Elseif, Repeat, Return, Global, Extern, Typedef, Function,
	KEYWORDS_END,
	
	
	NUM_TYPES
};


const char* tok_name(T t);


struct Token
{
	// [begin, end) contains the tokens. Includes quotes for strings.
	const char* str_begin;
	const char* str_end;
	
	// The start of the token is at...
	unsigned    line;  // Starts counting at 1
	unsigned    col;
	
	T   type;
};


// Line, Char, Descr
class sol_error : public std::exception
{
public:
	sol_error(unsigned line, unsigned col, std::string what)
	: m_line(line), m_col(col), m_what(what) {}
	
	const char* what() const override {
		return m_what.c_str();
	}
	
	unsigned line() const { return m_line; }
	unsigned col()  const { return m_col; }
	
private:
	unsigned m_line, m_col;
	std::string m_what;
};

class lex_error : public sol_error
{
public:
	using sol_error::sol_error;
};

class parse_error : public sol_error
{
public:
	using sol_error::sol_error;
};



// Used by Ast-nodes to hold a list of its tokens.
// Used later for restoring whitespaces etc.
typedef std::vector<const Token*, STLAllocator<const Token*>> TokList;
//typedef std::forward_list<const Token*, STLAllocator<const Token*>> TokList;



/*
 Encapsulates a tokenized file, including support data, like allocator.
 Also provides an interface for parsing through the tokens.
 */
class Tokens
{
public:
	/*
	 Zero-ended ASCII encoded string of characters.
	 The lifetime of the string is assumed to be longer than the lifetime of 'Tokens'.
	 'size' == strlen(str)
	 */
	explicit Tokens(const char* str, size_t size, bool sol) throw(lex_error);
	
	size_t num_whites() const { return m_whites.size(); }
	size_t num_tokens() const { return m_tokens.size(); }
	size_t input_size() const { return m_end-m_start; }
	
	inline bool         is_eof()     const { return m_it->type == T::Eof; }
	inline const Token* peek_token() const { return &*m_it; }
	inline T            peek()       const { return m_it->type; }
	inline T            lookahead()  const { return (m_it+1)->type; }
	
	// Does the next token have any preceeding whitespace?
	inline bool has_space_before() const {
		size_t ix = m_it - m_tokens.begin();
		return m_white_ix[ix-1] != m_white_ix[ix];
	}
	
	inline void get_space_before(const char*& out_begin, const char*& out_end, const Token* t) const
	{
		assert(m_tokens.data() < t && t < m_tokens.data() + m_tokens.size());
		//size_t ix = t - m_tokens.data();
		//size_t white_begin = m_white_ix[ix-1];
		//out_begin = m_whites[white_begin].str_begin;
		out_begin = (t-1)->str_end;
		out_end   = t->str_begin;
		assert(out_begin <= out_end);
	}
	
	inline const Token* get_white(size_t ix) const
	{
		return &m_whites[ix];
	}
	
	inline size_t white_index(const Token* t) const
	{
		assert(m_tokens.data() < t && t < m_tokens.data() + m_tokens.size());
		size_t ix = t - m_tokens.data();
		return m_white_ix[ix];
	}
	
	
	inline bool is(T type) {
		return m_it->type == type;
	}
	
	inline const Token* consume_any(TokList& tl) {
		const Token* t = &*m_it;
		tl.push_back(t);
		++m_it;
		return t;
	}
	
	inline const Token* try_consume(T type, TokList& tl) {
		if (m_it->type == type) {
			return consume_any(tl);
		} else {
			return nullptr;
		}
	}
	
	inline const Token* force_consume(T type, TokList& tl) {
		const Token* t = try_consume(type, tl);
		if (t == nullptr) {
			throw lex_error(m_it->line, m_it->col,
								 (std::string)"Expected " + tok_name(type) +
								 ", got " + tok_name(m_it->type));
		}
		return t;
	}
	
	
private:
	typedef std::vector<Token> TokList;
	TokList m_whites; // Ignorable tokens, i.e. comments, whitespace
	TokList m_tokens; // Proper tokens. First is 'Bof' last is 'Eof'.
	
	/*
	 token 'ix' is preceeded by the whitepsace in the range:
	    [ m_white_ix[ix-1], m_white_ix[ix] )
	 and followed by the whitespace in the range
	    [ m_white_ix[ix], m_white_ix[ix+1] )
	*/
	std::vector<size_t> m_white_ix;
	
	// Entire file:
	const char* m_start;
	const char* m_end;
	
	// For reading (mutable):
	TokList::const_iterator m_it;
};
