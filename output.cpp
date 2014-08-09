#include "output.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include <cassert>


/*
 Token popularity:
 'Symbol'  : 17409 41.9 %
 'Ident'   : 14409 34.7 %
 'Keyword' :  7318 17.6 %
 'String'  :  1998  4.8 %
 'Number'  :   390  0.9 %
 'Eof'     :    15  0.0 %

 Symbol popularity:
 '.'    : 2803 16.1 %
 ','    : 2467 14.2 %
 ')'    : 2363 13.6 %
 '('    : 2363 13.6 %
 '='    : 1929 11.1 %
 ':'    : 1318  7.6 %
 '=='   :  614  3.5 %
 '{'    :  481  2.8 %
 '}'    :  481  2.8 %
 ']'    :  476  2.7 %
 '['    :  476  2.7 %
 ';'    :  400  2.3 %
 '->'   :  247  1.4 %
 '?'    :  187  1.1 %
 '#'    :  160  0.9 %
 '..'   :  149  0.9 %
 '...'  :  106  0.6 %
 '~='   :   94  0.5 %
 '#='   :   81  0.5 %
 '+='   :   40  0.2 %
 '..='  :   33  0.2 %
 '-'    :   33  0.2 %
 '+'    :   29  0.2 %
 '=>'   :   22  0.1 %
 '>'    :   19  0.1 %
 '<'    :   16  0.1 %
 '<='   :   10  0.1 %
 '*'    :    5  0.0 %
 '/'    :    3  0.0 %
 '>='   :    3  0.0 %
 '-='   :    1  0.0 %
 
 Keyword popularity:
 'end'      : 1249 17.1 %
 'then'     : 1072 14.7 %
 'if'       :  804 11.0 %
 'return'   :  710  9.7 %
 'local'    :  575  7.9 %
 'function' :  323  4.4 %
 'var'      :  288  3.9 %
 'elseif'   :  268  3.7 %
 'or'       :  257  3.5 %
 'not'      :  252  3.4 %
 'else'     :  227  3.1 %
 'false'    :  204  2.8 %
 'and'      :  187  2.6 %
 'do'       :  166  2.3 %
 'true'     :  151  2.1 %
 'nil'      :  144  2.0 %
 'for'      :  132  1.8 %
 'in'       :   94  1.3 %
 'typedef'  :   82  1.1 %
 'extern'   :   41  0.6 %
 'while'    :   34  0.5 %
 'break'    :   23  0.3 %
 'global'   :   19  0.3 %
 'until'    :    7  0.1 %
 'repeat'   :    7  0.1 %
 'class'    :    1  0.0 %

 Ast type popularity:
 'IdExpr'                : 7720 31.7 %
 'MemberExpr'            : 2807 11.5 %
 'CallExpr'              : 1997  8.2 %
 'StringExpr'            : 1854  7.6 %
 'Statlist'              : 1765  7.2 %
 'BinopExpr'             : 1451  6.0 %
 'IfStatement'           :  803  3.3 %
 'VarDeclareStatement'   :  772  3.2 %
 'CallStatement'         :  749  3.1 %
 'ReturnStatement'       :  710  2.9 %
 'AssignmentStatement'   :  645  2.6 %
 'UnopExpr'              :  506  2.1 %
 'NumberExpr'            :  471  1.9 %
 'IndexExpr'             :  401  1.6 %
 'ConstructorExpr'       :  365  1.5 %
 'BooleanExpr'           :  350  1.4 %
 'FunctionDeclStatement' :  233  1.0 %
 'NilExpr'               :  131  0.5 %
 'ParenthesesExpr'       :  115  0.5 %
 'GenericForStatement'   :   95  0.4 %
 'CastExpr'              :   90  0.4 %
 'Typedef'               :   82  0.3 %
 'StringCallExpr'        :   55  0.2 %
 'LambdaFunctionExpr'    :   47  0.2 %
 'NumericForStatement'   :   38  0.2 %
 'WhileStatement'        :   34  0.1 %
 'DotsExpr'              :   34  0.1 %
 'BreakStatement'        :   23  0.1 %
 'TableCallExpr'         :   18  0.1 %
 'Eof'                   :   14  0.1 %
 'RepeatStatement'       :    7  0.0 %
 'ClassDeclStatement'    :    1  0.0 %
 */


