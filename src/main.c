#include <stdio.h>
#include <stdlib.h>

#include "libdsmu.h"

int main(int argc, char *argv[]) {

  if (argc < 2) {
    printf("Usage: main MANAGER_PORT\n");
    return 1;
  }

  // Start libdsmu. Shared memory is the 10 pages beginning at 0x12340000.
  int port = atoi(argv[1]); 
  initlibdsmu(port, 0x12340000, 4096 * 10);

  // Spin for now.
  while (1);

  teardownlibdsmu();
  return 0;
}
