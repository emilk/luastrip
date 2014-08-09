#include "lexer.hpp"
#include <cassert>


typedef unsigned char uchar;


template<int N>
inline bool check(const char* a, const char* b)
{
	for (int i=0; i<N; ++i) {
		if (a[i] != b[i]) {
			return false;
		}
	}
	return true;
}

inline T get_keyword(bool sol, const char* start, const char* end)
{
	// Yes this is ugly - but fast =)

	size_t n = end-start;
	if (n==2) {
		if (check<2>(start, "if")) return T::If;
		if (check<2>(start, "do")) return T::Do;
		if (check<2>(start, "or")) return T::Or;
		if (check<2>(start, "in")) return T::In;
	} else if (n==3) {
		if (check<3>(start, "end")) return T::End;
		if (check<3>(start, "not")) return T::Not;
		if (check<3>(start, "and")) return T::And;
		if (check<3>(start, "nil")) return T::Nil;
		if (check<3>(start, "for")) return T::For;
		if (sol && check<3>(start, "var")) return T::Var;
	} else if (n==4) {
		if (check<4>(start, "then")) return T::Then;
		if (check<4>(start, "else")) return T::Else;
		if (check<4>(start, "true")) return T::True;
		if (check<4>(start, "goto")) return T::Goto;
		if (sol && check<4>(start, "void")) return T::Void;
	} else if (n==5) {
		if (check<5>(start, "local")) return T::Local;
		if (check<5>(start, "break")) return T::Break;
		if (check<5>(start, "false")) return T::False;
		if (check<5>(start, "until")) return T::Until;
		if (check<5>(start, "while")) return T::While;
		if (sol && check<5>(start, "class")) return T::Class;
	} else if (n==6) {
		if (check<6>(start, "return")) return T::Return;
		if (check<6>(start, "elseif")) return T::Elseif;
		if (check<6>(start, "repeat")) return T::Repeat;
		if (sol && check<6>(start, "global")) return T::Global;
		if (sol && check<6>(start, "extern")) return T::Extern;
	} else if (n==7) {
		if (sol && check<7>(start, "typedef")) return T::Typedef;
	} else if (n==8) {
		if (check<8>(start, "function")) return T::Function;
	}

	return T::Identifier;
}

// Sets an inclusive range
void set_range(bool lookup[256], char a, char b) {
	for (char c=a; c<=b; ++c) {
		lookup[c] = true;
	}
}

// -----------------------------------------------