class TokenFormater
{
public:
	inline TokenFormater(const ASTNode* node, CharVec& out, size_t& printed_whites_, const Tokens& tok)
	: tl(*node->tokens), synthetic(node->synthetic), buff(out), printed_whites(printed_whites_), m_tok(tok)
	{
	}

	inline ~TokenFormater()
	{
		if (!synthetic) {
			assert(ix == tl.size());
		}
	}
	
	inline size_t num_left() const {
		return tl.size() - ix;
	}
	
	inline void consume_rest()
	{
		ix = tl.size();
	}
	
	inline void append_raw(const char* str_begin, const char* str_end)
	{
		buff.append(str_begin, str_end);
	}
	
	inline void append_raw(const Token* t)
	{
		buff.append(t->str_begin, t->str_end);
	}
	
	inline void append_synthetic_ws()
	{
		const char* DEFAULT_SPACING = " ";
		buff.append(DEFAULT_SPACING, DEFAULT_SPACING + 1);
	}

	inline void append_white(const Token* t)
	{
		if (t->line == 0) {
			// Synthetic token
			append_synthetic_ws();
			return;
		}
		
		size_t white_index = m_tok.white_index(t);
		
		assert(white_index==0 || t->type!=T::Bof);
		
		if (white_index <= printed_whites) {
			if (synthetic) {
				append_synthetic_ws();
			}
		} else {
			while (printed_whites < white_index) {
				auto w = m_tok.get_white(printed_whites);
				append_raw(w);
				printed_whites += 1;
			}
		}
	}

	inline void put(const Token* t)
	{
		if (synthetic) {
			append_white(t);
			buff.append(t->str_begin, t->str_end);
		} else {
			if (ix < tl.size()) {
				assert(tl[ix] == t);
				++ix;
			}
			append_white(t);
			buff.append(t->str_begin, t->str_end);
		}
	}
	
	inline void put(T t)
	{
		if (synthetic) {
			append_synthetic_ws();
			const char* str = tok_name(t);
			append_raw(str, str + strlen(str));
		} else {
			assert(ix < tl.size());
			assert(tl[ix]->type == t);
			put_next_token();
		}
	}
	
	inline void put(Scoping scoping)
	{
		if (scoping == Scoping::Local) {
			put(T::Local);
		} else if (scoping == Scoping::Global) {
			skip(T::Global);
		} else if (scoping == Scoping::Var) {
			replace_token("local");
		}
	}

	inline void put_next_token()
	{
		assert(ix < tl.size());
		put(tl[ix]);
	}
	
	inline void skip(T t)
	{
		assert(ix < tl.size());
		assert(tl[ix]->type == t);
		append_white(tl[ix]);
		++ix;
	}
	
	inline void replace_token(const char* str)
	{
		assert(ix < tl.size());
		append_white(tl[ix]);
		++ix;
		append_raw(str, str + strlen(str));
	}
	
	inline void put_optional_comma_semicolon()
	{
		assert(ix < tl.size());
		if (tl[ix]->type == T::Comma ||
			 tl[ix]->type == T::Semicolon)
		{
			put_next_token();
		}
	}
	
	inline void put_optional_semicolon()
	{
		if (ix < tl.size() && tl[ix]->type == T::Semicolon)
		{
			put_next_token();
		}
	}

private:
	const TokList& tl;
	bool           synthetic;
	CharVec&       buff;
	size_t&        printed_whites;
	size_t         ix = 0;
	const Tokens&  m_tok;
};


class Output
{
public:
	Output(const Tokens& t, const Parser& p)
	: tok(t), parser(p)
	{
		buff.reserve(tok.input_size());
	}

	void stat_list(const StatList* node)
	{
		for (auto&& s : node->stats) {
			stat(s);
		}
	}

