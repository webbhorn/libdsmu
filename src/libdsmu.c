#include <err.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <ucontext.h>

#include "b64.h"
#include "libdsmu.h"
#include "mem.h"
#include "rpc.h"

int writehandler(void *pg);
int readhandler(void *pg);
void pgfaultsh(int sig, siginfo_t *info, ucontext_t *ctx);

extern int id;  // For timing debug output.
int rfcnt;
int wfcnt;

// Signal handler state.
static struct sigaction oldact;

// Shared regions.
#define MAX_SHARED_REGIONS 100
int nextshrp;
struct sharedregion shrp[MAX_SHARED_REGIONS];

pthread_condattr_t waitca[MAX_SHARED_PAGES];
pthread_cond_t waitc[MAX_SHARED_PAGES];
pthread_mutex_t waitm[MAX_SHARED_PAGES];

static pthread_t tlisten;

// Check if the address (addr) is in a shared memory range.
// The address is in a shared memory page if it is in the same page as any
// address in the range [start, start + len).
// If it is a shared address, return 1.
// If it is not a shared address, return 0.
int sharedaddr(void *addr) {
  int i;
  int uaddr = (uintptr_t)addr;
  for (i = 0; i < nextshrp; i++) {
    if ((uaddr >= shrp[i].start) &&
	(uaddr < PGADDR(shrp[i].start + shrp[i].len + PG_SIZE))) {
      return 1;
    }
  }
  return 0;
}

// Intercept a pagefault for pages that are:
// Any other faults should be forwarded to the default handler.
void pgfaultsh(int sig, siginfo_t *info, ucontext_t *ctx) {
  void *pgaddr;

  // Ignore signals that are not segfaults.
  if (sig != SIGSEGV) {
    (oldact.sa_handler)(sig);
  }

  // Only handle faults in the shared memory region. 
  if (! sharedaddr(info->si_addr)) {
    printf("SEGFAULT not in shared memory region (was at %p)... ignoring.\n", (void *)info->si_addr);
    printf("reverting to old handler\n");
    (oldact.sa_handler)(sig);
  }

  // Dispatch fault to a read or write handler.
  pgaddr = (void *)PGADDR((uintptr_t) info->si_addr);
  if (ctx->uc_mcontext.gregs[REG_ERR] & PG_WRITE) {
    if (writehandler(pgaddr) < 0) {
      fprintf(stderr, "writehandler failed\n");
      exit(1);
    }
  } else {
    if (readhandler(pgaddr) < 0) {
      fprintf(stderr, "readhandler failed\n");
      exit(1);
    }
  }
  return;
}

// Connect to manager, etc.
// pg should be page-aligned.
// Return 0 on success.
int writehandler(void *pg) { 
  struct timeval tv;
  gettimeofday(&tv, NULL);
  double start_us = (tv.tv_sec) * 1000000 + (tv.tv_usec);

  int pgnum = PGADDR_TO_PGNUM((uintptr_t) pg);
  pthread_mutex_lock(&waitm[pgnum % MAX_SHARED_PAGES]); // Need to lock to use our condition variable.

  gettimeofday(&tv, NULL);
  double end_us = (tv.tv_sec) * 1000000 + (tv.tv_usec);
  start_us = (tv.tv_sec) * 1000000 + (tv.tv_usec);
  
  if (requestpage(pgnum, "WRITE") != 0) {
    pthread_mutex_unlock(&waitm[pgnum % MAX_SHARED_PAGES]);
    return -1;
  }

  gettimeofday(&tv, NULL);
  end_us = (tv.tv_sec) * 1000000 + (tv.tv_usec);
  start_us = (tv.tv_sec) * 1000000 + (tv.tv_usec);

  pthread_cond_wait(&waitc[pgnum % MAX_SHARED_PAGES],
    &waitm[pgnum % MAX_SHARED_PAGES]); // Wait for page message from server.
  
  gettimeofday(&tv, NULL);
  end_us = (tv.tv_sec) * 1000000 + (tv.tv_usec);
  start_us = (tv.tv_sec) * 1000000 + (tv.tv_usec);
     
  pthread_mutex_unlock(&waitm[pgnum % MAX_SHARED_PAGES]); // Unlock, allow another handler to run.

  wfcnt++;
  return 0;
}

