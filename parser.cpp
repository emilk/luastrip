#include "parser.hpp"


const size_t TOKENS_PER_NODE = 8;  // Expected
const auto UNOP_PRIO = 8;


Parser::Parser(Tokens& tokens) : m_tok(tokens),
// Sized based on what's used by parser.sol
m_tok_alloc (tokens.num_tokens() * 46),
m_stat_alloc(tokens.num_tokens() * 11),
m_expr_alloc(tokens.num_tokens() * 15),
m_type_alloc(tokens.num_tokens() *  3)
{
	memset(STAT_LIST_CLOSE_KEYWORDS, 0, sizeof(STAT_LIST_CLOSE_KEYWORDS));
	STAT_LIST_CLOSE_KEYWORDS[(int)T::End]    = true;
	STAT_LIST_CLOSE_KEYWORDS[(int)T::Else]   = true;
	STAT_LIST_CLOSE_KEYWORDS[(int)T::Elseif] = true;
	STAT_LIST_CLOSE_KEYWORDS[(int)T::Until]  = true;
	STAT_LIST_CLOSE_KEYWORDS[(int)T::Eof]    = true;
	
	memset(ASSIGN_OPS, 0, sizeof(ASSIGN_OPS));
	ASSIGN_OPS[(int)T::AddAssign]    = T::Add;
	ASSIGN_OPS[(int)T::SubAssign]    = T::Sub;
	ASSIGN_OPS[(int)T::MulAssign]    = T::Mul;
	ASSIGN_OPS[(int)T::DivAssign]    = T::Div;
	ASSIGN_OPS[(int)T::ConcatAssign] = T::Concat;
	
	memset(PRIORITY, 0, sizeof(PRIORITY));
	PRIORITY[(int)T::Add]     = prio{ 6, 6};
	PRIORITY[(int)T::Sub]     = prio{ 6, 6};
	PRIORITY[(int)T::Mod]     = prio{ 7, 7};
	PRIORITY[(int)T::Div]     = prio{ 7, 7};
	PRIORITY[(int)T::Mul]     = prio{ 7, 7};
	PRIORITY[(int)T::Pow]     = prio{10, 9};
	PRIORITY[(int)T::Concat]  = prio{ 5, 4};
	PRIORITY[(int)T::Eq]      = prio{ 3, 3};
	PRIORITY[(int)T::Less]    = prio{ 3, 3};
	PRIORITY[(int)T::Leq]     = prio{ 3, 3};
	PRIORITY[(int)T::Neq]     = prio{ 3, 3};
	PRIORITY[(int)T::Greater] = prio{ 3, 3};
	PRIORITY[(int)T::Geq]     = prio{ 3, 3};
	PRIORITY[(int)T::And]     = prio{ 2, 2};
	PRIORITY[(int)T::Or]      = prio{ 1, 1};
	
	
	{
		auto tl = alloc_tok_list();
		m_tok.force_consume(T::Bof, *tl);
	}
	
	m_root = parse_stat_list();
	
	{
		auto tl = alloc_tok_list();
		//m_tok.force_consume(T::Eof, *tl);
		auto node = alloc_stat<EOFStat>(tl);
		m_tok.force_consume(T::Eof, *tl);
		m_root->stats.push_back( node );
	}
	
#if 0
	printf("m_tok_alloc:  %u kiB\n", unsigned(m_tok_alloc.size()  / 1024));
	printf("m_stat_alloc: %u kiB\n", unsigned(m_stat_alloc.size() / 1024));
	printf("m_expr_alloc: %u kiB\n", unsigned(m_expr_alloc.size() / 1024));
	printf("m_type_alloc: %u kiB\n", unsigned(m_type_alloc.size() / 1024));
#endif
}

StatList* Parser::parse_stat_list()
{
	auto node = alloc_stat_list();
	while (!STAT_LIST_CLOSE_KEYWORDS[ (int)m_tok.peek() ]) {
		Statement* stat = parse_statement();
		m_tok.try_consume(T::Semicolon, *stat->tokens);
		node->stats.push_back(stat);
	}
	return node;
}