	void stat(const Statement* arg)
	{
		if (arg->synthetic) {
#define BreakIntoDebugger() __asm__("int $3\n" : : )
			//BreakIntoDebugger();
		}
		
		TokenFormater tf(arg, buff, printed_whites, tok);
		
		// Ordered by popularity
		if (auto node = dynamic_cast<const IfStat*>(arg)) {
			auto ix = 0;
			for (auto&& clause : node->clauses) {
				if (clause.cond) {
					if (ix == 0) {
						tf.put(T::If);
					} else {
						tf.put(T::Elseif);
					}
					expr(clause.cond);
					tf.put(T::Then);
				} else {
					tf.put(T::Else);
				}
			
				stat_list(clause.body);
				++ix;
			}
			tf.put(T::End);
		}
		
		else if (auto node = dynamic_cast<const VarDeclStat*>(arg)) {
			tf.put(node->scoping);
			tok_list(tf, node->name_list);
			if (!node->init_list.empty()) {
				tf.put(T::Assign);
				expr_list(tf, node->init_list);
			}
		}
		
		else if (auto node = dynamic_cast<const CallStat*>(arg)) {
			expr(node->expr);
		}
		
		else if (auto node = dynamic_cast<const ReturnStat*>(arg)) {
			tf.put(T::Return);
			expr_list(tf, node->arguments);
		}
		
		else if (auto node = dynamic_cast<const AssignmentStat*>(arg)) {
			expr_list(tf, node->lhs);
			tf.put(T::Assign);
			expr_list(tf, node->rhs);
		}
		else if (auto node = dynamic_cast<const FunDeclStat*>(arg)) {
			tf.put(node->scoping);
			tf.put(T::Function);
			expr(node->name_expr);
			fun_type(node->type);
			stat_list(node->body);
			tf.put(T::End);
		}
		else if (auto node = dynamic_cast<const GenericForStat*>(arg)) {
			tf.put(T::For);
			tok_list(tf, node->var_names);
			tf.put(T::In);
			expr_list(tf, node->generators);
			tf.put(T::Do);
			stat_list(node->body);
			tf.put(T::End);
		}
		else if (auto node = dynamic_cast<const NumericForStat*>(arg)) {
			tf.put(T::For);
			tf.put(node->it_name);
			tf.put(T::Assign);
			
			expr(node->begin);
			tf.put(T::Comma);
			expr(node->end);
			if (node->step) {
				tf.put(T::Comma);
				expr(node->step);
			}
			
			tf.put(T::Do);
			stat_list(node->body);
			tf.put(T::End);
		}
		else if (auto node = dynamic_cast<const WhileStat*>(arg)) {
			tf.put(T::While);
			expr(node->cond);
			tf.put(T::Do);
			stat_list(node->body);
			tf.put(T::End);
		}
		else if (dynamic_cast<const BreakStat*>(arg)) {
			tf.put(T::Break);
		}
		else if (auto node = dynamic_cast<const RepeatUntilStat*>(arg)) {
			tf.put(T::Repeat);
			stat_list(node->body);
			tf.put(T::Until);
			expr(node->cond);
		}
		else if (auto node = dynamic_cast<const DoStat*>(arg)) {
			tf.put(T::Do);
			stat_list(node->body);
			tf.put(T::End);
		}
		else if (dynamic_cast<const TypedefStat*>(arg)) {
			// Ignored
			tf.consume_rest();
		}
		else if (auto node = dynamic_cast<const ClassStat*>(arg)) {
			tf.put(node->scoping);
			tf.skip(T::Class);
			tf.put(node->name);
			tf.put(T::Assign);
			expr(node->rhs);
		}
		//else if (dynamic_cast<const ClassDeclStat*>(arg)) {
		//}
		else if (dynamic_cast<const EOFStat*>(arg)) {
			//tf.append_white((*arg->tokens)[0]);
			tf.skip(T::Eof);
		}
		else {
			assert(false);
		}
		
		tf.put_optional_semicolon();
	}

