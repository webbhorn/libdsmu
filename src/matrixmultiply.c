#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "libdsmu.h"
#include "mem.h"

#define SIZE 20
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

int main(int argc, char *argv[]) {
  if (argc < 5) {
    printf("Usage: main MANAGER_IP MANAGER_PORT id[1|2|...|n] nodes[n]\n");
    return 1;
  }

  srand(SEED);

  char *ip = argv[1];
  int port = atoi(argv[2]);
  int id = atoi(argv[3]);
  int n = atoi(argv[4]);

  initlibdsmu(ip, port, 0x12340000, 4096 * 10);
  matrix_t *C = (matrix_t *) 0x12340000;

  int i, j, k;
  for (i = 0; i < SIZE; i++) {
    for (j = 0; j < SIZE; j++) {
      A[i][j] = randint();
      B[i][j] = randint();
    }
  }

  for (i = 0; i < SIZE; i++) {
    if (((i % n) != (id % n))) {
      continue;
    }
    
    for (j = 0; j < SIZE; j++) {
      for (k = 0; k < SIZE; k++) {
	int temp = A[i][k] * B[k][j];
	(*C)[i][j] += temp;
      }
    }
  }

  printf("done\n");
  scanf(" ");

  printf("Matrix A\n--------\n");
  print_matrix(A);
  printf("Matrix B\n--------\n");
  print_matrix(B);
  printf("Matrix C\n--------\n");
  print_matrix(*C);

  while (1);

  teardownlibdsmu();
  return 0;
}
