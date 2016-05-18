// Copyright (c) 2015-2016 Andrew Sutton
// All rights reserved

#ifndef BANJO_ELAB_DECLARATIONS_HPP
#define BANJO_ELAB_DECLARATIONS_HPP

#include "language.hpp"
//#include "ast.hpp"


namespace banjo
{

struct Parser;


// Recursively parse and analyze the types of all declared names. After 
// elaboration, every declaration has a type, which may be a placeholder.
struct Elaborate_declarations
{
  using Self = Elaborate_declarations;

  Elaborate_declarations(Parser&);

  void operator()(Translation_unit& s) { translation_unit(s); }

  void translation_unit(Translation_unit&);

  void statement(Stmt&);
  void statement_seq(Stmt_list&);
  void clear_extensions(Stmt_list&);
  void compound_statement(Compound_stmt&);
  void declaration_statement(Declaration_stmt&);

  void declaration(Decl&);
  void variable_declaration(Variable_decl&);
  void constant_declaration(Constant_decl&);
  void super_declaration(Super_decl&);
  void function_declaration(Function_decl&);
  void coroutine_declaration(Coroutine_decl&);
  void class_declaration(Class_decl&);

  void parameter(Decl&);
  void parameter(Object_parm&);

  Type& type(Type&);

  Parser&  parser;
  Context& cxt;
};

struct Elaborate_partials
{
  Elaborate_partials(Context&,Scope&, Class_decl&);
  ~Elaborate_partials();
  void collect(Scope&);
  void add_to_main_class(Decl&, Scope&);
  bool is_extension(Decl&);

  Scope* current_scope;
  Class_decl*   decl;
  Overload_set* ovl;
  Context& cxt;
  bool appropriate_declaration;
  Class_def* def;
};


} // nammespace banjo


#endif