	void expr(const Expression* arg)
	{
		TokenFormater tf(arg, buff, printed_whites, tok);

		if (auto node = dynamic_cast<const IdExpr*>(arg)) {
			tf.put( node->ident );
		}
		
		else if (auto node = dynamic_cast<const MemberExprDot*>(arg)) {
			expr(node->base);
			tf.put(T::Dot);
			tf.put(node->ident);
		}
		
		else if (auto node = dynamic_cast<const MemberExprColon*>(arg)) {
			expr(node->base);
			tf.put(T::Colon);
			tf.put(node->ident);
		}
		
		else if (dynamic_cast<const RawTokenExpr*>(arg)) {
			// Number, String, True, False etc
			tf.put_next_token();
		}
		
		else if (auto node = dynamic_cast<const ParenCallExpr*>(arg)) {
			expr(node->base);
			tf.put(T::ParenStart);
			expr_list(tf, node->args);
			tf.put(T::ParenEnd);
		}
		else if (auto node = dynamic_cast<const CallExpr*>(arg)) {
			// Table or string call
			expr(node->base);
			assert(node->args.size() == 1);
			expr(node->args[0]);
		}
		
		else if (auto node = dynamic_cast<const BinopExpr*>(arg)) {
			expr(node->lhs);
			tf.put(node->op);
			expr(node->rhs);
		}
		
		else if (auto node = dynamic_cast<const UnopExpr*>(arg)) {
			tf.put(node->op);
			expr(node->rhs);
		}
		
		else if (auto node = dynamic_cast<const IndexExpr*>(arg)) {
			expr(node->base);
			tf.put(T::BracketStart);
			expr(node->index);
			tf.put(T::BracketEnd);
		}
		
		else if (auto node = dynamic_cast<const ConstructorExpr*>(arg)) {
			tf.put(T::CurlyStart);
			for (auto&& entry : node->entries) {
				if (entry.key_ident) {
					tf.put(entry.key_ident);
					tf.put(T::Assign);
				} else if (entry.key_expr) {
					tf.put(T::BracketStart);
					expr(entry.key_expr);
					tf.put(T::BracketEnd);
					tf.put(T::Assign);
				}
				expr(entry.value);
				tf.put_optional_comma_semicolon();
			}
			tf.put(T::CurlyEnd);
		}
		
		else if (auto node = dynamic_cast<const ParenExpr*>(arg)) {
			tf.put(T::ParenStart);
			expr(node->inner);
			tf.put(T::ParenEnd);
		}
		
		else if (auto node = dynamic_cast<const CastExpr*>(arg)) {
			expr(node->expr);
			tf.consume_rest();
		}
		
		else if (auto node = dynamic_cast<const LambdaExpr*>(arg)) {
			tf.put(T::Function);
			fun_type(node->type);
			stat_list(node->body);
			tf.put(T::End);
		}
		
		else {
			assert(false);
		}
	}

	inline void expr_list(TokenFormater& tf, const ExprList& list)
	{
		auto ix = 0;
		for (auto&& e : list) {
			expr(e);
			++ix;
			if (ix != list.size()) {
				tf.put(T::Comma);
			}
		}
	}
	
	inline void tok_list(TokenFormater& tf, const TokList& list)
	{
		auto ix = 0;
		for (auto&& t : list) {
			tf.put(t);
			++ix;
			if (ix != list.size()) {
				tf.put(T::Comma);
			}
		}
	}
	
	void fun_type(TokenFormater& tf, const FunType* type)
	{
		tf.put(T::ParenStart);
		auto ix = 0;
		for (auto&& arg : type->args) {
			assert(arg.name);
			tf.put(arg.name);
			++ix;
			if (arg.type) {
				tf.skip(T::Colon);
			}
			if (ix != type->args.size()) {
				tf.put(T::Comma);
			}
		}
		tf.put(T::ParenEnd);
		tf.consume_rest();  // Type etc
	}
	
	inline void fun_type(const FunType* type)
	{
		TokenFormater tf(type, buff, printed_whites, tok);
		fun_type(tf, type);
	}


	const Tokens& tok;
	const Parser& parser;
	CharVec       buff;
	size_t        printed_whites = 0;
};

CharVec output(const Tokens& tok, const Parser& p)
{
	Output op(tok,p);
	op.stat_list(p.root());
	return std::move( op.buff );
}
