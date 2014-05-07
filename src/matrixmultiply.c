#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "libdsmu.h"
#include "mem.h"

#define SIZE 2

typedef int matrix_t [SIZE][SIZE];

void print_matrix(matrix_t m) {
  int i, j;
  for (i = 0; i < SIZE; i++) {
    for (j = 0; j < SIZE; j++) {
      printf(" %d ", m[i][j]);
    }
    printf("\n");
  }
}

int main(int argc, char *argv[]) {
  if (argc < 4) {
    printf("Usage: main MANAGER_PORT [1|2]\n");
    return 1;
  }

  matrix_t A = {{3, 8}, {4, 7}};
  matrix_t B = {{1, 3}, {2, 5}};

  int port = atoi(argv[1]); 
  initlibdsmu(port, 0x12340000, 4096 * 10);
  matrix_t *C = (matrix_t *) 0x12340000;

  int i, j, k;
  for (i = 0; i < SIZE; i++) {
    if ((i % atoi(argv[3])) != (atoi(argv[2]) % atoi(argv[3]))) {
      printf("Skipping row %d\n", i);
      continue;
    }
    
    for (j = 0; j < SIZE; j++) {
      for (k = 0; k < SIZE; k++) {
	int temp = A[i][k] * B[k][j];
	printf("A[%d][%d] * B[%d][%d] = %d\n", i, k, k, j, temp);
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

  teardownlibdsmu();
  return 0;
}
