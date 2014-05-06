#ifndef _LIBDSMU_H_
#define _LIBDSMU_H_

#include <signal.h>
#include <stdint.h>
#include <ucontext.h>

//
// Initialize distributed shared memory.
// The manager is listening on port.
// Shared memory will begin at starta and will include all pages that include
// addresses in the range [starta, starta + len).
//
int initlibdsmu(int port, uintptr_t starta, size_t len);

int teardownlibdsmu(void);

void setwaiting(int page, int val);

int getwaiting(int page);

#define SHRPOL_NONE (0)
#define SHRPOL_INIT_ZERO (1 << 0)

struct sharedregion {
  uintptr_t start;
  size_t len;
  uint16_t policy;
};

#endif  // _LIBDSMU_H_
