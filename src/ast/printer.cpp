#include <regex>

#include "printer.h"
#include "ast.h"
#include "driver.h"

namespace bpftrace {
namespace ast {

void Printer::visit(Integer &integer)
{
  std::string indent(depth_, ' ');
  out_ << indent << "int: " << integer.n << std::endl;
}

void Printer::visit(String &string)
{
  std::string indent(depth_, ' ');

  std::string str = string.str;
  str = std::regex_replace(str, std::regex("\\\\"), "\\\\");
  str = std::regex_replace(str, std::regex("\n"), "\\n");
  str = std::regex_replace(str, std::regex("\t"), "\\t");
  str = std::regex_replace(str, std::regex("\""), "\\\"");

  out_ << indent << "string: " << str << std::endl;
}

void Printer::visit(Builtin &builtin)
{
  std::string indent(depth_, ' ');
  out_ << indent << "builtin: " << builtin.ident << std::endl;
}

void Printer::visit(Call &call)
{
  std::string indent(depth_, ' ');
  out_ << indent << "call: " << call.func << std::endl;

  ++depth_;
  if (call.vargs) {
    for (Expression *expr : *call.vargs) {
      expr->accept(*this);
    }
  }
  --depth_;
}

void Printer::visit(Map &map)
{
  std::string indent(depth_, ' ');
  out_ << indent << "map: " << map.ident << std::endl;

  ++depth_;
  if (map.vargs) {
    for (Expression *expr : *map.vargs) {
      expr->accept(*this);
    }
  }
  --depth_;
}

void Printer::visit(Variable &var)
{
  std::string indent(depth_, ' ');
  out_ << indent << "variable: " << var.ident << std::endl;
}

void Printer::visit(Binop &binop)
{
  std::string indent(depth_, ' ');
  out_ << indent << opstr(binop) << std::endl;

  ++depth_;
  binop.left->accept(*this);
  binop.right->accept(*this);
  --depth_;
}

void Printer::visit(Unop &unop)
{
  std::string indent(depth_, ' ');
  out_ << indent << opstr(unop) << std::endl;

  ++depth_;
  unop.expr->accept(*this);
  --depth_;
}

void Printer::visit(FieldAccess &acc)
{
  std::string indent(depth_, ' ');
  out_ << indent << "." << std::endl;

  ++depth_;
  acc.expr->accept(*this);
  --depth_;

  out_ << indent << " " << acc.field << std::endl;
}

void Printer::visit(Cast &cast)
{
  std::string indent(depth_, ' ');
  out_ << indent << "(" << cast.cast_type << ")" << std::endl;

  ++depth_;
  cast.expr->accept(*this);
  --depth_;
}

void Printer::visit(ExprStatement &expr)
{
  expr.expr->accept(*this);
}

void Printer::visit(AssignMapStatement &assignment)
{
  std::string indent(depth_, ' ');
  out_ << indent << "=" << std::endl;

  ++depth_;
  assignment.map->accept(*this);
  assignment.expr->accept(*this);
  --depth_;
}

void Printer::visit(AssignVarStatement &assignment)
{
  std::string indent(depth_, ' ');
  out_ << indent << "=" << std::endl;

  ++depth_;
  assignment.var->accept(*this);
  assignment.expr->accept(*this);
  --depth_;
}

void Printer::visit(Predicate &pred)
{
  std::string indent(depth_, ' ');
  out_ << indent << "pred" << std::endl;

  ++depth_;
  pred.expr->accept(*this);
  --depth_;
}

void Printer::visit(AttachPoint &ap)
{
  std::string indent(depth_, ' ');
  out_ << indent << ap.name(ap.func) << std::endl;
}

void Printer::visit(Probe &probe)
{
  for (AttachPoint *ap : *probe.attach_points) {
    ap->accept(*this);
  }

  ++depth_;
  if (probe.pred) {
    probe.pred->accept(*this);
  }
  for (Statement *stmt : *probe.stmts) {
    stmt->accept(*this);
  }
  --depth_;
}

void Printer::visit(Program &program)
{
  std::string indent(depth_, ' ');
  out_ << indent << "Program" << std::endl;

  ++depth_;
  for (Probe *probe : *program.probes)
    probe->accept(*this);
  --depth_;
}

void Printer::print_all(Driver &driver)
{
  for (Include *include : *driver.includes_)
    print_include(*include);
  for (Struct *cstruct : *driver.structs_)
    print_struct(*cstruct);
  driver.root_->accept(*this);
}

void Printer::print_include(Include &include)
{
  std::string indent(depth_, ' ');
  if (include.system_header)
    out_ << indent << "#include <" << include.file << ">" << std::endl;
  else
    out_ << indent << "#include \"" << include.file << "\"" << std::endl;
}

void Printer::print_field(Field &field)
{
  std::string indent(depth_, ' ');
  out_ << indent << field.type;
  if (field.is_ptr)
    out_ << "*";
  if (field.array_size > 1)
    out_ << "[" << field.array_size << "]";
  out_ << " " << field.name << std::endl;
}

void Printer::print_struct(Struct &cstruct)
{
  std::string indent(depth_, ' ');
  out_ << indent << "struct " << cstruct.type << std::endl;

  ++depth_;
  for (Field *field : *cstruct.fields)
    print_field(*field);
  --depth_;
}

} // namespace ast
} // namespace bpftrace
