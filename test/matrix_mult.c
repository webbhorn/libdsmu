#include <stdio.h>
#include <time.h>

#define SIZE 2000

int A[SIZE][SIZE] = {{0}};
int B[SIZE][SIZE] = {{0}};
int C[SIZE][SIZE] = {{0}};

int main() {
  int i, j, k;
  srand(time(NULL));

  for (i = 0; i < SIZE; i++) {
    for (j = 0; j < SIZE; j++) {
      A[i][j] = randint();
      B[i][j] = randint();
    }
  }

  for (i = 0; i < SIZE; i++) {
    for (j = 0; j < SIZE; j++) {
      for (k = 0; k < SIZE; k++) {
        C[i][j] += A[i][k] * B[k][j];
      }
    }
  }

  //printf("Matrix A\n--------\n");
  //print_matrix(A);
  //printf("Matrix B\n--------\n");
  //print_matrix(B);
  //printf("Matrix C\n--------\n");
  //print_matrix(C);

  return 0;
}

void print_matrix(int matrix[SIZE][SIZE]) {
  int i, j;
  for (i = 0; i < SIZE; i++) {
    for (j = 0; j < SIZE; j++) {
      printf(" %d ", matrix[i][j]);
    }
    printf("\n");
  }
}

int randint() {
  return rand() % 10;
}
