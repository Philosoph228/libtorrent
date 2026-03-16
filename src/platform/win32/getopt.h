#pragma once
// getopt / getopt_long for Windows (minimal implementation)
#include <cstring>
#include <cstdio>

extern "C" {

extern int   optind;
extern int   opterr;
extern int   optopt;
extern char* optarg;

struct option {
  const char* name;
  int         has_arg;
  int*        flag;
  int         val;
};

#define no_argument       0
#define required_argument 1
#define optional_argument 2

int getopt(int argc, char* const argv[], const char* optstring);
int getopt_long(int argc, char* const argv[], const char* optstring,
                const struct option* longopts, int* longindex);

} // extern "C"
