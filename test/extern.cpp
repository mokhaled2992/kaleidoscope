#include <stdio.h>
#include <stdlib.h>

extern "C" {

double myprint(double arg)
{
    printf("%f", arg);
    return 0;
}

double foo(double, double);

double bar(double a, double b)
{
    return a + b;
}

int main()
{
    exit(foo(4, 3));
    return 0;
}
}