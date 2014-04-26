#include "sigsegv.h"

#include <fcntl.h>
#include <stdio.h>

static sigsegv_dispatcher dispatcher;

static int handler(void *fault_address, int serious) {
  return sigsegv_dispatch(&dispatcher, fault_address);
}

int main(void) {
    int zero_fd = open("/dev/zero", O_RDONLY, 0644);
    printf("Hello world!\n");

    // Initialize sigsegv.
    sigsegv_init(&dispatcher);
    sigsegv_install_handler(&handler);
    printf("Done initializing libsigsegv\n");
}
