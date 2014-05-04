#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <ucontext.h>

#include "b64.h"
#include "libdsmu.h"
#include "mem.h"
#include "rpc.h"

int writehandler(void *pg);
int readhandler(void *pg);
void pgfaultsh(int sig, siginfo_t *info, ucontext_t *ctx);

pthread_mutex_t ptable[MAX_SHARED_PAGES];
pthread_cond_t ptablec[MAX_SHARED_PAGES];

// Signal handler state.
static struct sigaction oldact;
uintptr_t shmstarta;
size_t shmlen;

static pthread_t tlisten;

// Intercept a pagefault for pages that are:
// Any other faults should be forwarded to the default handler.
void pgfaultsh(int sig, siginfo_t *info, ucontext_t *ctx) {
  void *pgaddr;

  // Ignore signals that are not segfaults.
  if (sig != SIGSEGV) {
    printf("signal is not a segfault... ignoring.\n");
    printf("reverting to old handler\n");
    (oldact.sa_handler)(sig);
  }

  // Only handle faults in the shared memory region. 
  if (! ((info->si_addr >= (void *)shmstarta) &&
	 (info->si_addr < (void *)(shmstarta + shmlen)))) {
    printf("segfault was not in shared memory region... ignoring.\n");
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

  printf("page fault handled successfully.\n");
  return;
}

// Connect to manager, etc.
// pg should be page-aligned.
// For now, just change permissions to R+W.
// Return 0 on success.
int writehandler(void *pg) {
  printf("Entering writehandler...\n");
  if (mprotect(pg, PG_SIZE, (PROT_READ|PROT_WRITE)) == 0) {
    char pgb64[PG_SIZE * 2] = {0};
    b64encode((const char *)pg, PG_SIZE, pgb64);

    char msg[PG_SIZE * 2];
    snprintf(msg, PG_SIZE * 3, "WRITE FAULT AT %p, %s", pg, pgb64);
    if (sendman(msg, strlen(msg)) < 0)
      return -3;
    printf("done talking to server\n");

    return 0;
  }
  return -1;
}

// Connect to manager, etc.
// pg should be page-aligned.
// For now, just change permissions to R-.
// Return 0 on success.
int readhandler(void *pg) {
  printf("Entering readhandler...\n");

  int pgnum = PGADDR_TO_PGNUM((uintptr_t) pg);

  printf("Acquiring lock for page number %d\n", pgnum);

  pthread_mutex_lock(&ptable[pgnum % MAX_SHARED_PAGES]);

  printf("Trying to read page number %d\n", pgnum);
  
  printf("Sending manager read request for page number %d\n", pgnum);

  if (readrequestpage(pgnum) != 0) {
    pthread_mutex_unlock(&ptable[pgnum % MAX_SHARED_PAGES]);
    return -1;
  }

  printf("Waiting for manager to send page number %d\n", pgnum);

  // Wait for page message form server.
  if (pthread_cond_wait(&ptablec[pgnum % MAX_SHARED_PAGES],
	                &ptable[pgnum % MAX_SHARED_PAGES]) != 0) {
    return -1;
  }

  // Set permissions.
  if (mprotect(pg, PG_SIZE, PROT_READ) != 0) {
    pthread_mutex_unlock(&ptable[pgnum]);
    return -1;
  }

  pthread_mutex_unlock(&ptable[pgnum]);
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
    pthread_mutex_init(&ptable[i], NULL);
    pthread_cond_init(&ptablec[i], NULL);
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
  int i;
  teardownsocks();
  for (i = 0; i < MAX_SHARED_PAGES; i++) {
    pthread_cond_destroy(&ptablec[i]);
    pthread_mutex_destroy(&ptable[i]);
  }
  return 0;
}

