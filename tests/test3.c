// testing while loop.
int main() {
  int a = 1;
  int b = 2;
  int c = 3;
  int x = 4;
  int y = 5;

  if (x) {
    a = c;
    while (x) {
      x = a + b;
    }
  } else {
    y = 1;
  }

  y = a + b;
  return y;
}