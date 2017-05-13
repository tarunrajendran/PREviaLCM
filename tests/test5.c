int test(int a, int b) {
  int t1;
  if (a) {
    t1 = a + b;
  } else {
    t1 = 1;
  }
  int t3 = a + b;
  return t3;
}

int main() {
  return test(1, 2);
}