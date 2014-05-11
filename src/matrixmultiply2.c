#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include "libdsmu.h"
#include "mem.h"

#define SIZE 2048
#define SEED 69

typedef int matrix_t [SIZE][SIZE];

int randint() {
  return rand() % 10;
}

void print_matrix(matrix_t m) {
  int i, j;
  for (i = 0; i < SIZE; i++) {
    for (j = 0; j < SIZE; j++) {
      printf(" %d ", m[i][j]);
    }
    printf("\n");
  }
}

matrix_t A, B;

int id;

int main(int argc, char *argv[]) {
  if (argc < 5) {
    printf("Usage: main MANAGER_IP MANAGER_PORT id[1|2|...|n] nodes[n]\n");
    return 1;
  }

  srand(SEED);

  char *ip = argv[1];
  int port = atoi(argv[2]);
  id = atoi(argv[3]);
  int n = atoi(argv[4]);

  initlibdsmu(ip, port, 0x12340000, 4096 * 10000);

  struct timeval tv;
  gettimeofday(&tv, NULL);
  double start_ms = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;

  matrix_t *C = (matrix_t *) 0x12340000;

  int i, j, k;
  for (i = 0; i < SIZE; i++) {
    for (j = 0; j < SIZE; j++) {
      A[i][j] = randint();
      B[i][j] = randint();
    }
  }

  double s = SIZE;
  double factor = s / n;
  int counter = 0;

  for (i = 0; i < SIZE; i++) {
    if (i >= id * factor || i < (id - 1) * factor) {
      continue;
    }

    counter++;
    //printf("Processor id %d doing row %d\n", id, i);

    for (j = 0; j < SIZE; j++) {
      for (k = 0; k < SIZE; k++) {
        int temp = A[i][k] * B[k][j];
        (*C)[i][j] += temp;
      }
    }
  }

  printf("Processor id %d did %d rows\n", id, counter);

  gettimeofday(&tv, NULL);
  double end_ms = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;

  printf("TOTAL TIME (ms): %lf\n", (end_ms - start_ms));
  printf("done\n");

  printf("sleeping for 10 seconds...\n");
  sleep(10);

  /*
  printf("Matrix A\n--------\n");
  print_matrix(A);
  printf("Matrix B\n--------\n");
  print_matrix(B);
  printf("Matrix C\n--------\n");
  print_matrix(*C);
  */

  teardownlibdsmu();
  return 0;
}
