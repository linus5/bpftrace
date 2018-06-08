#pragma once

#include <string>
#include <vector>

#include "types.h"

namespace bpftrace {
namespace ast {

class Visitor;

class Node {
public:
  virtual ~Node() { }
  virtual void accept(Visitor &v) = 0;
};

class Map;
class Variable;
class Expression : public Node {
public:
  SizedType type;
  Map *map = nullptr; // Only set when this expression is assigned to a map
  Variable *var = nullptr; // Set when this expression is assigned to a variable
  bool is_literal = false;
  bool is_variable = false;
};
using ExpressionList = std::vector<Expression *>;

class Integer : public Expression {
public:
  explicit Integer(int n) : n(n) { is_literal = true; }
  int n;

  void accept(Visitor &v) override;
};

class String : public Expression {
public:
  explicit String(std::string str) : str(str) { is_literal = true; }
  std::string str;

  void accept(Visitor &v) override;
};

class Builtin : public Expression {
public:
  explicit Builtin(std::string ident) : ident(ident) { }
  std::string ident;

  void accept(Visitor &v) override;
};

class Call : public Expression {
public:
  explicit Call(std::string &func) : func(func), vargs(nullptr) { }
  Call(std::string &func, ExpressionList *vargs) : func(func), vargs(vargs) { }
  std::string func;
  ExpressionList *vargs;

  void accept(Visitor &v) override;
};

class Map : public Expression {
public:
  explicit Map(std::string &ident) : ident(ident), vargs(nullptr) { }
  Map(std::string &ident, ExpressionList *vargs) : ident(ident), vargs(vargs) { }
  std::string ident;
  ExpressionList *vargs;

  void accept(Visitor &v) override;
};

class Variable : public Expression {
public:
  explicit Variable(std::string &ident) : ident(ident) { is_variable = true; }
  std::string ident;

  void accept(Visitor &v) override;
};

class Binop : public Expression {
public:
  Binop(Expression *left, int op, Expression *right) : left(left), right(right), op(op) { }
  Expression *left, *right;
  int op;

  void accept(Visitor &v) override;
};

class Unop : public Expression {
public:
  Unop(int op, Expression *expr) : expr(expr), op(op) { }
  Expression *expr;
  int op;

  void accept(Visitor &v) override;
};

class FieldAccess : public Expression {
public:
  FieldAccess(Expression *expr, const std::string &field) : expr(expr), field(field) { }
  Expression *expr;
  std::string field;

  void accept(Visitor &v) override;
};

class Cast : public Expression {
public:
  Cast(const std::string &type, Expression *expr) : cast_type(type), expr(expr) { }
  std::string cast_type;
  Expression *expr;

  void accept(Visitor &v) override;
};

class Statement : public Node {
};
using StatementList = std::vector<Statement *>;

class ExprStatement : public Statement {
public:
  explicit ExprStatement(Expression *expr) : expr(expr) { }
  Expression *expr;

  void accept(Visitor &v) override;
};

class AssignMapStatement : public Statement {
public:
  AssignMapStatement(Map *map, Expression *expr) : map(map), expr(expr) {
    expr->map = map;
  }
  Map *map;
  Expression *expr;

  void accept(Visitor &v) override;
};

class AssignVarStatement : public Statement {
public:
  AssignVarStatement(Variable *var, Expression *expr) : var(var), expr(expr) {
    expr->var = var;
  }
  Variable *var;
  Expression *expr;

  void accept(Visitor &v) override;
};

class Predicate : public Node {
public:
  explicit Predicate(Expression *expr) : expr(expr) { }
  Expression *expr;

  void accept(Visitor &v) override;
};

class AttachPoint : public Node {
public:
  explicit AttachPoint(const std::string &provider)
    : provider(provider) { }
  AttachPoint(const std::string &provider,
              const std::string &func)
    : provider(provider), func(func) { }
  AttachPoint(const std::string &provider,
              const std::string &target,
              const std::string &func)
    : provider(provider), target(target), func(func) { }
  AttachPoint(const std::string &provider,
              const std::string &target,
              int freq)
    : provider(provider), target(target), freq(freq) { }

  std::string provider;
  std::string target;
  std::string func;
  int freq = 0;

  void accept(Visitor &v) override;
  std::string name(const std::string &attach_point) const;
};
using AttachPointList = std::vector<AttachPoint *>;

class Probe : public Node {
public:
  Probe(AttachPointList *attach_points, Predicate *pred, StatementList *stmts)
    : attach_points(attach_points), pred(pred), stmts(stmts) { }

  AttachPointList *attach_points;
  Predicate *pred;
  StatementList *stmts;

  void accept(Visitor &v) override;
  std::string name() const;
};
using ProbeList = std::vector<Probe *>;

class Include : public Node {
public:
  explicit Include(const std::string &file, bool system_header)
    : file(file), system_header(system_header) { }
  std::string file;
  bool system_header;

  void accept(Visitor &v) override;
};
using IncludeList = std::vector<Include *>;

class Program : public Node {
public:
  Program(IncludeList *includes, ProbeList *probes)
    : includes(includes), probes(probes) { }
  IncludeList *includes;
  ProbeList *probes;

  void accept(Visitor &v) override;
};

class Visitor {
public:
  virtual ~Visitor() { }
  virtual void visit(Integer &integer) = 0;
  virtual void visit(String &string) = 0;
  virtual void visit(Builtin &builtin) = 0;
  virtual void visit(Call &call) = 0;
  virtual void visit(Map &map) = 0;
  virtual void visit(Variable &var) = 0;
  virtual void visit(Binop &binop) = 0;
  virtual void visit(Unop &unop) = 0;
  virtual void visit(FieldAccess &acc) = 0;
  virtual void visit(Cast &cast) = 0;
  virtual void visit(ExprStatement &expr) = 0;
  virtual void visit(AssignMapStatement &assignment) = 0;
  virtual void visit(AssignVarStatement &assignment) = 0;
  virtual void visit(Predicate &pred) = 0;
  virtual void visit(AttachPoint &ap) = 0;
  virtual void visit(Probe &probe) = 0;
  virtual void visit(Include &include) = 0;
  virtual void visit(Program &program) = 0;
};

std::string opstr(Binop &binop);
std::string opstr(Unop &unop);

} // namespace ast
} // namespace bpftrace
