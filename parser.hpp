#pragma once

#include "lexer.hpp"
#include "ast.hpp"


class Parser
{
public:
	explicit Parser(Tokens& tokens);
	
	const StatList* root() const { return m_root; }
	StatList* root() { return m_root; }
	
protected:
	enum SuffixStyle { Anything, OnlyDotColon };
	enum ArgTypes { MustHaveNames, MustHaveTypes };
	
	StatList*    parse_stat_list();
	Statement*   parse_statement();
	VarDeclStat* parse_declaration(TokList* tl, Scoping scoping);
	FunDeclStat* parse_fun_decl(TokList* tl, Scoping scoping);
	TypedefStat* parse_typedef(TokList* tl, Scoping scoping);
	ClassStat*   parse_class(TokList* tl, Scoping scoping);
	
	TypeList*    parse_type_list();
	TypeNode*    parse_type();
	TypeNode*    parse_simple_type();
	FunType*     parse_fun_type(TokList* tl, ArgTypes arg_types);
	
	Expression*  parse_expr(int prio_level = 0);
	Expression*  parse_simple_expr();
	Expression*  parse_suffixed_expr(SuffixStyle style = Anything);
	Expression*  parse_primary_expr();
	Expression*  parse_id_expr();
	
private:
	inline TokList* alloc_tok_list() {
		return m_tok_alloc.make<TokList>(m_tok_alloc);
	}
	
	inline StatList* alloc_stat_list() {
		return m_stat_alloc.make<StatList>(m_stat_alloc);
	}
	
	inline TypeList* alloc_type_list(TokList* tl) {
		return m_type_alloc.make<TypeList>(tl, m_type_alloc);
	}
	
	template<class NodeType>
	inline NodeType* alloc_stat(TokList* tl) {
		return m_stat_alloc.make<NodeType>(tl, m_stat_alloc);
	}
	
	template<class NodeType>
	inline NodeType* alloc_expr(TokList* tl) {
		return m_expr_alloc.make<NodeType>(tl, m_expr_alloc);
	}
	
	template<class NodeType>
	inline NodeType* alloc_type(TokList* tl) {
		return m_type_alloc.make<NodeType>(tl, m_type_alloc);
	}
	
	void throw_error(std::string msg) {
		auto t = m_tok.peek_token();
		throw parse_error(t->line, t->col, msg);
	}
	
	inline void p_assert(bool b, const char* fail_msg) {
		if (!b) {
			throw_error(fail_msg);
		}
	}
	
private:
	// Precalculated lookups:
	struct prio { byte a=0,b=0; prio(){} prio(byte a_, byte b_) : a(a_), b(b_) {} };
	prio   PRIORITY[ (int)T::NUM_TYPES ];
	bool   STAT_LIST_CLOSE_KEYWORDS[ (int)T::NUM_TYPES ];
	T      ASSIGN_OPS[ (int)T::NUM_TYPES ];
	
	
	Tokens& m_tok;
		
	Allocator m_tok_alloc; // For TokenLists:s.
	Allocator m_stat_alloc;
	Allocator m_expr_alloc;
	Allocator m_type_alloc;
	
	StatList* m_root;
};
