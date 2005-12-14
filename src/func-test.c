#include <stdio.h>

void (*func)(char *s);

void output(char *s)
{
  printf("%s\n", s);
}

int main()
{
  func=output;
  func("abc");
  return 0;
}
