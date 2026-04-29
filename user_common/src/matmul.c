#include <stdint.h>
#include <stdio.h>
#include <matmul.h>
const int C[N][1] = {
  {12},
  {-3},
  {25},
  {7},
  {-14},
  {31},
  {5},
  {18}
};

void print_vector(uint8_t *v, uint8_t len) {
  printf("[");
  for(int i = 0; i < len; i++) {
    printf("%d ", v[i]);
  }
  printf("]\n");
}