// Connect to manager, etc.
// pg should be page-aligned.
// Return 0 on success.
int readhandler(void *pg) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  double start_us = (tv.tv_sec) * 1000000 + (tv.tv_usec);

  int pgnum = PGADDR_TO_PGNUM((uintptr_t) pg);
  pthread_mutex_lock(&waitm[pgnum % MAX_SHARED_PAGES]); // Need to lock to use our condition variable.

  gettimeofday(&tv, NULL);
  double end_us = (tv.tv_sec) * 1000000 + (tv.tv_usec);
  start_us = (tv.tv_sec) * 1000000 + (tv.tv_usec);

  if (requestpage(pgnum, "READ") != 0) {
    pthread_mutex_unlock(&waitm[pgnum % MAX_SHARED_PAGES]);
    return -1;
  }

  gettimeofday(&tv, NULL);
  end_us = (tv.tv_sec) * 1000000 + (tv.tv_usec);
  start_us = (tv.tv_sec) * 1000000 + (tv.tv_usec);

  pthread_cond_wait(&waitc[pgnum % MAX_SHARED_PAGES],
    &waitm[pgnum % MAX_SHARED_PAGES]); // Wait for page message from server.

  gettimeofday(&tv, NULL);
  end_us = (tv.tv_sec) * 1000000 + (tv.tv_usec);
  start_us = (tv.tv_sec) * 1000000 + (tv.tv_usec);

  pthread_mutex_unlock(&waitm[pgnum % MAX_SHARED_PAGES]); // Unlock, allow another handler to run.
  
  rfcnt++;
  return 0;
}

int addsharedregion(uintptr_t start, size_t len, int policy) {
  if (nextshrp >= MAX_SHARED_PAGES) {
    return -1;
  }

  if (policy & SHRPOL_INIT_ZERO) {
    int zero_fd = open("/dev/zero", O_RDONLY, 0644);
    void *p = mmap((void *)start, len, (PROT_NONE),
                   (MAP_ANON|MAP_PRIVATE), zero_fd, 0);
    if ((p < 0) || (p == NULL)) {
      fprintf(stderr, "mmap failed.\n");
      return -1;
    }
  } else {
    if ((mprotect((void *)start, PGADDR(len + PG_SIZE), PROT_NONE)) != 0) {
      return -1;
    }
  }

  struct sharedregion r = {start, len, policy};
  shrp[nextshrp] = r;
  nextshrp++;
  return 0;
}

// Test the page fault handler.
// Register the handler, setup a non-readable, non-writeable memory region.
// Try to read from it -- expect handler to run and make it readable.
// Try to write from it -- expect handler to run and make it writeable.
// Try to derefence NULL pointer -- expect handler to forward segfault to the
// default handler, which should terminate the program.
int initlibdsmu(char *ip, int port, uintptr_t starta, size_t len) {
  int i;
  struct sigaction sa;

  rfcnt = 0;
  wfcnt = 0;

  // Register page fault handler.
  sa.sa_sigaction = (void *)pgfaultsh;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_SIGINFO;
  if (sigaction(SIGSEGV, &sa, &oldact) != 0) {
    fprintf(stderr, "sigaction failed\n");
  }

  // Make condition variables/mutexes for each page.
  for (i = 0; i < MAX_SHARED_PAGES; i++) {
    pthread_condattr_init(&waitca[i]);
    pthread_cond_init(&waitc[i], &waitca[i]);
    pthread_mutex_init(&waitm[i], NULL);
  }

  // Setup shared regions.
  nextshrp = 0;
  for (i = 0; i < MAX_SHARED_REGIONS; i++) {
    struct sharedregion z = {
      .start = 0,
      .len = 0,
      .policy = 0,
    };
    shrp[i] = z;
  }

  // Setup optional initial shared memory area.
  if (addsharedregion(starta, len, SHRPOL_INIT_ZERO) < 0) {
    err(1, "Could not initialize shared region.");
  }

  // Setup sockets.
  initsocks(ip, port);

  // Spin up thread that listens for messages from manager.
  if ((pthread_create(&tlisten, NULL, listenman, NULL) != 0)) {
    fprintf(stderr, "failed to spawn listener thread\n");
    return -1;
  }

  return 0;
}

int teardownlibdsmu(void) {
  int i;

  // Clear condition variables/mutexes for each page.
  for (i = 0; i < MAX_SHARED_PAGES; i++) {
    pthread_condattr_destroy(&waitca[i]);
    pthread_cond_destroy(&waitc[i]);
    pthread_mutex_destroy(&waitm[i]);
  }

  teardownsocks();

  return 0;
}

