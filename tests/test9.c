void modify(int *n) {
  *n = *n + 1;
}

int test(int a, int b) {
  int t1;
  if (a) {
    modify(&a);
    t1 = a + b;
  } else {
    int c = 1;
  }
  int t3 = a + b;
  return t1;
}

int main() {
  return test(1, 2);
}