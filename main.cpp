#include <iostream>
#include <ctime>
#include "lexer.hpp"
#include "parser.hpp"
#include "output.hpp"
#include "util.hpp"
#include "stripper.hpp"

bool g_spam = true;
StringList g_call_bases, g_function_names;


void point_to_source(const char* src, unsigned line, unsigned col)
{
	while (line > 1) {
		if (*src == 0) {
			return;
		}
		if (*src == '\n') {
			line -= 1;
		}
		++src;
	}

	while (*src && *src !='\n') {
		if (*src == '\t') {
			putchar(' ');
		} else {
			putchar(*src);
		}
		++src;
	}
	putchar('\n');
	for (unsigned i=1; i<col; ++i) {
		putchar(' ');
	}
	putchar('^');
	putchar('\n');
}


void parse(const char* in_file, const char* out_file)
{
	bool sol = false;
	
	if (strlen(in_file) >= 4 &&
		 strncmp(in_file+strlen(in_file)-4, ".sol", 4) == 0)
	{
		if (g_spam) { printf("Sol file detected.\n"); }
		sol = true;
	}
	
	
	if (g_spam) { printf("Parsing %s...\n", in_file); fflush(stdout); }
	
	try {
		auto data = read_file(in_file);
		data.push_back(0); // Make zero ended
		const char* contents = (const char*)data.data();
	
		try {
			auto tic = clock();
			Tokens tokens( contents, data.size()-1, sol );
			
			if (g_spam) {
				auto toc = clock();
				
				printf("Lexing: %.2f ms\n", 1000.0f*(toc-tic)/CLOCKS_PER_SEC);
				printf("%u bytes, %u tokens, %u whites\n", (unsigned)data.size(),
						 (unsigned)tokens.num_tokens(), (unsigned)tokens.num_whites());
			}
			tic = clock();
			
			Parser parser( tokens );
			
			if (g_spam) {
				auto toc = clock();
				printf("Parsing: %.2f ms\n", 1000.0f*(toc-tic)/CLOCKS_PER_SEC);
			}
			
			tic = clock();

			Stripper stripper(g_call_bases, g_function_names);
			stripper.stat_list( parser.root() );
			if (g_spam) {
				auto toc = clock();
				printf("Stripped %d statements in %.2f ms\n", stripper.num_erased_stats(), 1000.0f*(toc-tic)/CLOCKS_PER_SEC);
			}
			
			tic = clock();
			auto out_data = output( tokens, parser );
			
			if (g_spam) {
				auto toc = clock();
				
				printf("Output: %u bytes, %.2f ms\n", (unsigned)out_data.size(), 1000.0f*(toc-tic)/CLOCKS_PER_SEC);
			}
			
			if (out_file) {
				if (g_spam) { printf("Writing to %s...\n", out_file); fflush(stdout); }
				write_file(out_file, out_data.data(), out_data.size());
			}
		}
		catch (lex_error& e) {
			fprintf(stderr, "Lexer error: %s:%d:%d: %s\n", in_file, e.line(), e.col(), e.what());
			point_to_source(contents, e.line(), e.col());
			exit(1);
		}
		catch (parse_error& e) {
			fprintf(stderr, "Parser error: %s:%d:%d: %s\n", in_file, e.line(), e.col(), e.what());
			point_to_source(contents, e.line(), e.col());
			exit(1);
		}
	}
	catch (std::exception& e) {
		fprintf(stderr, "Error %s: %s\n", in_file, e.what());
		exit(1);
	}
}

void print_usage()
{
	const char* usage = R"( Usage:
-fun foo   - will strip all calls on the form foo(...)
-fb  Bar   - will strip all calls on the form Bar.baz(...)
-o   dest  - will write the output here
--v        - turn on verbose output

Several -fun and -fb can be added. All will be used.
All matching is case sensitive and matches whole words.

Examples:
	solstrip --v -fun assert -o stripped.lua input.lua
)";
	printf(usage);
}


int main(int argc, const char* argv[])
{
#if 0
	printf("Benching...\n");
	g_spam = false;
	auto ITERS = 2000;
	auto tic = clock();
	for (int i=0; i<ITERS; ++i) {
		parse("/Users/emilk/Documents/kod/lua/Sol/sol/parser.sol", nullptr);
	}
	auto toc = clock();
	printf("Avg time: %.2f ms\n", 1000.0f*(toc-tic)/CLOCKS_PER_SEC/ITERS);
#elif 0
	g_spam = true;
	parse("/Users/emilk/Documents/kod/lua/Sol/sol/parser.sol", "test_output.lua");
#else
	if (argc == 1) {
		print_usage();
		exit(0);
	}
	
	g_spam = false;
	const char* out_path = nullptr;
	for (int i=1; i<argc; ++i) {
		if (strcmp(argv[i], "--v") == 0) {
			g_spam = true;
		} else if (strcmp(argv[i], "-o") == 0) {
			out_path = argv[i + 1];
			++i;
		} else if (strcmp(argv[i], "-fb") == 0) {
			g_call_bases.push_back(argv[i + 1]);
			++i;
		} else if (strcmp(argv[i], "-fun") == 0) {
			g_function_names.push_back(argv[i + 1]);
			++i;
		} else {
			parse(argv[i], out_path);
		}
	}
#endif
}
