#include <err.h>
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

#include "b64.h"
#include "libdsmu.h"
#include "mem.h"
#include "rpc.h"

volatile int waiting[MAX_SHARED_PAGES];

// FORMERLY IN RPC.C/RPC.H
// Socket state.
int serverfd;
struct addrinfo *resolvedAddr;
struct addrinfo hints;

pthread_mutex_t sockl;

// Listen for manager messages and dispatch them.
void *listenman(void *ptr) {
  printf("Listening...\n");
  while (1) {
    char buf[7000] = {0};
    if (recv(serverfd, (void *)buf, 7000, 0) > 0) {
        dispatch(buf);
    }
  }
}

// Handle newly arrived messages.
int dispatch(char *msg) {
#ifdef DEBUG
  printf("< %.40s\n", msg);
#endif  // DEBUG
  if (strstr(msg, "INVALIDATE") != NULL) {
    invalidate(msg);
  } else if (strstr(msg, "REQUESTPAGE") != NULL) {
    handleconfirm(msg);
  } else {
    printf("Undefined message.\n");
  }
  return 0;
}

// Send a message to the manager.
int sendman(char *str, int len) {
#ifdef DEBUG
  printf("> %.40s\n", str);
#endif  // DEBUG
  pthread_mutex_lock(&sockl);
  send(serverfd, str, len, 0);
  pthread_mutex_unlock(&sockl);
  return 0;
}

// Initialize socket with manager.
// Return 0 on success.
int initsocks(int port) {
  char sport[5];
  snprintf(sport, 5, "%d", port);
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  if (getaddrinfo(NULL, sport, &hints, &resolvedAddr) < 0)
    return -2;

  serverfd = socket(resolvedAddr->ai_family, resolvedAddr->ai_socktype,
                    resolvedAddr->ai_protocol);
  if (serverfd < 0)
    return -2;

  if (connect(serverfd, resolvedAddr->ai_addr, resolvedAddr->ai_addrlen) < 0)
    return -2;

  if (pthread_mutex_init(&sockl, NULL) != 0) {
    return -3;
  }

  return 0;
}

// Cleanup sockets.
int teardownsocks(void) {
  if (pthread_mutex_destroy(&sockl) != 0) {
    return -3;
  }
  close(serverfd);
  return 0;
}

void confirminvalidate(int pgnum) {
  char msg[100] = {0};
  snprintf(msg, 100, "INVALIDATE CONFIRMATION %d", pgnum);
  sendman(msg, strlen(msg));
}

// Return 0 on success.
int requestpage(int pgnum, char *type) {
  char msg[100] = {0};
  snprintf(msg, 100, "REQUESTPAGE %s %d", type, pgnum);
  return sendman(msg, strlen(msg));
}

void confirminvalidate_encoded(int pgnum, char *pgb64) {
  char msg[10000] = {0};
  snprintf(msg, 100 + strlen(pgb64), "INVALIDATE CONFIRMATION %d %s", pgnum, pgb64);
  sendman(msg, strlen(msg));
}

int handleconfirm(char *msg) {
  char *spgnum = strstr(msg, "ION ") + 4;
  int pgnum = atoi(spgnum);
  void *pg = (void *)PGNUM_TO_PGADDR((uintptr_t)pgnum);

  int err;
  int nspaces;

  // If not using existing page, decode the b64 data into the page.  The b64
  // string begins after the 4th ' ' character in msg.
  if (strstr(msg, "EXISTING") == NULL) {
    char *b64str = msg;
    nspaces = 0;
    while (nspaces < 4) {
      if (b64str[0] == ' ') {
	nspaces++;
      }
      b64str++;
    }
    
    char b64data[7000];
    if (b64decode(b64str, b64data) < 0) {
      fprintf(stderr, "Failure decoding b64 string");
      return -1;
    }

    // memcpy -- must set to write first to fill in page!
    if ((err = mprotect(pg, 1, (PROT_READ|PROT_WRITE))) != 0) {
      fprintf(stderr, "permission setting of page addr %p failed with error %d\n", pg, err);
      return -1;
    }
    if (memcpy(pg, b64data, PG_SIZE) == NULL) {
      fprintf(stderr, "memcpy failed.\n");
      return -1;
    }
  }

  if (strstr(msg, "WRITE") == NULL) {
    if ((err = mprotect(pg, 1, PROT_READ)) != 0) {
      fprintf(stderr, "permission setting of page addr %p failed with error %d\n", pg, err);
      return -1;
    }
  } else {
    if ((err = mprotect(pg, 1, PROT_READ|PROT_WRITE)) != 0) {
      fprintf(stderr, "permission setting of page addr %p failed with error %d\n", pg, err);
      return -1;
    }
  }


  waiting[pgnum % MAX_SHARED_PAGES] = 0;
  return 0;
}

