#pragma once

#include "ast.hpp"
#include "ast_visitor.hpp"

typedef std::vector<std::string> StringList;

class Stripper : public AstVisitor
{
	typedef AstVisitor base;
	
public:
	/*
	foo.bar(...) -- stripped if 'foo' is in call_bases
	baz(...)     -- stripped if 'baz' is in function_names
	*/
	Stripper(const StringList& call_bases, const StringList& function_names) : _call_bases(call_bases), _function_names(function_names) {

	}

	inline bool is(const Token* tok, const std::string& name)
	{
		auto n = tok->str_end - tok->str_begin;
		return name.size() == n && strncmp(name.c_str(), tok->str_begin, n) == 0;
	}
	
	inline bool is_any_of(const Expression* expr, const StringList& names)
	{
		if (auto id = dynamic_cast<const IdExpr*>(expr)) {
			for (auto&& name : names) {
				if (is(id->ident, name)) {
					return true;
				}
			}
		}
		return false;
	}
	
	Action visit_stat(Statement* arg) override
	{
		if (auto call_stat = dynamic_cast<CallStat*>(arg)) {
			CallExpr* call_expr = call_stat->expr;
			Expression* base_expr = call_expr->base;
			
			if (auto member = dynamic_cast<MemberExpr*>(base_expr)) {
				if (is_any_of(member->base, _call_bases)) {
					return Remove;
				}
			} else {
				if (is_any_of(base_expr, _function_names)) {
					return Remove;
				}
			}
		}
		
		return base::visit_stat(arg);
	}

private:
	StringList _call_bases;
	StringList _function_names;
};
