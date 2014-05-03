#ifndef LIBDSMU_H_
#define LIBDSMU_H_

#include <signal.h>
#include <stdint.h>
#include <ucontext.h>

#define REG_ERR 19
#define PG_WRITE 0x2
#define PG_PRESENT 0x1
#define PG_BITS 12
#define PG_SIZE (1 << (PG_BITS))

int writehandler(void *pg);

int readhandler(void *pg);

void pgfaultsh(int sig, siginfo_t *info, ucontext_t *ctx);

int b64encode(const char *starta, unsigned int sz, char *outbuf);

int sendman(char *str, int len);

int initsocks(void);

int teardownsocks(void);

void confirminvalidate(int pgnum);

void confirminvalidate_encoded(int pgnum, char *pgb64);

int invalidate(char *msg);

int dispatch(char *msg);

void *listenman(void *ptr);


// Convert address to the start of the page.
static inline uintptr_t PGADDR(uintptr_t addr) {
  return addr & ~(PG_SIZE - 1);
}

// Convert page number to address of start of page.
static inline uintptr_t PGNUM_TO_PGADDR(uintptr_t pgnum) {
  return pgnum << PG_BITS;
}

#endif  // LIBDSMU_H_