Statement* Parser::parse_statement()
{
	auto tl = alloc_tok_list();
	tl->reserve(TOKENS_PER_NODE);
	
	if (m_tok.try_consume(T::If, *tl))
	{
		auto node = alloc_stat<IfStat>(tl);
		
		do {
			auto cond = parse_expr();
			m_tok.force_consume(T::Then, *tl);
			auto body = parse_stat_list();
			node->clauses.push_back( IfStat::Clause{cond,body} );
		} while (m_tok.try_consume(T::Elseif, *tl));
		
		if (m_tok.try_consume(T::Else, *tl)) {
			// TODO: -- Warn agains C-style 'else if'
			auto body = parse_stat_list();
			node->clauses.push_back( IfStat::Clause{nullptr,body} );
		}
		m_tok.force_consume(T::End, *tl);
		return node;
	}
	
	else if (m_tok.try_consume(T::While, *tl))
	{
		auto node = alloc_stat<WhileStat>(tl);
		node->cond = parse_expr();
		m_tok.force_consume(T::Do, *tl);
		node->body = parse_stat_list();
		m_tok.force_consume(T::End, *tl);
		return node;
	}
	
	else if (m_tok.try_consume(T::Do, *tl))
	{
		auto node = alloc_stat<DoStat>(tl);
		node->body = parse_stat_list();
		m_tok.force_consume(T::End, *tl);
		return node;
	}
	
	else if (m_tok.try_consume(T::For, *tl))
	{
		auto base_var_name = m_tok.force_consume(T::Identifier, *tl);
		if (m_tok.try_consume(T::Assign, *tl)) {
			auto node = alloc_stat<NumericForStat>(tl);
			node->it_name = base_var_name;
			node->begin = parse_expr();
			m_tok.force_consume(T::Comma, *tl);
			node->end   = parse_expr();
			if (m_tok.try_consume(T::Comma, *tl)) {
				node->step  = parse_expr();
			} else {
				node->step = nullptr;
			}
			m_tok.force_consume(T::Do, *tl);
			node->body = parse_stat_list();
			m_tok.force_consume(T::End, *tl);
			return node;
		} else {
			auto node = alloc_stat<GenericForStat>(tl);
			
			node->var_names.push_back( base_var_name );
			while (m_tok.try_consume(T::Comma, *tl)) {
				node->var_names.push_back( m_tok.force_consume(T::Identifier, *tl) );
			}
			
			m_tok.force_consume(T::In, *tl);
			
			node->generators.push_back( parse_expr() );
			while (m_tok.try_consume(T::Comma, *tl)) {
				node->generators.push_back( parse_expr() );
			}
			
			m_tok.force_consume(T::Do, *tl);
			node->body = parse_stat_list();
			m_tok.force_consume(T::End, *tl);
			return node;
		}
	}
	
	else if (m_tok.try_consume(T::Repeat, *tl))
	{
		auto node = alloc_stat<RepeatUntilStat>(tl);
		node->body = parse_stat_list();
		m_tok.force_consume(T::Until, *tl);
		node->cond = parse_expr();
		return node;
	}
	
	else if (m_tok.try_consume(T::Function, *tl))
	{
		return parse_fun_decl(tl, Scoping::Implicit);
	}
	else if (m_tok.try_consume(T::Local, *tl))
	{
		if (m_tok.try_consume(T::Function, *tl)) {
			return parse_fun_decl(tl, Scoping::Local);
		} else {
			return parse_declaration(tl, Scoping::Local);
		}
	}
	else if (m_tok.try_consume(T::Global, *tl))
	{
		if (m_tok.try_consume(T::Function, *tl)) {
			return parse_fun_decl(tl, Scoping::Global);
		} else if (m_tok.try_consume(T::Typedef, *tl)) {
			return parse_typedef(tl, Scoping::Global);
		} else if (m_tok.try_consume(T::Class, *tl)) {
			return parse_class(tl, Scoping::Global);
		} else {
			return parse_declaration(tl, Scoping::Global);
		}
	}
	else if (m_tok.try_consume(T::Var, *tl))
	{
		return parse_declaration(tl, Scoping::Var);
	}
	else if (m_tok.try_consume(T::Typedef, *tl))
	{
		return parse_typedef(tl, Scoping::Local);
	}
	else if (m_tok.try_consume(T::Class, *tl))
	{
		return parse_class(tl, Scoping::Local);
	}
	
	else if (m_tok.try_consume(T::Return, *tl))
	{
		auto node = alloc_stat<ReturnStat>(tl);
		if (!STAT_LIST_CLOSE_KEYWORDS[ (int)m_tok.peek() ]) {
			node->arguments.reserve(2);
			node->arguments.push_back( parse_expr() );
			while (m_tok.try_consume(T::Comma, *tl)) {
				node->arguments.push_back( parse_expr() );
			}
		}
		return node;
	}
	
	else if (m_tok.try_consume(T::Break, *tl))
	{
		return alloc_stat<BreakStat>(tl);
	}
	else if (m_tok.try_consume(T::Goto, *tl))
	{
		auto node = alloc_stat<GotoStat>(tl);
		node->label = m_tok.force_consume(T::Identifier, *tl);
		return node;
	}
	
	else if (m_tok.try_consume(T::Label, *tl)) // ::
	{
		auto node = alloc_stat<LabelStat>(tl);
		node->label = m_tok.force_consume(T::Identifier, *tl);
		m_tok.force_consume(T::Label, *tl); // ::
		return node;
	}
	
	else {
		auto suffixed = parse_suffixed_expr();
		
		if (m_tok.peek() == T::Comma || m_tok.peek() == T::Assign)
		{
			const size_t GUESSED_NUM = 2;
			auto tl = alloc_tok_list();
			tl->reserve(2 * GUESSED_NUM - 1);
			auto node = alloc_stat<AssignmentStat>(tl);
			node->lhs.reserve(GUESSED_NUM);
			node->lhs.push_back( suffixed );
			while (m_tok.try_consume(T::Comma, *tl)) {
				node->lhs.push_back( parse_suffixed_expr() );
			}
			
			m_tok.force_consume(T::Assign, *tl);
			
			node->rhs.reserve( node->lhs.size() );
			node->rhs.push_back( parse_expr() );
			while (m_tok.try_consume(T::Comma, *tl)) {
				node->rhs.push_back( parse_expr() );
			}
			
			return node;
		}
		
		else if (ASSIGN_OPS[ (int)m_tok.peek() ] != T::None)
		{
			// += etc
			suffixed->synthetic = true; // FIXME: this is a lie
			
			auto op = ASSIGN_OPS[ (int)m_tok.peek() ];
			m_tok.consume_any(*tl);
			
			auto assign = alloc_stat<AssignmentStat>(tl);
			assign->synthetic = true;
			auto binop  = alloc_stat<BinopExpr>(tl);
			binop->synthetic = true;
			auto rhs_parens  = alloc_stat<ParenExpr>(tl);
			rhs_parens->synthetic = true;
			rhs_parens->inner = parse_expr();
			
			binop->lhs = suffixed;
			binop->op  = op;
			binop->rhs = rhs_parens;
			
			assign->lhs.push_back( suffixed );
			assign->rhs.push_back( binop    );
			
			return assign;
		}
		
		else if (m_tok.try_consume(T::AppendAssign, *tl))  // #=
		{
			suffixed->synthetic = true; // FIXME: this is a lie
			/*
			 Table append operator:
			 IN:    foo #= bar
			 OUT:   foo[#foo + 1] = bar
			 SLOW:  table.insert(foo, bar)
			 
			 TODO:  foo #= a, b, c
			 OUT:   foo[#foo + 1] = a; foo[#foo + 1] = b; foo[#foo + 1] = c;
			 */
			auto assign = alloc_stat<AssignmentStat>(tl);
			assign->synthetic = true;
			auto index  = alloc_stat<IndexExpr>(tl);
			index->synthetic = true;
			auto add    = alloc_stat<BinopExpr>(tl);
			add->synthetic = true;
			auto length = alloc_stat<UnopExpr>(tl);
			length->synthetic = true;
			
			static const auto s_one_literal = "1";
			static const Token one_token = { s_one_literal, s_one_literal+1, 0, 0, T::Integer };
			auto one_tl = alloc_tok_list();
			one_tl->push_back(&one_token);
			auto one    = alloc_stat<IntegerExpr>(one_tl);
			one->synthetic = true;
			auto array = suffixed;
			
			index->base  = array;
			index->index = add;
			
			length->op  = T::Hash;
			length->rhs = array;
			
			add->lhs = length;
			add->op  = T::Add;
			add->rhs = one;
			
			assign->lhs.push_back( index        );
			assign->rhs.push_back( parse_expr() );
			
			return assign;
		}
		
		else
		{
			auto call_expr = dynamic_cast<CallExpr*>(suffixed);
			p_assert(call_expr != nullptr, "Call epxression expected");
			auto node = alloc_stat<CallStat>(tl);
			node->expr = call_expr;
			return node;
		}
	}
}


