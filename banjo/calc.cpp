// Copyright (c) 2015-2016 Andrew Sutton
// All rights reserved

#include "context.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "printer.hpp"
#include "evaluation.hpp"

#include "gen/llvm/generator.hpp"

#include <lingo/file.hpp>
#include <lingo/io.hpp>
#include <lingo/error.hpp>

#include <iostream>


using namespace lingo;
using namespace banjo;


int
main(int argc, char* argv[])
{
  Context cxt;

  while (true) {
    String str;
    std::getline(std::cin, str);
    if (!std::cin)
      break;
    
    // Build our input buffer and streams.
    Buffer buf = str;
    Character_stream cs = buf;
    Token_stream ts;

    // Lexical analysis.
    Lexer lex(cxt, cs, ts);
    lex();
    if (error_count())
      return 1;

    // Syntactic and semantic analysis.
    Parser parse(cxt, ts);
    Expr& expr = parse.expression();
    
    // Print the expression in reduced form.
    Expr& red = reduce(cxt, expr);
    std::cout << red << '\n';
  }
}
