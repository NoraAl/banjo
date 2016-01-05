// Copyright (c) 2015-2016 Andrew Sutton
// All rights reserved

#include "print.hpp"

#include <iostream>


namespace beaker
{

void
Printer::id(Name const* n)
{
  if (Qualified_id const* q = as<Qualified_id>(n))
    qualified_id(q);
  else
    unqualified_id(n);
}


void
Printer::unqualified_id(Name const* n)
{
  struct fn
  {
    Printer& p;
    void operator()(Simple_id const* n)      { p.unqualified_id(n); }
    void operator()(Global_id const* n)      { }
    void operator()(Placeholder_id const* n) { }
    void operator()(Operator_id const* n)    { p.operator_id(n); }
    void operator()(Conversion_id const* n)  { p.conversion_id(n); }
    void operator()(Literal_id const* n)     { p.literal_id(n); }
    void operator()(Destructor_id const* n)  { p.destructor_id(n); }
    void operator()(Template_id const* n)    { p.template_id(n); }
    void operator()(Qualified_id const* n)   { lingo_unreachable(); }
  };
  apply(n, fn{*this});
}


void
Printer::unqualified_id(Simple_id const* n)
{
  os << *n->symbol();
}


void
Printer::destructor_id(Destructor_id const*)
{
  os << '~';
  os << "<|type|>";
}


void
Printer::operator_id(Operator_id const*)
{
  os << "operator";
}


void
Printer::conversion_id(Conversion_id const*)
{
  os << "operator";
}


void
Printer::literal_id(Literal_id const*)
{
  os << "operator";
}


void
Printer::template_id(Template_id const*)
{
  os << "<|template-decl|>" << '<' << '>';
}


void
Printer::qualified_id(Qualified_id const* n)
{
  nested_name_specifier(n);
  while (is<Qualified_id>(n->name()))
    n = cast<Qualified_id>(n->name());
  unqualified_id(n->name());
}


void
Printer::nested_name_specifier(Qualified_id const* n)
{
  unqualified_id(n->context()->name());
  os << "::";
  if (Qualified_id const* q = as<Qualified_id>(n->name()))
    nested_name_specifier(q);
}

// -------------------------------------------------------------------------- //
// Printing of types

void
Printer::type(Type const* t)
{
  struct fn
  {
    Printer& p;
    void operator()(Void_type const* t)      { p.simple_type(t); }
    void operator()(Boolean_type const* t)   { p.simple_type(t); }
    void operator()(Integer_type const* t)   { p.simple_type(t); }
    void operator()(Float_type const* t)     { p.simple_type(t); }
    void operator()(Auto_type const* t)      { p.simple_type(t); }
    void operator()(Decltype_type const* t)  { p.simple_type(t); }
    void operator()(Declauto_type const* t)  { p.simple_type(t); }
    void operator()(Qualified_type const* t) { lingo_unreachable(); }
    void operator()(Pointer_type const* t)   { lingo_unreachable(); }
    void operator()(Reference_type const* t) { lingo_unreachable(); }
    void operator()(Array_type const* t)     { lingo_unreachable(); }
    void operator()(Sequence_type const* t)  { lingo_unreachable(); }
    void operator()(Class_type const* t)     { lingo_unreachable(); }
    void operator()(Union_type const* t)     { lingo_unreachable(); }
    void operator()(Enum_type const* t)      { lingo_unreachable(); }
  };
  apply(t, fn{*this});
}


void
Printer::simple_type(Void_type const* t)
{
  os << "void";
}


void
Printer::simple_type(Boolean_type const* t)
{
  os << "bool";
}


void
Printer::simple_type(Integer_type const* t)
{
  if (t->is_unsigned())
    os << 'u';
  os << "int" << t->precision();
}


void
Printer::simple_type(Float_type const* t)
{
  os << "float" << t->precision();
}


void
Printer::simple_type(Auto_type const* t)
{
  os << "auto";
}


// TODO: Implement me.
void
Printer::simple_type(Decltype_type const* t)
{
  os << "decltype(<|expr|>)";
}


void
Printer::simple_type(Declauto_type const* t)
{
  os << "decltype(auto)";
}


// -------------------------------------------------------------------------- //
// Streaming.

std::ostream&
operator<<(std::ostream& os, Name const& n)
{
  Printer print(os);
  print(&n);
  return os;
}


std::ostream&
operator<<(std::ostream& os, Type const& t)
{
  Printer print(os);
  print(&t);
  return os;
}


} // namespace beaker