VarDeclStat* Parser::parse_declaration(TokList* tl, Scoping scoping)
{
	auto node = alloc_stat<VarDeclStat>(tl);
	node->scoping = scoping;
	node->name_list.push_back( m_tok.force_consume(T::Identifier, *tl) );
	while (m_tok.try_consume(T::Comma, *tl)) {
		node->name_list.push_back( m_tok.force_consume(T::Identifier, *tl) );
	}
	
	if (m_tok.try_consume(T::Assign, *tl)) {
		node->init_list.reserve( node->name_list.size() );
		do {
			node->init_list.push_back( parse_expr() );
		} while (m_tok.try_consume(T::Comma, *tl));
	}
	
	return node;
}

FunDeclStat* Parser::parse_fun_decl(TokList* tl, Scoping scoping)
{
	auto node = alloc_stat<FunDeclStat>(tl);
	node->scoping   = scoping;
	node->name_expr = parse_suffixed_expr(OnlyDotColon);
	node->type      = parse_fun_type(alloc_tok_list(), MustHaveNames);
	node->body      = parse_stat_list();
	m_tok.force_consume(T::End, *tl);
	return node;
}


TypedefStat* Parser::parse_typedef(TokList* tl, Scoping scoping)
{
	/*
	 We allow:
	 typedef foo = ...    -- Normal local typedef
	 typedef M.bar = ...  -- namespaced typedef
	 typedef foo;    -- Forward declaration
	 typedef M.bar;  -- Forward declaration
	 
	 A namespaced typedef works like this:
	 
	 M must be a locally accessible object variable, which is now also a namespace.
	 A namespace maps idendifiers to types.
	 
	 The namespace variable can be returned from a module, and the namespace will then also be propagated.
	 
	 module.sol:
	 local M = {}
	 typedef M.Foo = int
	 return M
	 
	 user.sol:
	 local Mod = require 'module'
	 var foo = {} : Mod.Foo
	*/
	
	auto node = alloc_stat<TypedefStat>(tl);
	
	{
		auto first_name = m_tok.force_consume(T::Identifier, *tl);
		if (m_tok.try_consume(T::Dot, *tl)) {
			p_assert(scoping != Scoping::Global, "global typedef cannot have namespaced name");
			node->namespace_name = first_name;
			node->type_name      = m_tok.force_consume(T::Identifier, *tl);
		} else {
			node->namespace_name = nullptr;
			node->type_name      = first_name;
		}
	}
	
	if (!m_tok.try_consume(T::Semicolon, *tl)) {
		// Not forward declared
		
		if (m_tok.try_consume(T::Colon, *tl)) {
			// Inheritance
			node->base_types.reserve(1);
			do {
				node->base_types.push_back( parse_type() );
			} while (m_tok.try_consume(T::Comma, *tl));
		}
		
		m_tok.force_consume(T::Assign, *tl);
		node->type = parse_type();
	}
	
	return node;
}


