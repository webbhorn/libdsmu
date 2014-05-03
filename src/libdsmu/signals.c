#include "b64/cencode.h"

#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <ucontext.h>
#include <unistd.h>

#define REG_ERR 19
#define PG_WRITE 0x2
#define PG_PRESENT 0x1
#define PG_BITS 12
#define PG_SIZE (1 << (PG_BITS))

// Signal handler state.
static struct sigaction oldact;

// Function declarations.
int writehandler(void *pg);
int readhandler(void *pg);

// Socket state.
int serverfd;
struct addrinfo *resolvedAddr;
struct addrinfo hints;

// Convert address to the start of the page.
static inline uintptr_t PGADDR(uintptr_t addr) {
  return addr & ~(PG_SIZE - 1);
}

// Convert page number to address of start of page.
static inline uintptr_t PGNUM_TO_PGADDR(uintptr_t pgnum) {
  return pgnum << PG_BITS;
}

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

  // Only handle faults on the heap (or a special area).
  if (! ((info->si_addr >= (void *)0x12340000) &&
	 (info->si_addr < (void *)(0x12340000 + 4000)))) {
    printf("segfault was not on heap... ignoring.\n");
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

// Encode a block of data in starta of length sz into a base64 string, stored
// in outbuf.
int b64encode(const char *starta, unsigned int sz, char *outbuf) {
  base64_encodestate s;
  base64_init_encodestate(&s);
  int cnt = base64_encode_block(starta, sz, outbuf, &s);
  cnt += base64_encode_blockend(outbuf + cnt, &s);
  return cnt;
}

// Send a message to the manager.
int sendman(char *str, int len) {
  // Acquire manager socket lock.
  return send(serverfd, str, len, 0);
}

// Connect to manager, etc.
// pg should be page-aligned.
// For now, just change permissions to R+W.
// Return 0 on success.
int writehandler(void *pg) {
  printf("Entering writehandler...\n");
  if (mprotect(pg, PG_SIZE, (PROT_READ|PROT_WRITE)) == 0) {
    char pgb64[PG_SIZE * 2] = {0};
    int errcnt = b64encode((const char *)pg, PG_SIZE, pgb64);

    char msg[PG_SIZE * 2];
    snprintf(msg, PG_SIZE * 3, "READ FAULT AT %p, %s", pg, pgb64);
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
  if (mprotect(pg, PG_SIZE, PROT_READ) == 0) {
    return 0;
  }
  return -1;
}

// Initialize socket with manager.
// Return 0 on success.
int initsocks(void) {
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  if (getaddrinfo(NULL, "4444", &hints, &resolvedAddr) < 0)
    return -2;

  serverfd = socket(resolvedAddr->ai_family, resolvedAddr->ai_socktype,
                        resolvedAddr->ai_protocol);
  if (serverfd < 0)
    return -2;

  if (connect(serverfd, resolvedAddr->ai_addr, resolvedAddr->ai_addrlen) < 0)
    return -2;
}

// Cleanup sockets.
int teardownsocks(void) {
  close(serverfd);
}

void confirminvalidate(int pgnum) {
  char msg[100] = {0};
  snprintf(msg, 100, "INVALIDATE CONFIRMATION %d", pgnum);
  sendman(msg, strlen(msg));
}

// Handle invalidate messages.
int invalidate(char *msg) {
  char *spgnum = strtok(msg, " ");
  int pgnum = atoi(spgnum);
  printf(">> Invalidate page number %d\n", pgnum);
  void *pg = (void *)PGNUM_TO_PGADDR((uintptr_t)pgnum);
  if (mprotect(pg, 1, PROT_NONE) != 0) {
    fprintf(stderr, "Invalidation of page addr %p failed\n", pg);
    return -1;
  }
  printf("Successfully invalidated page at addr %p\n", pg);
  confirminvalidate(pgnum);
  return 0;
}

// Handle newly arrived messages.
int dispatch(char *msg) {
  if (strstr(msg, "INVALIDATE") != NULL) {
    invalidate(msg);
  }
}

// Listen for manager messages and dispatch them.
void *listenman(void *ptr) {
  printf("Listening...\n");
  while (1) {
    char buf[7000] = {0};
    ssize_t err = recv(serverfd, (void *)buf, 7000, 0);
    dispatch(buf);
  }
}

// Test the page fault handler.
// Register the handler, setup a non-readable, non-writeable memory region.
// Try to read from it -- expect handler to run and make it readable.
// Try to write from it -- expect handler to run and make it writeable.
// Try to derefence NULL pointer -- expect handler to forward segfault to the
// default handler, which should terminate the program.
int main(void) {
  struct sigaction sa;

  // Register page fault handler.
  sa.sa_sigaction = (void *)pgfaultsh;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_SIGINFO;
  if (sigaction(SIGSEGV, &sa, &oldact) != 0) {
    fprintf(stderr, "sigaction failed\n");
  }

  // Setup sockets.
  initsocks();

  // Spin up thread that listens for messages from manager.
  pthread_t tlisten;
  if ((pthread_create(&tlisten, NULL, listenman, NULL) != 0)) {
    printf("failed to spawn listener thread\n");
    return -1;
  }

  // Setup test memory area.
  int zero_fd = open("/dev/zero", O_RDONLY, 0644);
  void *p = mmap((void *)0x12340000, 0x4000, (PROT_NONE),
                 (MAP_ANON|MAP_PRIVATE), zero_fd, 0);
  if (p < 0) {
    fprintf(stderr, "mmap failed.\n");
    return 1;
  } else {
    printf("mmap succeeded.\n");
  }

  // Trigger a read fault, then a write fault.
  printf("Will try to read %p\n", ((int *)p) + 612);
  printf("p[612] is %d\n", ((int *)p)[612]);
  ((int *)p)[612] = 3;
  printf("p[612] is now %d\n", ((int *)p)[612]);

  pthread_join(tlisten, NULL);

  // Cleanup.
  teardownsocks();

  return 0;
}