const char* tok_name(T t)
{
	static const char* s_tok_name[(int)T::NUM_TYPES] = {0};
	static bool s_is_initialized = false;
	if (!s_is_initialized)
	{
#define TOK_NAME(x) s_tok_name[(int)T::x] = ##x
#define TOK_NAME_AS(x, name) s_tok_name[(int)T::x] = name
		TOK_NAME_AS( Bof, "BoF" );
		TOK_NAME_AS( Eof, "EoF" );

		TOK_NAME_AS( Shebang,          "#!"                          );
		TOK_NAME_AS( Newline,          "\\n"                         );
		TOK_NAME_AS( Indentation,      "\\t"                         );
		TOK_NAME_AS( Whitespace,       "whitespace"                  );
		TOK_NAME_AS( OneLineComment,   "-- one line comment"         );
		TOK_NAME_AS( MultilineComment, "--[[ multiline comment --]]" );

		TOK_NAME_AS( Identifier, "Identifier" );
		TOK_NAME_AS( Integer,    "Integer"    );
		TOK_NAME_AS( Number,     "Number"     );
		TOK_NAME_AS( String,     "String"     );

		TOK_NAME_AS( Label,    "::"  );
		TOK_NAME_AS( Ellipsis, "..." );
		TOK_NAME_AS( Concat,   ".."  );
		TOK_NAME_AS( Dot,      "."   );

		TOK_NAME_AS( BracketStart, "[" );
		TOK_NAME_AS( BracketEnd,   "]" );
		TOK_NAME_AS( CurlyStart,   "{" );
		TOK_NAME_AS( CurlyEnd,     "}" );
		TOK_NAME_AS( ParenStart,   "(" );
		TOK_NAME_AS( ParenEnd,     ")" );

		TOK_NAME_AS( Eq,      "==" );
		TOK_NAME_AS( Neq,     "~=" );
		TOK_NAME_AS( Leq,     "<=" );
		TOK_NAME_AS( Geq,     ">=" );
		TOK_NAME_AS( Greater, ">"  );
		TOK_NAME_AS( Less,    "<"  );

		TOK_NAME_AS( AddAssign,    "+="  );
		TOK_NAME_AS( SubAssign,    "-="  );
		TOK_NAME_AS( MulAssign,    "*="  );
		TOK_NAME_AS( DivAssign,    "/="  );
		TOK_NAME_AS( AppendAssign, "#="  );
		TOK_NAME_AS( ConcatAssign, "..=" );

		TOK_NAME_AS( FunctionTypeArrow, "->" );
		TOK_NAME_AS( MapArrow,          "=>" );

		TOK_NAME_AS( Add, "+" );
		TOK_NAME_AS( Sub, "-" );
		TOK_NAME_AS( Mul, "*" );
		TOK_NAME_AS( Div, "/" );
		TOK_NAME_AS( Pow, "^" );
		TOK_NAME_AS( Mod, "%" );

		TOK_NAME_AS( Comma,        "," );
		TOK_NAME_AS( Semicolon,    ";" );
		TOK_NAME_AS( Hash,         "#" );
		TOK_NAME_AS( Colon,        ":" );
		TOK_NAME_AS( Assign,       "=" );
		TOK_NAME_AS( Questionmark, "?" );

		TOK_NAME_AS( Do,       "do"       );
		TOK_NAME_AS( If,       "if"       );
		TOK_NAME_AS( In,       "in"       );
		TOK_NAME_AS( Or,       "or"       );
		TOK_NAME_AS( And,      "and"      );
		TOK_NAME_AS( End,      "end"      );
		TOK_NAME_AS( For,      "for"      );
		TOK_NAME_AS( Nil,      "nil"      );
		TOK_NAME_AS( Not,      "not"      );
		TOK_NAME_AS( Var,      "var"      );
		TOK_NAME_AS( Else,     "else"     );
		TOK_NAME_AS( Goto,     "goto"     );
		TOK_NAME_AS( Then,     "then"     );
		TOK_NAME_AS( True,     "true"     );
		TOK_NAME_AS( Void,     "void"     );
		TOK_NAME_AS( Break,    "break"    );
		TOK_NAME_AS( False,    "false"    );
		TOK_NAME_AS( Local,    "local"    );
		TOK_NAME_AS( Until,    "until"    );
		TOK_NAME_AS( While,    "while"    );
		TOK_NAME_AS( Class,    "class"    );
		TOK_NAME_AS( Elseif,   "elseif"   );
		TOK_NAME_AS( Repeat,   "repeat"   );
		TOK_NAME_AS( Return,   "return"   );
		TOK_NAME_AS( Global,   "global"   );
		TOK_NAME_AS( Extern,   "extern"   );
		TOK_NAME_AS( Typedef,  "typedef"  );
		TOK_NAME_AS( Function, "function" );
	}

	const char* name = s_tok_name[(int)t];
	if (name) {
		return name;
	} else {
		return "ERROR: bad token";
	}
}


// -----------------------------------------------


Tokens::Tokens(const char* ptr, size_t num_bytes, bool sol) throw(lex_error)
: m_start(ptr)
{
	////////////////////////////////////////////////////
	// Build lookups:
	bool IDENT_STARTERS[256] = {0};
	IDENT_STARTERS['_'] = true;
	set_range(IDENT_STARTERS, 'a', 'z');
	set_range(IDENT_STARTERS, 'A', 'Z');

	bool IDENT_CHARS[256] = {0};
	IDENT_CHARS['_'] = true;
	set_range(IDENT_CHARS, 'a', 'z');
	set_range(IDENT_CHARS, 'A', 'Z');
	set_range(IDENT_CHARS, '0', '9');

	bool DIGITS[256] = {0};
	set_range(DIGITS, '0', '9');

	bool HEX_DIGITS[256] = {0};
	set_range(HEX_DIGITS, '0', '9');
	set_range(HEX_DIGITS, 'a', 'f');
	set_range(HEX_DIGITS, 'A', 'F');

	T ONE_LETTER_SYMBOLS[256] = {T::None};
	ONE_LETTER_SYMBOLS['+'] = T::Add;
	ONE_LETTER_SYMBOLS['-'] = T::Sub;
	ONE_LETTER_SYMBOLS['*'] = T::Mul;
	ONE_LETTER_SYMBOLS['/'] = T::Div;
	ONE_LETTER_SYMBOLS['^'] = T::Pow;
	ONE_LETTER_SYMBOLS['%'] = T::Mod;
	ONE_LETTER_SYMBOLS[','] = T::Comma;
	ONE_LETTER_SYMBOLS['{'] = T::CurlyStart;
	ONE_LETTER_SYMBOLS['}'] = T::CurlyEnd;
	ONE_LETTER_SYMBOLS['['] = T::BracketStart;
	ONE_LETTER_SYMBOLS[']'] = T::BracketEnd;
	ONE_LETTER_SYMBOLS['('] = T::ParenStart;
	ONE_LETTER_SYMBOLS[')'] = T::ParenEnd;
	ONE_LETTER_SYMBOLS[';'] = T::Semicolon;
	ONE_LETTER_SYMBOLS['#'] = T::Hash;
	ONE_LETTER_SYMBOLS[':'] = T::Colon;
	ONE_LETTER_SYMBOLS['>'] = T::Greater;
	ONE_LETTER_SYMBOLS['<'] = T::Less;
	ONE_LETTER_SYMBOLS['='] = T::Assign;
	ONE_LETTER_SYMBOLS['?'] = T::Questionmark;

	////////////////////////////////////////////////////
	// Estimating the number of tokens we are gonna need
	// saves about 20% time.

	// Pretty good estimates
	m_whites.reserve(num_bytes / 5);
	m_tokens.reserve(num_bytes / 6);
	m_white_ix.reserve(m_tokens.capacity());

	////////////////////////////////////////////////////


	unsigned    token_line  = 1;    // line the token started at
	const char* token_begin = ptr;  // where the token started
	const char* line_start  = ptr;  // where the line started

#define OUTPUT(where, type) where.push_back( Token \
{token_begin, ptr, token_line, 1 + unsigned(token_begin-line_start), type}); \
token_begin = ptr