ClassStat* Parser::parse_class(TokList* tl, Scoping scoping)
{
	auto node = alloc_stat<ClassStat>(tl);
	node->scoping = scoping;
	node->name    = m_tok.force_consume(T::Identifier, *tl);
	m_tok.force_consume(T::Assign, *tl);
	node->rhs     = parse_expr();
	return node;
}


// ------------------------------------------------------------------

TypeList* Parser::parse_type_list()
{
	auto tl = alloc_tok_list();
	auto node = alloc_type_list(tl);
	if (m_tok.try_consume(T::Void, *tl)) {
		return node;  // Empty type list means 'void'
	}
	
	node->types.reserve(2);
	do {
		node->types.push_back( parse_type() );
	} while (m_tok.try_consume(T::Comma, *tl));
	return node;
}


TypeNode* Parser::parse_type()
{
	auto tl = alloc_tok_list();
	
	if (m_tok.try_consume(T::ParenStart, *tl)) {
		auto node = alloc_type<ParenType>(tl);
		node->inner = parse_simple_type();
		m_tok.force_consume(T::ParenEnd, *tl);
		return node;
	}
	
	auto type = parse_simple_type();
	
	if (m_tok.try_consume(T::Questionmark, *tl))
	{
		auto node = alloc_type<NilableType>(tl);
		node->inner = type;
		type = node;
	}
	
	if (m_tok.try_consume(T::Or, *tl))
	{
		auto node = alloc_type<OrType>(tl);
		node->lhs = type;
		node->rhs = parse_type();
		type = node;
	}
	
	return type;
}


