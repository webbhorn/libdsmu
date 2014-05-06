#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <ucontext.h>
#include <unistd.h>

#include "b64.h"
#include "libdsmu.h"
#include "mem.h"
#include "rpc.h"

int writehandler(void *pg);
int readhandler(void *pg);
void pgfaultsh(int sig, siginfo_t *info, ucontext_t *ctx);

// Signal handler state.
static struct sigaction oldact;
uintptr_t shmstarta;
size_t shmlen;

volatile int waiting[MAX_SHARED_PAGES];

static pthread_t tlisten;

// Intercept a pagefault for pages that are:
// Any other faults should be forwarded to the default handler.
void pgfaultsh(int sig, siginfo_t *info, ucontext_t *ctx) {
  void *pgaddr;

  // Ignore signals that are not segfaults.
  if (sig != SIGSEGV) {
    (oldact.sa_handler)(sig);
  }

  // Only handle faults in the shared memory region. 
  if (! ((info->si_addr >= (void *)shmstarta) &&
	 (info->si_addr < (void *)(shmstarta + shmlen)))) {
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
// For now, just change permissions to R+W.
// Return 0 on success.
int writehandler(void *pg) {
  int pgnum = PGADDR_TO_PGNUM((uintptr_t) pg);
  if (requestpage(pgnum, "WRITE") != 0) {
    return -1;
  }

//  waiting[pgnum % MAX_SHARED_PAGES] = 1;
  setwaiting(pgnum, 1);
//  while (waiting[pgnum % MAX_SHARED_PAGES] == 1)  {
  while (getwaiting(pgnum) == 1)  {
    fprintf(stderr, "[libdsmu] waiting %d\n",waiting[pgnum % MAX_SHARED_PAGES] );
    sleep(1);
  }

  return 0;
}

// Connect to manager, etc.
// pg should be page-aligned.
// For now, just change permissions to R-.
// Return 0 on success.
int readhandler(void *pg) {
  int pgnum = PGADDR_TO_PGNUM((uintptr_t) pg);
  if (requestpage(pgnum, "READ") != 0) {
    return -1;
  }

  // Wait for page message form server.
  //waiting[pgnum % MAX_SHARED_PAGES] = 1;
  setwaiting(pgnum, 1);
  //while (waiting[pgnum % MAX_SHARED_PAGES] == 1)  {
  while (getwaiting(pgnum) == 1)  {
  }

  return 0;
}

// Test the page fault handler.
// Register the handler, setup a non-readable, non-writeable memory region.
// Try to read from it -- expect handler to run and make it readable.
// Try to write from it -- expect handler to run and make it writeable.
// Try to derefence NULL pointer -- expect handler to forward segfault to the
// default handler, which should terminate the program.
int initlibdsmu(int port, uintptr_t starta, size_t len) {
  int i;
  struct sigaction sa;

  shmstarta = starta;
  shmlen = len;

  // Register page fault handler.
  sa.sa_sigaction = (void *)pgfaultsh;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_SIGINFO;
  if (sigaction(SIGSEGV, &sa, &oldact) != 0) {
    fprintf(stderr, "sigaction failed\n");
  }

  // Setup ptable.
  for (i = 0; i < MAX_SHARED_PAGES; i++) {
    waiting[i] = 0;
  }

  // Setup sockets.
  initsocks(port);

  // Spin up thread that listens for messages from manager.
  if ((pthread_create(&tlisten, NULL, listenman, NULL) != 0)) {
    printf("failed to spawn listener thread\n");
    return -1;
  }

  // Setup shared memory area.
  int zero_fd = open("/dev/zero", O_RDONLY, 0644);
  void *p = mmap((void *)starta, len, (PROT_NONE),
                 (MAP_ANON|MAP_PRIVATE), zero_fd, 0);
  if (p < 0) {
    fprintf(stderr, "mmap failed.\n");
    return 1;
  } else {
    printf("mmap succeeded.\n");
  }

  return 0;
}

int teardownlibdsmu(void) {
  teardownsocks();
  return 0;
}

void setwaiting(int page, int val) {
  waiting[page % MAX_SHARED_PAGES] = val;
}

int getwaiting(int page) {
  return waiting[page % MAX_SHARED_PAGES];
}