// Handle invalidate messages.
int invalidate(char *msg) {
  int err;
  char *spgnum = strstr(msg, " ") + 1;
  int pgnum = atoi(spgnum);
  void *pg = (void *)PGNUM_TO_PGADDR((uintptr_t)pgnum);

  // If we don't need to reply with a b64-encoding of the page, just invalidate
  // and reply.
  if (strstr(msg, "PAGEDATA") == NULL) {
    printf("invalidation does not need a b64-encoding of the page\n");
    if ((err = mprotect(pg, 1, PROT_NONE)) != 0) {
      fprintf(stderr, "Invalidation of page addr %p failed with error %d\n", pg, err);
      return -1;
    }
    confirminvalidate(pgnum);
    return 0;
  }

  // We need to reply with a b64-encoding of the page. Set to read-only, encode
  // the page, set to non-readable, non-writeable, and confirm with the
  // encoding.
  // TODO: We need to hold a lock to prevent the page from becoming
  // writeable while we are encoding it. (Do we?)
  if (mprotect(pg, 1, PROT_READ) != 0) {
    fprintf(stderr, "Invalidation of page addr %p failed\n", pg);
    return -1;
  }
  char pgb64[PG_SIZE * 2] = {0};
  b64encode((const char *)pg, PG_SIZE, pgb64);
  if (mprotect(pg, 1, PROT_NONE) != 0) {
    fprintf(stderr, "Invalidation of page addr %p failed\n", pg);
    return -1;
  }
  confirminvalidate_encoded(pgnum, pgb64);
  return 0;
}

// ENDING FORMERLY IN RPC

int writehandler(void *pg);
int readhandler(void *pg);
void pgfaultsh(int sig, siginfo_t *info, ucontext_t *ctx);


// Signal handler state.
static struct sigaction oldact;

// Shared regions.
#define MAX_SHARED_REGIONS 100
int nextshrp;
struct sharedregion shrp[MAX_SHARED_REGIONS];


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
// For now, just change permissions to R+W.
// Return 0 on success.
int writehandler(void *pg) {
  int pgnum = PGADDR_TO_PGNUM((uintptr_t) pg);
  if (requestpage(pgnum, "WRITE") != 0) {
    return -1;
  }

  waiting[pgnum % MAX_SHARED_PAGES] = 1;
  while (waiting[pgnum % MAX_SHARED_PAGES] == 1)  {
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
  waiting[pgnum % MAX_SHARED_PAGES] = 1;
  while (waiting[pgnum % MAX_SHARED_PAGES] == 1)  {
  }

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
int initlibdsmu(int port, uintptr_t starta, size_t len) {
  int i;
  struct sigaction sa;

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
  initsocks(port);

  // Spin up thread that listens for messages from manager.
  if ((pthread_create(&tlisten, NULL, listenman, NULL) != 0)) {
    fprintf(stderr, "failed to spawn listener thread\n");
    return -1;
  }

  return 0;
}

int teardownlibdsmu(void) {
  teardownsocks();
  return 0;
}