TypeNode* Parser::parse_simple_type()
{
	auto tl = alloc_tok_list();
	
	if (m_tok.try_consume(T::Integer, *tl)) {
		return alloc_type<IntegerType>(tl);
	}
	if (m_tok.try_consume(T::Number, *tl)) {
		return alloc_type<NumberType>(tl);
	}
	if (m_tok.try_consume(T::String, *tl)) {
		return alloc_type<StringType>(tl);
	}
	if (m_tok.try_consume(T::Nil, *tl)) {
		return alloc_type<NilType>(tl);
	}
	if (m_tok.try_consume(T::True, *tl)) {
		return alloc_type<TrueType>(tl);
	}
	if (m_tok.try_consume(T::False, *tl)) {
		return alloc_type<FalseType>(tl);
	}
	if (m_tok.try_consume(T::Extern, *tl)) {
		return alloc_type<ExternType>(tl);
	}
	
	if (m_tok.try_consume(T::BracketStart, *tl)) {
		auto node = alloc_type<ListType>(tl);
		node->type = parse_type();
		m_tok.force_consume(T::BracketEnd, *tl);
		return node;
	}
	
	if (m_tok.try_consume(T::CurlyStart, *tl)) {
		// Object or map?
		p_assert(m_tok.peek() != T::CurlyEnd, "Use 'object' instead of '{}'");
		
		if (m_tok.peek() == T::Identifier &&
			 m_tok.lookahead() == T::Colon)
		{
			auto node = alloc_type<ObjectType>(tl);
			do
			{
				node->names.push_back( m_tok.force_consume(T::Identifier, *tl) );
				m_tok.force_consume(T::Colon, *tl);
				node->types.push_back( parse_type() );
				
				if (m_tok.try_consume(T::Comma, *tl) || m_tok.try_consume(T::Semicolon, *tl)) {
					// Good separator
				} else if (m_tok.try_consume(T::CurlyEnd, *tl)) {
					break;
				} else {
					throw_error("Expected one of: ,;}");
				}
			} while (!m_tok.try_consume(T::CurlyEnd, *tl));
			return node;
		}
		else {
			// A map or a set
			auto key_type = parse_type();
			if (m_tok.try_consume(T::MapArrow, *tl)) {
				auto node = alloc_type<MapType>(tl);
				node->key   = key_type;
				node->value = parse_type();
				m_tok.force_consume(T::CurlyEnd, *tl);
				return node;
			} else {
				auto node = alloc_type<SetType>(tl);
				node->type = key_type;
				m_tok.force_consume(T::CurlyEnd, *tl);
				return node;
			}
		}
	}
	
	if (m_tok.try_consume(T::Function, *tl)) {
		return parse_fun_type(tl, MustHaveTypes);
	}
	
	if (m_tok.peek() == T::Identifier) {
		auto node = alloc_type<IdentifierType>(tl);
		auto name = m_tok.force_consume(T::Identifier, *tl);
		if (m_tok.try_consume(T::Dot, *tl)) {
			node->var_name = name;
			node->name     = m_tok.force_consume(T::Identifier, *tl);
		} else {
			node->var_name = nullptr;
			node->name     = name;
		}
		return node;
	}
	
	throw_error("Type expected");
	return nullptr;
}


