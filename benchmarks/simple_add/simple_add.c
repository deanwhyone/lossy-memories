#include <stddef.h>

void __COMPRESS__(void *ptr, size_t size) {
   (void)ptr;
   (void)size;
   return;
}

int main() {
   int num1, num2, sum;

   __COMPRESS__((void *)&num1, sizeof(num1));
   __COMPRESS__((void *)&num2, sizeof(num2));
   __COMPRESS__((void *)&sum, sizeof(sum));

   num1 = 15;
   num2 = 31;

   sum = num1 + num2;
   return 0;
}