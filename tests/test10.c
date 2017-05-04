// test global & while
int a = 1;
int b = 2;

void modify(int *n) {
  *n = *n + 1;
}

int test() {
  int t1;
  int i = 0;
  while (i < 10) {
    modify(&a);
    t1 = a + b;
    i++;
  }
  int t3 = a + b;
  return t1;
}

int main() {
  return test();
}