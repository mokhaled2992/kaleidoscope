#include <stdio.h>
#include <stdlib.h>

extern "C" {

double myprint(double arg)
{
    printf("%f", arg);
    return 0;
}

double foo(double, double);

int main()
{
    exit(foo(4, 3));
    return 0;
}
}