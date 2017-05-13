// loop invariants are paritially redundant.
int test(int a, int b) {
  int i = 0;
  int t = 0;
  while (i < 10) {
    i += 1;
    // a += 1;
    t = a + b;
  }
  return t;
}

int main() {
  return test(1, 2);
}