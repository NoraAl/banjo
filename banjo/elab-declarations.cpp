// Copyright (c) 2015-2016 Andrew Sutton
// All rights reserved

#include "elab-declarations.hpp"
#include "parser.hpp"
#include "printer.hpp"
#include "ast.hpp"
#include "declaration.hpp"

#include <iostream>


namespace banjo
{

 Elaborate_declarations::Elaborate_declarations(Parser& p)
  : parser(p), cxt(p.cxt)
{ }


// -------------------------------------------------------------------------- //
// Statements

void
Elaborate_declarations::translation_unit(Translation_unit& tu)
{
  Enter_scope scope(cxt, tu);
  statement_seq(tu.statements());
  clear_extensions(tu.statements());
}

//Remove all extension statements
void
Elaborate_declarations::clear_extensions(Stmt_list& ss)
{
  struct is_extension_stmt
  {
    bool operator()(Extension_decl& d) { return true;}
    bool operator()(Decl&d){ return false;}
  };

  struct fn
  {
    bool operator()(Stmt& s)             { return false;}
    bool operator()(Compound_stmt& s)    { return false; }
    bool operator()(Declaration_stmt& s) { return apply(s.declaration(), is_extension_stmt{}); }
  };
  for (auto iter = ss.begin(); iter != ss.end();){

    if(apply(*iter, fn{}))
      iter = ss.remove_itr(iter);
    else
      ++iter;
  }
}

void
Elaborate_declarations::statement(Stmt& s)
{
  struct fn
  {
    Self& elab;
    void operator()(Stmt& s)             { /* Do nothing. */ }
    void operator()(Compound_stmt& s)    { elab.compound_statement(s); }
    void operator()(Declaration_stmt& s) { elab.declaration_statement(s); }
  };
  apply(s, fn{*this});
}


void
Elaborate_declarations::statement_seq(Stmt_list& ss)
{
  for (Stmt& s : ss)
    statement(s);
}


void
Elaborate_declarations::compound_statement(Compound_stmt& s)
{
  Enter_scope scope(cxt, cxt.saved_scope(s));
  statement_seq(s.statements());
}


void
Elaborate_declarations::declaration_statement(Declaration_stmt& s)
{
  declaration(s.declaration());
}


// -------------------------------------------------------------------------- //
// Declarations

void
Elaborate_declarations::declaration(Decl& d)
{
  struct fn
  {
    Self& elab;
    void operator()(Decl& d)           { lingo_unhandled(d); }
    void operator()(Variable_decl& d)  { elab.variable_declaration(d); }
    void operator()(Constant_decl& d)  { elab.constant_declaration(d); }
    void operator()(Super_decl& d)     { elab.super_declaration(d); }
    void operator()(Function_decl& d)  { elab.function_declaration(d); }
    void operator()(Coroutine_decl& d) { elab.coroutine_declaration(d); }
    void operator()(Class_decl& d)     { elab.class_declaration(d); }
    void operator()(Extension_decl& d) {    /* Do nothing */        }
  };
  apply(d, fn{*this});
}


void
Elaborate_declarations::variable_declaration(Variable_decl& d)
{
  d.type_ = &type(d.type());
}


void
Elaborate_declarations::constant_declaration(Constant_decl& d)
{
  d.type_ = &type(d.type());
}


void
Elaborate_declarations::super_declaration(Super_decl& d)
{
  d.type_ = &type(d.type());
}


void
Elaborate_declarations::function_declaration(Function_decl& decl)
{
  // Create a new scope for the function and elaborate parameter
  // declarations. Note that we're going 
  Decl_list& parms = decl.parameters();
  for (Decl& p : parms)
    parameter(p);

  // Elaborate the return type.
  Type& ret = type(decl.return_type());

  // Rebuild the function type and update the declaration.
  decl.type_ = &cxt.get_function_type(parms, ret);

  // FIXME: Rewrite expression definitions into function definitions
  // to simplify later analysis and code gen.
  
  // Enter the function scope and recurse through the definition.
  Enter_scope scope(cxt, decl);
  if (Function_def* def = as<Function_def>(&decl.definition()))
    statement(def->statement());
}


// TODO: We should probably be doing more checking here.
//
// TODO: Is this the appropriate place to transform a couroutine
// into a class. Perhaps...
void
Elaborate_declarations::coroutine_declaration(Coroutine_decl &decl)
{
  // Elaborate the parameters.
  Decl_list& parms = decl.parameters();
  for (Decl& p : parms)
    parameter(p);
  
  // Elaborate the return type of the coroutine
  decl.ret_ = &type(decl.type());

  // FIXME: Don't we have to set the coroutine type here?

  Enter_scope scope(cxt, decl);
  if (Function_def* def = as<Function_def>(&decl.definition()))
    statement(def->statement());
}


void
Elaborate_declarations::class_declaration(Class_decl& d)
{
  struct fn
  {
    Self& elab;
    void operator()(Def& d)       { lingo_unhandled(d); }
    void operator()(Class_def& d) { elab.statement_seq(d.statements()); }
  };

  // Update the class kind/metatype
  d.kind_ = &type(d.kind());

  // search for declarations of the appropriate extensions in the current scope
  // current scope: nameMap: append decls definitions along with this one
  Elaborate_partials elaborate(cxt,parser.current_scope(), d);

  Enter_scope scope(cxt, d);
  apply(d.definition(), fn{*this});
}


void
Elaborate_declarations::parameter(Decl& p)
{
  parameter(cast<Object_parm>(p));
}


void
Elaborate_declarations::parameter(Object_parm& p)
{
  p.type_ = &type(p.type());
}


// -------------------------------------------------------------------------- //
// Types

// Parse the type as needed, returning its fully elaborated form.
Type&
Elaborate_declarations::type(Type& t)
{
  if (Unparsed_type* u = as<Unparsed_type>(&t)) {
    Save_input_location loc(cxt);
    Token_stream ts(u->tokens());
    Parser p(cxt, ts);
    return p.type();    
  }
  return t;
}



// -------------------------------------------------------------------------- //
// Elaborate Extensions

// Check overload set for every class declaration and collect related extensions
Elaborate_partials::Elaborate_partials(Context& c, Scope& s, Class_decl& d)
        : cxt(c), current_scope(&s), decl(&d)
{
  if(ovl = current_scope->lookup(decl->name())){
    if(ovl->size()>1){
      def = as<Class_def>(&decl->definition());
      appropriate_declaration = true;
      collect(cxt.saved_scope(d));}
  }

}

void
Elaborate_partials::collect(Scope& class_scope) {
  for (auto iter = ovl->begin(); iter != ovl->end(); ++iter)
    add_to_main_class(*iter, class_scope);
}

void
Elaborate_partials::add_to_main_class(Decl& d, Scope& class_scope) {
  if(!is<Extension_decl>(&d))//maybe show an error?
    return;

  Scope* temp = &cxt.saved_scope(d);

  // elaborate extensions recursively for every definition,
  // or to be handled later on?
  std::for_each(temp->names.begin(), temp->names.end(), [&class_scope](auto iter){
    auto i = class_scope.names.find(iter.first);

    if (i != class_scope.names.end()){
      Overload_set* tovl = &iter.second;
      i->second.append(tovl->begin(),tovl->end());
    } else{
      class_scope.names.insert(iter);
    }
  });

  struct fn
  {
    Stmt_list& operator()(Def& d)       { lingo_unhandled(d); }
    Stmt_list& operator()(Class_def& d) { return d.statements(); }
  };

  Extension_decl* extension_decl = as<Extension_decl>(&d);
  Stmt_list& lis = apply(extension_decl->definition(), fn{});
  def->add_statements(lis);
}

//removes all declarations of extensions in the current class scope
Elaborate_partials::~Elaborate_partials()
{
  if (appropriate_declaration)
    for (auto iter = ovl->begin(); iter != ovl->end();)
      if (is_extension(*iter))
        iter = ovl->erase_decl(iter);
      else
        ++iter;
}


bool
Elaborate_partials::is_extension(Decl& d) {
  if(!is<Extension_decl>(&d))
    return false;
  return true;
}
} // namespace banjo

