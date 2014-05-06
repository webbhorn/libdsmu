#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "libdsmu.h"
#include "mem.h"

int main(int argc, char *argv[]) {
  if (argc < 3) {
    printf("Usage: main MANAGER_PORT [1|2]\n");
    return 1;
  }

  int * ball = (int *) 0x12340000;

  int port = atoi(argv[1]); 
  initlibdsmu(port, 0x12340000, 4096 * 10);

  int temp = *ball;

  while ((temp) < 20) {
    printf("[PINGPONG] ball = %d me = %d\n", temp, atoi(argv[2]));
    if ((atoi(argv[2]) == 1) && ((temp) % 2 == 0)) {
      temp += 1;
      *ball = temp;
      continue;
    } else if ((atoi(argv[2]) == 2) && ((temp) % 2 == 1)) {
      temp += 1;
      *ball = temp;
      continue;
    }

    sleep(1);
    temp = *ball;
  }

  teardownlibdsmu();
  return 0;
}
