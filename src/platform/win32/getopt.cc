// Minimal getopt / getopt_long implementation for Windows/MSVC
#include "getopt.h"
#include <cstring>
#include <cstdio>

int   optind  = 1;
int   opterr  = 1;
int   optopt  = 0;
char* optarg  = nullptr;

static int optreset = 1;
static int optpos   = 0;

int getopt(int argc, char* const argv[], const char* optstring) {
  if (optreset || optpos == 0) {
    optreset = 0;
    optpos   = 1;
  }

  if (optind >= argc || argv[optind] == nullptr)
    return -1;

  const char* arg = argv[optind];
  if (arg[0] != '-' || arg[1] == '\0')
    return -1;

  if (arg[1] == '-' && arg[2] == '\0') { ++optind; return -1; }

  char c = arg[optpos];
  const char* p = strchr(optstring, c);
  if (!p) {
    optopt = c;
    if (opterr) fprintf(stderr, "Unknown option: -%c\n", c);
    if (arg[++optpos] == '\0') { ++optind; optpos = 1; }
    return '?';
  }

  if (p[1] == ':') {
    if (arg[optpos + 1] != '\0') {
      optarg = const_cast<char*>(&arg[optpos + 1]);
    } else if (++optind < argc) {
      optarg = argv[optind];
    } else {
      optopt = c;
      if (opterr) fprintf(stderr, "Option -%c requires an argument\n", c);
      ++optind; optpos = 1;
      return (optstring[0] == ':') ? ':' : '?';
    }
    ++optind; optpos = 1;
  } else {
    optarg = nullptr;
    if (arg[++optpos] == '\0') { ++optind; optpos = 1; }
  }
  return c;
}

int getopt_long(int argc, char* const argv[], const char* optstring,
                const struct option* longopts, int* longindex) {
  if (optind >= argc || argv[optind] == nullptr)
    return -1;

  const char* arg = argv[optind];

  if (arg[0] == '-' && arg[1] == '-' && arg[2] != '\0') {
    const char* name = arg + 2;
    const char* eq   = strchr(name, '=');
    size_t      len  = eq ? (size_t)(eq - name) : strlen(name);

    for (int i = 0; longopts[i].name != nullptr; ++i) {
      if (strncmp(longopts[i].name, name, len) == 0 && longopts[i].name[len] == '\0') {
        if (longindex) *longindex = i;
        ++optind;
        if (longopts[i].has_arg == required_argument) {
          if (eq) { optarg = const_cast<char*>(eq + 1); }
          else if (optind < argc) { optarg = argv[optind++]; }
          else {
            if (opterr) fprintf(stderr, "Option --%s requires an argument\n", longopts[i].name);
            return '?';
          }
        } else {
          optarg = eq ? const_cast<char*>(eq + 1) : nullptr;
        }
        if (longopts[i].flag) { *longopts[i].flag = longopts[i].val; return 0; }
        return longopts[i].val;
      }
    }
    if (opterr) fprintf(stderr, "Unknown option: %s\n", arg);
    ++optind;
    return '?';
  }

  return getopt(argc, argv, optstring);
}