FunType* Parser::parse_fun_type(TokList* tl, ArgTypes arg_types)
{
	auto node = alloc_type<FunType>(tl);
	m_tok.force_consume(T::ParenStart, *tl);
	
	if (!m_tok.try_consume(T::ParenEnd, *tl)) {
		node->args.reserve(8);
		
		for (;;)
		{
			FunType::Arg arg { nullptr, nullptr };
			
			if (m_tok.peek() == T::Ellipsis) {
				if (m_tok.lookahead() == T::Colon) {
					arg.name = m_tok.force_consume(T::Ellipsis, *tl);
					m_tok.force_consume(T::Colon, *tl);
					arg.type = parse_type();
				} else if (arg_types == MustHaveNames) {
					arg.name = m_tok.force_consume(T::Ellipsis, *tl);
				} else {
					arg.name = m_tok.force_consume(T::Ellipsis, *tl);
					//arg.type = parse_type();  // TODO: Ellipsis type ?
				}
				m_tok.force_consume(T::ParenEnd, *tl);
				node->args.push_back(arg);
				break;
			}
			
			if (arg_types == MustHaveNames) {
				arg.name = m_tok.force_consume(T::Identifier, *tl);
				if (m_tok.try_consume(T::Colon, *tl)) {
					arg.type = parse_type();
				}
			} else {
				if (m_tok.peek() == T::Identifier &&
					 m_tok.lookahead() == T::Colon)
				{
					arg.name = m_tok.force_consume(T::Identifier, *tl);
					m_tok.force_consume(T::Colon, *tl);
					arg.type = parse_type();
				}
				else {
					arg.type = parse_type();
				}
			}
			node->args.push_back(arg);
			
			if (!m_tok.try_consume(T::Comma, *tl)) {
				m_tok.force_consume(T::ParenEnd, *tl);
				break;
			}
		}
	}
	
	if (m_tok.try_consume(T::FunctionTypeArrow, *tl)) {
		node->rets = parse_type_list();
	} else {
		node->rets = nullptr;
	}
	
	return node;
}

// ------------------------------------------------------------------


Expression* Parser::parse_expr(int prio_level)
{
	Expression* expr;
	
	T next_t = m_tok.peek();
	
	if (next_t == T::Sub || next_t == T::Not || next_t == T::Hash) {
		// Unary operation. TODO: ALLOW unary +
		auto tl = alloc_tok_list();
		tl->reserve(1);
		auto node = alloc_expr<UnopExpr>(tl);
		node->op  = m_tok.consume_any(*tl)->type;
		node->rhs = parse_expr(UNOP_PRIO);
		expr = node;
	} else {
		expr = parse_simple_expr();
	}
	
	for (;;) {
		auto prio = PRIORITY[ (int)m_tok.peek() ];
		if (prio.a > prio_level) {
			auto tl = alloc_tok_list();
			tl->reserve(1);
			auto node = alloc_expr<BinopExpr>(tl);
			node->lhs = expr;
			node->op  = m_tok.consume_any(*tl)->type;
			node->rhs = parse_expr();
			expr = node;
		} else {
			break;
		}
	}
	
	if (m_tok.peek() == T::Colon && m_tok.has_space_before()) {
		auto tl = alloc_tok_list();
		auto node = alloc_expr<CastExpr>(tl);
		node->expr = expr;
		m_tok.force_consume(T::Colon, *tl);
		node->type = parse_type();
		expr = node;
	}
	
	return expr;
}


Expression* Parser::parse_simple_expr()
{
	auto tl  = alloc_tok_list();
	tl->reserve(1);
	
	if (m_tok.try_consume(T::Integer, *tl)) {
		return alloc_expr<IntegerExpr>(tl);
	}
	if (m_tok.try_consume(T::Number, *tl)) {
		return alloc_expr<NumberExpr>(tl);
	}
	if (m_tok.try_consume(T::String, *tl)) {
		return alloc_expr<StringExpr>(tl);
	}
	if (m_tok.try_consume(T::Nil, *tl)) {
		return alloc_expr<NilExpr>(tl);
	}
	if (m_tok.try_consume(T::True, *tl)) {
		return alloc_expr<TrueExpr>(tl);
	}
	if (m_tok.try_consume(T::False, *tl)) {
		return alloc_expr<FalseExpr>(tl);
	}
	if (m_tok.try_consume(T::Ellipsis, *tl)) {
		return alloc_expr<VarArgsExpr>(tl);
	}
	
	// Externally defined value (i.e. an intrinsic)
	if (m_tok.try_consume(T::Extern, *tl)) {
		return alloc_expr<ExternExpr>(tl);
	}
	
	if (m_tok.try_consume(T::CurlyStart, *tl)) {  // {
		auto node = alloc_expr<ConstructorExpr>(tl);
		
		if (!m_tok.try_consume(T::CurlyEnd, *tl))
		{
			node->entries.reserve(4);
			do
			{
				ConstructorExpr::Entry entry;
				
				if (m_tok.try_consume(T::BracketStart, *tl)) { // [
					entry.key_expr = parse_expr();
					m_tok.force_consume(T::BracketEnd, *tl); // ]
					m_tok.force_consume(T::Assign,     *tl); // =
				}
				else if (m_tok.peek() == T::Identifier) {
					if (m_tok.lookahead() == T::Assign) {
						entry.key_ident = m_tok.consume_any(*tl);
						m_tok.force_consume(T::Assign, *tl);
					}
				}
				
				entry.value = parse_expr();
				node->entries.push_back( entry );
				
				if (m_tok.try_consume(T::Comma, *tl) || m_tok.try_consume(T::Semicolon, *tl)) {
					// Good separator
				} else if (m_tok.try_consume(T::CurlyEnd, *tl)) {
					break;
				} else {
					throw_error("Expected one of: ,;}");
				}
			} while (!m_tok.try_consume(T::CurlyEnd, *tl));
		}
		return node;
	}
	
	if (m_tok.try_consume(T::Function, *tl))
	{
		auto node = alloc_expr<LambdaExpr>(tl);
		node->type = parse_fun_type(alloc_tok_list(), MustHaveNames);
		node->body = parse_stat_list();
		m_tok.force_consume(T::End, *tl);
		return node;
	}
	
	return parse_suffixed_expr();
}


