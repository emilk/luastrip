#pragma once

#include "ast.hpp"
#include "ast_visitor.hpp"

class Stripper : public AstVisitor
{
	typedef AstVisitor base;
	
public:
	inline bool is(const Token* tok, const char* name)
	{
		return strncmp(tok->str_begin, name, tok->str_end - tok->str_begin) == 0;
	}
	
	inline bool is(const Expression* expr, const char* name)
	{
		if (auto id = dynamic_cast<const IdExpr*>(expr)) {
			return is(id->ident, name);
		} else {
			return false;
		}
	}
	
	Action visit_stat(Statement* arg) override
	{
		if (auto call_stat = dynamic_cast<CallStat*>(arg)) {
			CallExpr* call_expr = call_stat->expr;
			Expression* base_expr = call_expr->base;
			
			if (auto member = dynamic_cast<MemberExpr*>(base_expr)) {
				if (is(member->base, "Log") || is(member->base, "Debug")) {
					return Remove;
				}
			} else if (is(base_expr, "assert")) {
				return Remove;
			}
		}
		
		return base::visit_stat(arg);
	}
};
