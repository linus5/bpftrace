#include <sstream>

#include "gtest/gtest.h"
#include "driver.h"
#include "printer.h"

namespace bpftrace {
namespace test {
namespace parser {

using Printer = ast::Printer;

void test(const std::string &input, const std::string &output)
{
  Driver driver;
  ASSERT_EQ(driver.parse_str(input), 0);

  std::ostringstream out;
  Printer printer(out);
  printer.print_all(driver);
  EXPECT_EQ(output, out.str());
}

TEST(Parser, builtin_variables)
{
  test("kprobe:f { pid }", "Program\n kprobe:f\n  builtin: pid\n");
  test("kprobe:f { tid }", "Program\n kprobe:f\n  builtin: tid\n");
  test("kprobe:f { uid }", "Program\n kprobe:f\n  builtin: uid\n");
  test("kprobe:f { gid }", "Program\n kprobe:f\n  builtin: gid\n");
  test("kprobe:f { nsecs }", "Program\n kprobe:f\n  builtin: nsecs\n");
  test("kprobe:f { cpu }", "Program\n kprobe:f\n  builtin: cpu\n");
  test("kprobe:f { comm }", "Program\n kprobe:f\n  builtin: comm\n");
  test("kprobe:f { stack }", "Program\n kprobe:f\n  builtin: stack\n");
  test("kprobe:f { ustack }", "Program\n kprobe:f\n  builtin: ustack\n");
  test("kprobe:f { arg0 }", "Program\n kprobe:f\n  builtin: arg0\n");
  test("kprobe:f { retval }", "Program\n kprobe:f\n  builtin: retval\n");
  test("kprobe:f { func }", "Program\n kprobe:f\n  builtin: func\n");
}

TEST(Parser, map_assign)
{
  test("kprobe:sys_open { @x = 1; }",
      "Program\n"
      " kprobe:sys_open\n"
      "  =\n"
      "   map: @x\n"
      "   int: 1\n");
  test("kprobe:sys_open { @x = @y; }",
      "Program\n"
      " kprobe:sys_open\n"
      "  =\n"
      "   map: @x\n"
      "   map: @y\n");
  test("kprobe:sys_open { @x = arg0; }",
      "Program\n"
      " kprobe:sys_open\n"
      "  =\n"
      "   map: @x\n"
      "   builtin: arg0\n");
  test("kprobe:sys_open { @x = count(); }",
      "Program\n"
      " kprobe:sys_open\n"
      "  =\n"
      "   map: @x\n"
      "   call: count\n");
  test("kprobe:sys_open { @x = \"mystring\" }",
      "Program\n"
      " kprobe:sys_open\n"
      "  =\n"
      "   map: @x\n"
      "   string: mystring\n");
  test("kprobe:sys_open { @x = $myvar; }",
      "Program\n"
      " kprobe:sys_open\n"
      "  =\n"
      "   map: @x\n"
      "   variable: $myvar\n");
}

TEST(Parser, variable_assign)
{
  test("kprobe:sys_open { $x = 1; }",
      "Program\n"
      " kprobe:sys_open\n"
      "  =\n"
      "   variable: $x\n"
      "   int: 1\n");
}

TEST(Parser, map_key)
{
  test("kprobe:sys_open { @x[0] = 1; @x[0,1,2] = 1; }",
      "Program\n"
      " kprobe:sys_open\n"
      "  =\n"
      "   map: @x\n"
      "    int: 0\n"
      "   int: 1\n"
      "  =\n"
      "   map: @x\n"
      "    int: 0\n"
      "    int: 1\n"
      "    int: 2\n"
      "   int: 1\n");

  test("kprobe:sys_open { @x[@a] = 1; @x[@a,@b,@c] = 1; }",
      "Program\n"
      " kprobe:sys_open\n"
      "  =\n"
      "   map: @x\n"
      "    map: @a\n"
      "   int: 1\n"
      "  =\n"
      "   map: @x\n"
      "    map: @a\n"
      "    map: @b\n"
      "    map: @c\n"
      "   int: 1\n");

  test("kprobe:sys_open { @x[pid] = 1; @x[tid,uid,arg9] = 1; }",
      "Program\n"
      " kprobe:sys_open\n"
      "  =\n"
      "   map: @x\n"
      "    builtin: pid\n"
      "   int: 1\n"
      "  =\n"
      "   map: @x\n"
      "    builtin: tid\n"
      "    builtin: uid\n"
      "    builtin: arg9\n"
      "   int: 1\n");
}

TEST(Parser, predicate)
{
  test("kprobe:sys_open / @x / { 1; }",
      "Program\n"
      " kprobe:sys_open\n"
      "  pred\n"
      "   map: @x\n"
      "  int: 1\n");
}

TEST(Parser, predicate_containing_division)
{
  test("kprobe:sys_open /100/25/ { 1; }",
      "Program\n"
      " kprobe:sys_open\n"
      "  pred\n"
      "   /\n"
      "    int: 100\n"
      "    int: 25\n"
      "  int: 1\n");
}

TEST(Parser, expressions)
{
  test("kprobe:sys_open / 1 <= 2 && (9 - 4 != 5*10 || ~0) || comm == \"string\" /\n"
       "{\n"
       "  1;\n"
       "}",
      "Program\n"
      " kprobe:sys_open\n"
      "  pred\n"
      "   ||\n"
      "    &&\n"
      "     <=\n"
      "      int: 1\n"
      "      int: 2\n"
      "     ||\n"
      "      !=\n"
      "       -\n"
      "        int: 9\n"
      "        int: 4\n"
      "       *\n"
      "        int: 5\n"
      "        int: 10\n"
      "      ~\n"
      "       int: 0\n"
      "    ==\n"
      "     builtin: comm\n"
      "     string: string\n"
      "  int: 1\n");
}

TEST(Parser, call)
{
  test("kprobe:sys_open { @x = count(); @y = quantize(1,2,3); delete(@x); }",
      "Program\n"
      " kprobe:sys_open\n"
      "  =\n"
      "   map: @x\n"
      "   call: count\n"
      "  =\n"
      "   map: @y\n"
      "   call: quantize\n"
      "    int: 1\n"
      "    int: 2\n"
      "    int: 3\n"
      "  call: delete\n"
      "   map: @x\n");
}

TEST(Parser, call_unknown_function)
{
  test("kprobe:sys_open { myfunc() }",
      "Program\n"
      " kprobe:sys_open\n"
      "  call: myfunc\n");
}

TEST(Parser, multiple_probes)
{
  test("kprobe:sys_open { 1; } kretprobe:sys_open { 2; }",
      "Program\n"
      " kprobe:sys_open\n"
      "  int: 1\n"
      " kretprobe:sys_open\n"
      "  int: 2\n");
}

TEST(Parser, uprobe)
{
  test("uprobe:/my/program:func { 1; }",
      "Program\n"
      " uprobe:/my/program:func\n"
      "  int: 1\n");
}

TEST(Parser, escape_chars)
{
  test("kprobe:sys_open { \"newline\\nand tab\\tbackslash\\\\quote\\\"here\" }",
      "Program\n"
      " kprobe:sys_open\n"
      "  string: newline\\nand tab\\tbackslash\\\\quote\\\"here\n");
}

TEST(Parser, begin_probe)
{
  test("BEGIN { 1 }",
      "Program\n"
      " BEGIN\n"
      "  int: 1\n");
}

TEST(Parser, tracepoint_probe)
{
  test("tracepoint:sched:sched_switch { 1 }",
      "Program\n"
      " tracepoint:sched:sched_switch\n"
      "  int: 1\n");
}

TEST(Parser, profile_probe)
{
  test("profile:ms:997 { 1 }",
      "Program\n"
      " profile:ms:997\n"
      "  int: 1\n");
}

TEST(Parser, multiple_attach_points_kprobe)
{
  test("BEGIN,kprobe:sys_open,uprobe:/bin/sh:foo,tracepoint:syscalls:sys_enter_* { 1 }",
      "Program\n"
      " BEGIN\n"
      " kprobe:sys_open\n"
      " uprobe:/bin/sh:foo\n"
      " tracepoint:syscalls:sys_enter_*\n"
      "  int: 1\n");
}

TEST(Parser, character_class_attach_point)
{
  test("kprobe:[Ss]y[Ss]_read { 1 }",
      "Program\n"
      " kprobe:[Ss]y[Ss]_read\n"
      "  int: 1\n");
}

TEST(Parser, wildcard_attach_points)
{
  test("kprobe:sys_* { 1 }",
      "Program\n"
      " kprobe:sys_*\n"
      "  int: 1\n");
  test("kprobe:*blah { 1 }",
      "Program\n"
      " kprobe:*blah\n"
      "  int: 1\n");
  test("kprobe:sys*blah { 1 }",
      "Program\n"
      " kprobe:sys*blah\n"
      "  int: 1\n");
  test("kprobe:* { 1 }",
      "Program\n"
      " kprobe:*\n"
      "  int: 1\n");
  test("kprobe:sys_* { @x = cpu*retval }",
      "Program\n"
      " kprobe:sys_*\n"
      "  =\n"
      "   map: @x\n"
      "   *\n"
      "    builtin: cpu\n"
      "    builtin: retval\n");
  test("kprobe:sys_* { @x = *arg0 }",
      "Program\n"
      " kprobe:sys_*\n"
      "  =\n"
      "   map: @x\n"
      "   dereference\n"
      "    builtin: arg0\n");
}

TEST(Parser, short_map_name)
{
  test("kprobe:sys_read { @ = 1 }",
      "Program\n"
      " kprobe:sys_read\n"
      "  =\n"
      "   map: @\n"
      "   int: 1\n");
}

TEST(Parser, include)
{
  test("#include <stdio.h> kprobe:sys_read { @x = 1 }",
      "#include <stdio.h>\n"
      "Program\n"
      " kprobe:sys_read\n"
      "  =\n"
      "   map: @x\n"
      "   int: 1\n");
}

TEST(Parser, include_quote)
{
  test("#include \"stdio.h\" kprobe:sys_read { @x = 1 }",
      "#include \"stdio.h\"\n"
      "Program\n"
      " kprobe:sys_read\n"
      "  =\n"
      "   map: @x\n"
      "   int: 1\n");
}

TEST(Parser, include_multiple)
{
  test("#include <stdio.h> #include \"blah\" #include <foo.h> kprobe:sys_read { @x = 1 }",
      "#include <stdio.h>\n"
      "#include \"blah\"\n"
      "#include <foo.h>\n"
      "Program\n"
      " kprobe:sys_read\n"
      "  =\n"
      "   map: @x\n"
      "   int: 1\n");
}

TEST(Parser, brackets)
{
  test("kprobe:sys_read { (arg0*arg1) }",
      "Program\n"
      " kprobe:sys_read\n"
      "  *\n"
      "   builtin: arg0\n"
      "   builtin: arg1\n");
}

TEST(Parser, cast)
{
  test("kprobe:sys_read { (mytype)arg0; }",
      "Program\n"
      " kprobe:sys_read\n"
      "  (mytype)\n"
      "   builtin: arg0\n");
}

TEST(Parser, cast_ptr)
{
  test("kprobe:sys_read { (mytype*)arg0; }",
      "Program\n"
      " kprobe:sys_read\n"
      "  (mytype*)\n"
      "   builtin: arg0\n");
}

TEST(Parser, cast_or_expr1)
{
  test("kprobe:sys_read { (mytype)*arg0; }",
      "Program\n"
      " kprobe:sys_read\n"
      "  (mytype)\n"
      "   dereference\n"
      "    builtin: arg0\n");
}

TEST(Parser, cast_or_expr2)
{
  test("kprobe:sys_read { (arg1)*arg0; }",
      "Program\n"
      " kprobe:sys_read\n"
      "  *\n"
      "   builtin: arg1\n"
      "   builtin: arg0\n");
}

TEST(Parser, cast_precedence)
{
  test("kprobe:sys_read { (mytype)arg0.field; }",
      "Program\n"
      " kprobe:sys_read\n"
      "  (mytype)\n"
      "   .\n"
      "    builtin: arg0\n"
      "    field\n");

  test("kprobe:sys_read { (mytype*)arg0->field; }",
      "Program\n"
      " kprobe:sys_read\n"
      "  (mytype*)\n"
      "   .\n"
      "    dereference\n"
      "     builtin: arg0\n"
      "    field\n");

  test("kprobe:sys_read { (mytype)arg0+123; }",
      "Program\n"
      " kprobe:sys_read\n"
      "  +\n"
      "   (mytype)\n"
      "    builtin: arg0\n"
      "   int: 123\n");
}

TEST(Parser, dereference_precedence)
{
  test("kprobe:sys_read { *@x+1 }",
      "Program\n"
      " kprobe:sys_read\n"
      "  +\n"
      "   dereference\n"
      "    map: @x\n"
      "   int: 1\n");

  test("kprobe:sys_read { *@x**@y }",
      "Program\n"
      " kprobe:sys_read\n"
      "  *\n"
      "   dereference\n"
      "    map: @x\n"
      "   dereference\n"
      "    map: @y\n");

  test("kprobe:sys_read { *@x*@y }",
      "Program\n"
      " kprobe:sys_read\n"
      "  *\n"
      "   dereference\n"
      "    map: @x\n"
      "   map: @y\n");

  test("kprobe:sys_read { *@x.myfield }",
      "Program\n"
      " kprobe:sys_read\n"
      "  dereference\n"
      "   .\n"
      "    map: @x\n"
      "    myfield\n");
}

TEST(Parser, field_access)
{
  test("kprobe:sys_read { @x.myfield; }",
      "Program\n"
      " kprobe:sys_read\n"
      "  .\n"
      "   map: @x\n"
      "   myfield\n");

  test("kprobe:sys_read { @x->myfield; }",
      "Program\n"
      " kprobe:sys_read\n"
      "  .\n"
      "   dereference\n"
      "    map: @x\n"
      "   myfield\n");
}

TEST(Parser, field_access_builtin)
{
  test("kprobe:sys_read { @x.count; }",
      "Program\n"
      " kprobe:sys_read\n"
      "  .\n"
      "   map: @x\n"
      "   count\n");

  test("kprobe:sys_read { @x->count; }",
      "Program\n"
      " kprobe:sys_read\n"
      "  .\n"
      "   dereference\n"
      "    map: @x\n"
      "   count\n");
}

TEST(Parser, cstruct_int)
{
  test("struct Foo { int n; } kprobe:sys_read { 1 }",
      "struct Foo\n"
      " int n\n"
      "Program\n"
      " kprobe:sys_read\n"
      "  int: 1\n");
}

TEST(Parser, cstruct_int_ptr)
{
  test("struct Foo { int *n; } kprobe:sys_read { 1 }",
      "struct Foo\n"
      " int* n\n"
      "Program\n"
      " kprobe:sys_read\n"
      "  int: 1\n");
}

TEST(Parser, cstruct_int_multi)
{
  test("struct Foo { int n; int a,*b,c; } kprobe:sys_read { 1 }",
      "struct Foo\n"
      " int n\n"
      " int a\n"
      " int* b\n"
      " int c\n"
      "Program\n"
      " kprobe:sys_read\n"
      "  int: 1\n");
}

TEST(Parser, cstruct_char_ptr)
{
  test("struct Foo { char *str; } kprobe:sys_read { 1 }",
      "struct Foo\n"
      " char* str\n"
      "Program\n"
      " kprobe:sys_read\n"
      "  int: 1\n");
}

TEST(Parser, cstruct_char_array_ptr)
{
  test("struct Foo { char *str[32]; } kprobe:sys_read { 1 }",
      "struct Foo\n"
      " char*[32] str\n"
      "Program\n"
      " kprobe:sys_read\n"
      "  int: 1\n");
}

TEST(Parser, cstruct_char_array)
{
  test("struct Foo { char str[32]; } kprobe:sys_read { 1 }",
      "struct Foo\n"
      " char[32] str\n"
      "Program\n"
      " kprobe:sys_read\n"
      "  int: 1\n");
}

TEST(Parser, cstruct_containing_struct)
{
  test("struct Foo { struct Bar b; struct Car *c,d; } kprobe:sys_read { 1 }",
      "struct Foo\n"
      " Bar b\n"
      " Car* c\n"
      " Car d\n"
      "Program\n"
      " kprobe:sys_read\n"
      "  int: 1\n");
}

} // namespace parser
} // namespace test
} // namespace bpftrace