Expression* Parser::parse_suffixed_expr(SuffixStyle style)
{
	auto allow_anything = (style == Anything);
	auto prim = parse_primary_expr();
	
	for (;;)
	{
		auto tl = alloc_tok_list();
		tl->reserve(1);
		
		if (m_tok.try_consume(T::Dot, *tl))
		{
			auto node = alloc_expr<MemberExprDot>(tl);
			node->base  = prim;
			node->ident = m_tok.force_consume(T::Identifier, *tl);
			prim = node;
		}
		else if (!m_tok.has_space_before() && m_tok.try_consume(T::Colon, *tl))
		{
			auto node = alloc_expr<MemberExprColon>(tl);
			node->base  = prim;
			node->ident = m_tok.force_consume(T::Identifier, *tl);
			prim = node;
		}
		else if (allow_anything)
		{
			if (m_tok.try_consume(T::BracketStart, *tl))
			{
				auto node = alloc_expr<IndexExpr>(tl);
				node->base  = prim;
				node->index = parse_expr();
				m_tok.force_consume(T::BracketEnd, *tl);
				prim = node;
			}
			else if (m_tok.try_consume(T::ParenStart, *tl))
			{
				auto node = alloc_expr<ParenCallExpr>(tl);
				node->base  = prim;
				
				if (!m_tok.try_consume(T::ParenEnd, *tl))
				{
					node->args.reserve(4);
					do {
						node->args.push_back( parse_expr() );
					} while (m_tok.try_consume(T::Comma, *tl));
					m_tok.force_consume(T::ParenEnd, *tl);
				}
				
				prim = node;
			}
			else if (m_tok.peek() == T::String)
			{
				// FIXME: node has no tokens, so it cannot be placed!
				auto node = alloc_expr<StringCallExpr>(tl);
				node->args.resize(1);
				node->base  = prim;
				node->args[0] = parse_expr();
				prim = node;
			}
			else if (m_tok.peek() == T::CurlyStart)
			{
				// FIXME: node has no tokens, so it cannot be placed!
				auto node = alloc_expr<TableCallExpr>(tl);
				node->args.resize(1);
				node->base  = prim;
				node->args[0] = parse_expr();
				prim = node;
			}
			else
			{
				break;
			}
		}
		else
		{
			break;
		}
	}
	
	return prim;
}


Expression* Parser::parse_primary_expr()
{
	if (m_tok.peek() == T::Identifier)
	{
		return parse_id_expr();
	}
	else
	{
		auto tl = alloc_tok_list();
		tl->reserve(2);
		m_tok.force_consume(T::ParenStart, *tl);
		auto node = alloc_expr<ParenExpr>(tl);
		node->inner = parse_expr();
		m_tok.force_consume(T::ParenEnd, *tl);
		return node;
	}
}


Expression* Parser::parse_id_expr()
{
	auto tl = alloc_tok_list();
	tl->reserve(1);
	auto node = alloc_expr<IdExpr>(tl);
	node->ident = m_tok.force_consume(T::Identifier, *tl);
	return node;
}
