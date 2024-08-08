#pragma once

#include <stdlib.h>

#ifdef NDEBUG
#define dbgPrint(fmt, ...)
#else
#define dbgPrint(fmt, ...) fprintf (stderr, fmt, ##__VA_ARGS__);
#endif

#ifdef NDEBUG
#define dbgPrintExt(fmt, ...)
#else
#define dbgPrintExt(fmt, ...) fprintf (stderr, fmt " in %s:%d\n", ##__VA_ARGS__, __FILE__, __LINE__);
#endif

// Logs error to stderr.
#define logError(fmt, ...) fprintf (stderr, fmt " in %s:%d\n", ##__VA_ARGS__, __FILE__, __LINE__);

// Logs error to stderr and exits.
#define logExit(fmt, ...)                                                                                              \
  fprintf (stderr, fmt " in %s:%d\n", ##__VA_ARGS__, __FILE__, __LINE__);                                              \
  exit (EXIT_FAILURE);
