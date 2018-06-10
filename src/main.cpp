#include <iostream>
#include <signal.h>

#include "bpftrace.h"
#include "codegen_llvm.h"
#include "driver.h"
#include "printer.h"
#include "semantic_analyser.h"

using namespace bpftrace;

void usage()
{
  std::cerr << "Usage:" << std::endl;
  std::cerr << "  bpftrace filename" << std::endl;
  std::cerr << "  bpftrace -e 'script'" << std::endl;
}

int main(int argc, char *argv[])
{
  int err;
  Driver driver;

  std::string script;
  bool debug = false;
  int c;
  while ((c = getopt(argc, argv, "de:")) != -1)
  {
    switch (c)
    {
      case 'd':
        debug = true;
        break;
      case 'e':
        script = optarg;
        break;
      default:
        usage();
        return 1;
    }
  }

  if (script.empty())
  {
    // There should only be 1 non-option argument (the script file)
    if (optind != argc-1)
    {
      usage();
      return 1;
    }
    char *file_name = argv[optind];
    err = driver.parse_file(file_name);
  }
  else
  {
    // Script is provided as a command line argument
    if (optind != argc)
    {
      usage();
      return 1;
    }
    err = driver.parse_str(script);
  }

  if (err)
    return err;

  BPFtrace bpftrace;

  if (debug)
  {
    ast::Printer p(std::cout);
    p.print_all(driver);
  }

  ast::SemanticAnalyser semantics(driver.root_, bpftrace);
  err = semantics.analyse();
  if (err)
    return err;

  err = semantics.create_maps(debug);
  if (err)
    return err;

  ast::CodegenLLVM llvm(driver.root_, bpftrace);
  err = llvm.compile(debug);
  if (err)
    return err;

  if (debug)
    return 0;

  // Empty signal handler for cleanly terminating the program
  struct sigaction act;
  act.sa_handler = [](int) { };
  sigaction(SIGINT, &act, NULL);

  int num_probes = bpftrace.num_probes();
  if (num_probes == 0)
  {
    std::cout << "No probes to attach" << std::endl;
    return 1;
  }
  else if (num_probes == 1)
    std::cout << "Attaching " << bpftrace.num_probes() << " probe..." << std::endl;
  else
    std::cout << "Attaching " << bpftrace.num_probes() << " probes..." << std::endl;

  err = bpftrace.run();
  if (err)
    return err;

  std::cout << "\n\n";

  err = bpftrace.print_maps();
  if (err)
    return err;

  return 0;
}
