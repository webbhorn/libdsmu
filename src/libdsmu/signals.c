#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <sys/mman.h>

void pf_sighandler(int sig, struct sigcontext ctx) {
  printf("called!\n");
  return;
}

static struct sigaction signext;

int main(void) {
  struct sigaction sa;

  // Register page fault handler.
  sa.sa_handler = (void *)pf_sighandler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  if (sigaction(SIGSEGV, &sa, &signext) != 0) {
    fprintf(stderr, "sigaction failed\n");
  }

  // Setup test memory area.
  int zero_fd = open("/dev/zero", O_RDONLY, 0644);
  void *p = mmap((void *)0x12340000, 0x4000, (PROT_READ),
                 (MAP_ANON|MAP_PRIVATE), zero_fd, 0);
  if (p < 0) {
    fprintf(stderr, "mmap failed.\n");
    return 1;
  } else {
    printf("mmap succeeded.\n");
  }

  // Trigger a page fault.
  ((volatile int *)p)[612] = 11;

  printf("we done.\n");

  return 0;
}

