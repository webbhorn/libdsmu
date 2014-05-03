#ifndef _LIBDSMU_H_
#define _LIBDSMU_H_

#include <signal.h>
#include <stdint.h>
#include <ucontext.h>

int writehandler(void *pg);

int readhandler(void *pg);

void pgfaultsh(int sig, siginfo_t *info, ucontext_t *ctx);

#endif  // _LIBDSMU_H_
