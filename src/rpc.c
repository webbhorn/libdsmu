#include <err.h>
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

extern pthread_condattr_t waitca[MAX_SHARED_PAGES];
extern pthread_cond_t waitc[MAX_SHARED_PAGES];
extern pthread_mutex_t waitm[MAX_SHARED_PAGES];

// Listen for manager messages and dispatch them.
void *listenman(void *ptr) {
  int ret;
  printf("Listening...\n");
  while (1) {
    // See how long the payload (actualy message) is by peeking.
    char peekstr[20] = {0};
    ret = recv(serverfd, peekstr, 10, MSG_PEEK | MSG_WAITALL);
    if (ret != 10)
      err(1, "Could not peek into next packet's size");
    int payloadlen = atoi(peekstr);

    // Compute size of header (the length string).
    int headerlen;
    char *p = peekstr;
    for (headerlen = 1; *p != ' '; p++)
      headerlen++;

    // Read the entire unit (header + payload) from the socket.
    char msg[10000] = {0};
    ret = recv(serverfd, msg, headerlen + payloadlen, MSG_WAITALL);
    if (ret != headerlen + payloadlen)
      err(1, "Could not read entire unit from socket");

    const char s[] = " ";
    char *payload = strstr(msg, s) + 1;
    dispatch(payload);
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
int sendman(char *str) {
  int ret;

#ifdef DEBUG
  printf("> %.40s\n", str);
#endif  // DEBUG

  pthread_mutex_lock(&sockl);
  char msg[10000];
  sprintf(msg, "%zu %s", strlen(str), str);
  ret = send(serverfd, msg, strlen(msg), 0);
  if (ret != strlen(msg))
    err(1, "Could not send the message");
  pthread_mutex_unlock(&sockl);
  return 0;
}

// Initialize socket with manager.
// Return 0 on success.
int initsocks(char *ip, int port) {
  char sport[5];
  snprintf(sport, 5, "%d", port);
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  if (getaddrinfo(ip, sport, &hints, &resolvedAddr) < 0) {
    fprintf(stderr, "Could not resolve the IP address provided.\n");
    return -2;
  }

  serverfd = socket(resolvedAddr->ai_family, resolvedAddr->ai_socktype,
                    resolvedAddr->ai_protocol);
  if (serverfd < 0) {
    fprintf(stderr, "Socket file descriptor was invalid.\n");
    freeaddrinfo(resolvedAddr); // Free the address struct we created on error.
    return -2;
 }

  if (connect(serverfd, resolvedAddr->ai_addr,
    resolvedAddr->ai_addrlen) < 0) {
    fprintf(stderr, "Could not connect to the IP address provided.\n");
    freeaddrinfo(resolvedAddr);
    return -2;
  }

  freeaddrinfo(resolvedAddr); // Done with the address struct, free it.

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
  sendman(msg);
}

// Return 0 on success.
int requestpage(int pgnum, char *type) {
  char msg[100] = {0};
  snprintf(msg, 100, "REQUESTPAGE %s %d", type, pgnum);
  return sendman(msg);
}

void confirminvalidate_encoded(int pgnum, char *pgb64) {
  char msg[10000] = {0};
  snprintf(msg, 100 + strlen(pgb64), "INVALIDATE CONFIRMATION %d %s", pgnum, pgb64);
  sendman(msg);
}

int handleconfirm(char *msg) {
  char *spgnum = strstr(msg, "ION ") + 4;
  int pgnum = atoi(spgnum);
  void *pg = (void *)PGNUM_TO_PGADDR((uintptr_t)pgnum);

  int err;
  int nspaces;

  // Acquire mutex for condition variable.
  pthread_mutex_lock(&waitm[pgnum % MAX_SHARED_PAGES]);

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

  // Signal to the page handler that they can now run.
  pthread_cond_signal(&waitc[pgnum % MAX_SHARED_PAGES]);

  // Unlock to allow another page fault to be handled.
  pthread_mutex_unlock(&waitm[pgnum % MAX_SHARED_PAGES]); 
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

