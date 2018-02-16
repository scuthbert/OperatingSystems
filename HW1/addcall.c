#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>

int main() {
  int n1 = 2;
  int n2 = 3;
  int *result = malloc(sizeof(int));
  syscall(333, n1, n2, result);
  printf("Value: %d\n", *result);
}
