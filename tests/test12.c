/**
 * Transp function
 * check CallInst arguments isPointerTy
 */

int nop(int a) {
  return a;
}

int test(int a) {
 int t1;
 int i = 0;
 while (i < 10) {
   nop(2); // CallInst here
   t1 = a + 2;
   i++;
 }
 int t3 = a + 2;
 return t1;
}

int main() {
 return test(1);
}