#define ADD_WHITE(type) OUTPUT(m_whites, type)
#define ADD_TOKEN(type) OUTPUT(m_tokens, type); m_white_ix.push_back(m_whites.size());

#define TOKEN_ASSERT(expr, what) if (!(expr)) { throw lex_error(token_line, unsigned(ptr-line_start), what); }

	/*
	 If there is a long string here,
	 it will be skipped, and a token will be added.
	 Else, returns false and nothing changes.


	 --[==[
	 There can be any number of '=', including none.
	 --]==]
	 */
	auto try_get_long_string = [&](std::vector<Token>& where, T type)
	{
		const char* start = ptr;

		if (ptr[0] == '[') {
			++ptr;

			unsigned start_equals_count = 0;
			while (*ptr == '=') { ++ptr; ++start_equals_count; }

			if (ptr[0] == '[') {
				// Yes, a multiline comment

				++ptr;

				unsigned ml_line = token_line;
				const char* last_line_start = line_start;

				for (;;) {
					TOKEN_ASSERT(*ptr, "Multiline comment with no end")

					if (*ptr == ']') {
						// Check for ==]
						++ptr;
						unsigned end_equals_count = 0;
						while (*ptr == '=') { ++ptr; ++end_equals_count; }
						if (start_equals_count == end_equals_count && *ptr == ']') {
							++ptr;
							break;
						}
					}
					else if (*ptr=='\n') {
						++ptr;
						ml_line += 1;
						last_line_start = ptr;
					} else {
						++ptr;
					}
				}
				OUTPUT(where, type);
				token_line = ml_line;
				line_start = last_line_start;
				return true;
			}
		}

		// Restore:
		ptr = start;
		return false;
	};


	////////////////////////////////////////////////


	ADD_TOKEN(T::Bof);  // m_white_ix[0] == 0

	if (ptr[0]=='#' && ptr[1]=='!') {
		ptr += 2;
		while (*ptr && *ptr != '\n' && *ptr != '\r') {
			++ptr;
		}
		ADD_WHITE(T::Shebang);
	}

	while (*ptr) {
		assert(ptr == token_begin);

		if (*ptr == '\n') {
			// Unix style newline
			ptr += 1;
			ADD_WHITE(T::Newline);
			token_line += 1;
			line_start = ptr;

			while (*ptr == '\t') { ++ptr; }
			if (ptr != token_begin) {
				ADD_WHITE(T::Indentation);
			}
		}
		else if (*ptr == '\r') {
			// CR-LF - windows style newline
			TOKEN_ASSERT(ptr[1] == '\n', "CR with no LF")
			ptr += 2;
			ADD_WHITE(T::Newline);
			token_line += 1;
			line_start = ptr;

			while (*ptr == '\t') { ++ptr; }
			if (ptr != token_begin) {
				ADD_WHITE(T::Indentation);
			}
		}
		else if (*ptr == ' ' || *ptr == '\t') {
			do {
				++ptr;
			} while (*ptr == ' ' || *ptr == '\t');
			ADD_WHITE(T::Whitespace);
		}
		else if (ptr[0] == '-' && ptr[1] == '-') {
			ptr += 2;

			if (try_get_long_string(m_whites, T::MultilineComment)) {
				// Ok then
			} else {
				// Single line comment:
				while (*ptr && *ptr != '\n' && *ptr != '\r') {
					++ptr;
				}
				ADD_WHITE(T::OneLineComment);
			}
		}

		// Non-white:
		else if (IDENT_STARTERS[(uchar)*ptr]) {
			++ptr;
			while (IDENT_CHARS[(uchar)*ptr]) {
				++ptr;
			}
			ADD_TOKEN( get_keyword(sol, token_begin, ptr) );
		}

		else if (ptr[0]=='0' && ptr[1]=='x') {
			ptr += 2;
			bool integer = true;
			while ( HEX_DIGITS[(uchar)ptr[0]] ) { ++ptr; }
			if (ptr[0]=='p' || ptr[1]=='P') {
				++ptr;
				if (*ptr=='-' || *ptr=='+') { ++ptr; }
				while ( DIGITS[(uchar)ptr[0]] ) { ++ptr; }
				integer = false;
			}
			if (integer) {
				ADD_TOKEN(T::Integer);
			} else {
				ADD_TOKEN(T::Number);
			}
		}
		else if (DIGITS[(uchar)ptr[0]] || (ptr[0] == '.' && DIGITS[(uchar)ptr[1]])) {
			bool integer = true;
			while ( DIGITS[(uchar)ptr[0]] ) { ++ptr; }
			if (*ptr == '.') {
				++ptr;
				while ( DIGITS[(uchar)ptr[0]] ) { ++ptr; }
				integer = false;
			}
			if (*ptr == 'e' || *ptr == 'E') {
				++ptr;
				if (*ptr=='-' || *ptr=='+') { ++ptr; }
				while ( DIGITS[(uchar)ptr[0]] ) { ++ptr; }
				integer = false;
			}
			if (integer) {
				ADD_TOKEN(T::Integer);
			} else {
				ADD_TOKEN(T::Number);
			}
		}

		else if (*ptr == '\"' || *ptr == '\'') {
			char delim = *ptr;
			++ptr;
			while (*ptr != delim) {
				TOKEN_ASSERT(*ptr, "String with no end")
				TOKEN_ASSERT(*ptr != '\n', "Newline in string");
				if (*ptr == '\\') {
					++ptr;
					TOKEN_ASSERT(*ptr, "String does not escape anything")
					++ptr;
				} else {
					++ptr;
				}
			}
			++ptr;
			ADD_TOKEN(T::String);
		}

		else if (*ptr == '[') {
			if (try_get_long_string(m_tokens, T::String)) {
				// Ok then
				m_white_ix.push_back(m_whites.size()); // Must be done manually, since OUTPUT macro doesn't do this.
			} else {
				++ptr;
				ADD_TOKEN(T::BracketStart); // [
			}
		}

		else if (*ptr == '.') {
			++ptr;
			if (*ptr == '.') {
				++ptr;
				if (*ptr == '.') {
					++ptr;
					ADD_TOKEN(T::Ellipsis); // ...
				} else if (*ptr == '=') {
					++ptr;
					ADD_TOKEN(T::ConcatAssign); // ..=
				} else {
					ADD_TOKEN(T::Concat); // ..
				}
			} else {
				ADD_TOKEN(T::Dot);    // .
			}
		}

#define CHECK_SYMBOL_2(a, b, symbol)            \
		else if (ptr[0] == a && ptr[1] == b) {  \
			ptr += 2;                           \
			ADD_TOKEN(symbol);                  \
		}

		CHECK_SYMBOL_2('=', '=', T::Eq)
		CHECK_SYMBOL_2('~', '=', T::Neq)
		CHECK_SYMBOL_2('<', '=', T::Leq)
		CHECK_SYMBOL_2('>', '=', T::Geq)

		CHECK_SYMBOL_2('+', '=', T::AddAssign)
		CHECK_SYMBOL_2('-', '=', T::SubAssign)
		CHECK_SYMBOL_2('*', '=', T::MulAssign)
		CHECK_SYMBOL_2('/', '=', T::DivAssign)
		CHECK_SYMBOL_2('#', '=', T::AppendAssign)

		CHECK_SYMBOL_2(':', ':', T::Label)
		CHECK_SYMBOL_2('-', '>', T::FunctionTypeArrow)
		CHECK_SYMBOL_2('=', '>', T::MapArrow)


		// One-letter symbols:
		else
		{
			T t = ONE_LETTER_SYMBOLS[(uchar)ptr[0]];
			TOKEN_ASSERT(t != T::None, "Unexpected symbol");
			ptr += 1;
			ADD_TOKEN(t);
		}
	} // While

	m_end = ptr;
	ADD_TOKEN(T::Eof);


	///////////////////////////////////////////////

	m_it = m_tokens.begin();
}
