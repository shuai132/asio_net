#pragma once

#include <cstdio>
#include <cstdlib>

// clang-format off

#define __ASSERT(e, file, line) \
  ((void)std::fprintf(stderr, "%s:%d: failed assertion `%s'\n", file, line, e), std::exit(EXIT_FAILURE))

#define ASSERT(e) ((void)((e) ? ((void)0) : __ASSERT(#e, __FILE__, __LINE__)))
