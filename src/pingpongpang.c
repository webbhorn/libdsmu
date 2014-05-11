#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "libdsmu.h"
#include "mem.h"

int id;

int main(int argc, char *argv[]) {
  if (argc < 4) {
    printf("Usage: main MANAGER_IP MANAGER_PORT [1|2|3]\n");
    return 1;
  }

  int * ball = (int *) 0x12340000;

  char *ip = argv[1];
  int port = atoi(argv[2]); 
  initlibdsmu(ip, port, 0x12340000, 4096 * 10);

  int temp = *ball;

  while ((temp) < 100) {
    if ((atoi(argv[3]) == 1) && ((temp) % 3 == 0)) {
      printf("[PINGPONG] ball = %d me = %d\n", temp, atoi(argv[3]));
      temp += 1;
      *ball = temp;
      continue;
    } else if ((atoi(argv[3]) == 2) && ((temp) % 3 == 1)) {
      printf("[PINGPONG] ball = %d me = %d\n", temp, atoi(argv[3]));
      temp += 1;
      *ball = temp;
      continue;
    } else if ((atoi(argv[3]) == 3) && ((temp) % 3 == 2)) {
      printf("[PINGPONG] ball = %d me = %d\n", temp, atoi(argv[3]));
      temp += 1;
      *ball = temp;
      continue;
    }

    temp = *ball;
  }

  while(1);

  teardownlibdsmu();
  return 0;
}
