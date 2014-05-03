#ifndef _LIBDSMU_H_
#define _LIBDSMU_H_

#include <signal.h>
#include <stdint.h>
#include <ucontext.h>

int initlibdsmu(void);
int teardownlibdsmu(void);

#endif  // _LIBDSMU_H_
