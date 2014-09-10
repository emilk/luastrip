#pragma once

#include "ast.hpp"

class AstVisitor
{
public:
	unsigned num_erased_stats() const {
		return m_num_erased_stats;
	}
	
	
	enum Action {
		Nothing,
		Remove
	};
	
	virtual void stat_list(StatList* node)
	{
		/*
		for (auto&& s : node->stats) {
			stat(s);
		}*/
		auto it = std::remove_if(begin(node->stats), end(node->stats), [&](Statement* node) {
			auto action = visit_stat(node);
			return action == Remove;
		});
		m_num_erased_stats += end(node->stats) - it;
		node->stats.erase(it, end(node->stats));
	}
	
	virtual Action visit_stat(Statement* arg)
	{
		if (arg->synthetic) {
#define BreakIntoDebugger() __asm__("int $3\n" : : )
			//BreakIntoDebugger();
		}
		
		// Ordered by popularity
		if (auto node = dynamic_cast<IfStat*>(arg)) {
			for (auto&& clause : node->clauses) {
				if (clause.cond) {
					expr(clause.cond);
				}
				
				stat_list(clause.body);
			}
		} else if (auto node = dynamic_cast<VarDeclStat*>(arg)) {
			if (!node->init_list.empty()) {
				expr_list(node->init_list);
			}
		} else if (auto node = dynamic_cast<CallStat*>(arg)) {
			expr(node->expr);
		} else if (auto node = dynamic_cast<ReturnStat*>(arg)) {
			expr_list(node->arguments);
		} else if (auto node = dynamic_cast<AssignmentStat*>(arg)) {
			expr_list(node->lhs);
			expr_list(node->rhs);
		}
		else if (auto node = dynamic_cast<FunDeclStat*>(arg)) {
			expr(node->name_expr);
			fun_type(node->type);
			stat_list(node->body);
		}
		else if (auto node = dynamic_cast<GenericForStat*>(arg)) {
			tok_list(node->var_names);
			expr_list(node->generators);
			stat_list(node->body);
		}
		else if (auto node = dynamic_cast<NumericForStat*>(arg)) {
			expr(node->begin);
			expr(node->end);
			if (node->step) {
				expr(node->step);
			}
			stat_list(node->body);
		}
		else if (auto node = dynamic_cast<WhileStat*>(arg)) {
			expr(node->cond);
			stat_list(node->body);
		}
		else if (dynamic_cast<BreakStat*>(arg)) {
			
		}
		else if (auto node = dynamic_cast<RepeatUntilStat*>(arg)) {
			stat_list(node->body);
			expr(node->cond);
		}
		else if (auto node = dynamic_cast<DoStat*>(arg)) {
			stat_list(node->body);
		}
		else if (auto node = dynamic_cast<const LabelStat*>(arg)) {

		}
		else if (auto node = dynamic_cast<const GotoStat*>(arg)) {

		}
		else if (dynamic_cast<TypedefStat*>(arg)) {
			// Ignored
		}
		else if (auto node = dynamic_cast<ClassStat*>(arg)) {
			expr(node->rhs);
		}
		//else if (dynamic_cast<ClassDeclStat*>(arg)) {
		//}
		else if (dynamic_cast<EOFStat*>(arg)) {
			//
		}
		else {
			throw std::runtime_error("Uknown statement: "+ std::string(typeid(*arg).name()));
		}
		
		return Nothing;
	}
	
	void expr(Expression* arg)
	{
		if (dynamic_cast<IdExpr*>(arg)) {
			
		}
		
		else if (auto node = dynamic_cast<MemberExprDot*>(arg)) {
			expr(node->base);
		}
		
		else if (auto node = dynamic_cast<MemberExprColon*>(arg)) {
			expr(node->base);
		}
		
		else if (dynamic_cast<RawTokenExpr*>(arg)) {
			// Number, String, True, False etc
		}
		
		else if (auto node = dynamic_cast<ParenCallExpr*>(arg)) {
			expr(node->base);
			expr_list(node->args);
		}
		else if (auto node = dynamic_cast<CallExpr*>(arg)) {
			// Table or string call
			expr(node->base);
			assert(node->args.size() == 1);
			expr(node->args[0]);
		}
		
		else if (auto node = dynamic_cast<BinopExpr*>(arg)) {
			expr(node->lhs);
			expr(node->rhs);
		}
		
		else if (auto node = dynamic_cast<UnopExpr*>(arg)) {
			expr(node->rhs);
		}
		
		else if (auto node = dynamic_cast<IndexExpr*>(arg)) {
			expr(node->base);
			expr(node->index);
		}
		
		else if (auto node = dynamic_cast<ConstructorExpr*>(arg)) {
			for (auto&& entry : node->entries) {
				if (entry.key_ident) {
					
				} else if (entry.key_expr) {
					expr(entry.key_expr);
				}
				expr(entry.value);
			}
		}
		
		else if (auto node = dynamic_cast<ParenExpr*>(arg)) {
			expr(node->inner);
		}
		
		else if (auto node = dynamic_cast<CastExpr*>(arg)) {
			expr(node->expr);
		}
		
		else if (auto node = dynamic_cast<LambdaExpr*>(arg)) {
			fun_type(node->type);
			stat_list(node->body);
		}

		else {
			throw std::runtime_error("Uknown expression: "+ std::string(typeid(*arg).name()));
		}
	}
	
	inline void expr_list(ExprList& list)
	{
		for (auto&& e : list) {
			expr(e);
		}
	}
	
	inline void tok_list(TokList& list)
	{
	}
	
	void fun_type(FunType* type)
	{
	}
	
private:
	unsigned m_num_erased_stats = 0;
};

