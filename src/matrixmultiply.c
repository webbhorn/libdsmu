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
  if (argc < 4) {
    printf("Usage: main MANAGER_PORT [1|2]\n");
    return 1;
  }

  srand(SEED);

  int port = atoi(argv[1]); 
  initlibdsmu(port, 0x12340000, 4096 * 10);
  matrix_t *C = (matrix_t *) 0x12340000;

  int i, j, k;
  for (i = 0; i < SIZE; i++) {
    for (j = 0; j < SIZE; j++) {
      A[i][j] = randint();
      B[i][j] = randint();
    }
  }

  for (i = 0; i < SIZE; i++) {
    if ((i % atoi(argv[3])) != (atoi(argv[2]) % atoi(argv[3]))) {
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
