#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "b64.h"
#include "mem.h"
#include "rpc.h"

// Socket state.
int serverfd;
struct addrinfo *resolvedAddr;
struct addrinfo hints;

pthread_mutex_t sockl;

extern volatile int waiting[MAX_SHARED_PAGES];

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
  if (strstr(msg, "INVALIDATE") != NULL) {
    printf("< %.40s\n", msg);
    invalidate(msg);
  } else if (strstr(msg, "REQUESTPAGE") != NULL) {
    printf("< %.40s\n", msg);
    handleconfirm(msg);
  } else {
    printf("Undefined message.\n");
  }
  return 0;
}

// Send a message to the manager.
int sendman(char *str, int len) {
  printf("> %.40s\n", str);
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
  printf("page is unreadable, unwriteable\n");
  confirminvalidate_encoded(pgnum, pgb64);
  return 0;
}

