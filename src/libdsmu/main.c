#include "sigsegv.h"

#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/mman.h>

static sigsegv_dispatcher dispatcher;

static pthread_t invalidatort;

static int handler(void *fault_address, int serious) {
  return sigsegv_dispatch(&dispatcher, fault_address);
}

static int area_handler(void *fault_address, void *user_arg) {
    unsigned long area = *(unsigned long *)user_arg;

    printf("called\n");

    if (mprotect((void *)area, 0x4000, PROT_READ | PROT_WRITE) == 0) {
	return 1;
    }

    return 0;
}

void *invaliatorf(void *ptr) {
    printf("here i am...\n");
    return 0;
}

int main(void) {
    int zero_fd = open("/dev/zero", O_RDONLY, 0644);
    printf("Hello world!\n");

    // Initialize sigsegv.
    sigsegv_init(&dispatcher);
    sigsegv_install_handler(&handler);
    printf("Done initializing libsigsegv\n");

    // Setup memory and fault handlers.
    void *p = mmap((void *)0x12340000, 0x4000, (PROT_READ|PROT_WRITE),
                   (MAP_ANON|MAP_PRIVATE), zero_fd, 0);
    if (p == (void *)(-1)) {
        fprintf(stderr, "mmap failed.\n");
        return 1;
    } else {
        printf("mmap succeeded.\n");
    }

    void *area1 = p;
    sigsegv_register(&dispatcher, area1, 0x4000, &area_handler, &area1);

    if (mprotect((void *)area1, 0x4000, PROT_NONE) < 0) {
        fprintf(stderr, "mprotect failed.\n");
        return 1;
    }

    // This will trigger a fault (a write fault!).
    ((volatile int *)area1)[612] = 11;

    // This should not trigger a fault.
    printf("value is: %d\n", ((volatile int *)area1)[612]);


    // Spin off thread to listen on the network for invalidations.
    if ((pthread_create(&invalidatort, NULL, invaliatorf, NULL) != 0)) {
        printf("failed to spawn invalidator thread\n");
    }

    pthread_join(invalidatort, NULL);

    printf("we done!\n");

    return 0;
}

