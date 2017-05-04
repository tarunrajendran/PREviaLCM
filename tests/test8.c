
int test(int a, int b) {
  int i = 0;
  while (i < 10) {
    i += 1;
    a += 1;
    int t = a + b;
  }
}

int main() {
  return test(1, 2);
}