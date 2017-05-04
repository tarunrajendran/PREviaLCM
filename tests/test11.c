// test while
void modify(int *n) {
  *n = *n + 1;
}

int test(int a, int b) {
  int t1;
  int i = 0;
  while (i < 10) {
    modify(&a); // this line will cause issue.
    t1 = a + b;
    i++;
  }
  int t3 = a + b;
  return t1;
}

int main() {
  return test(1, 2);
}