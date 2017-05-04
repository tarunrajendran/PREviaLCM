#include <stdio.h>
float test(float a, float b) {
  float t1;
  if (a) {
    t1 = a + b;
  }
  float t3 = a + b;
  return t3;
}

float main() {
   printf("%f\n", test(1.5, 2.3));
}