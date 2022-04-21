#include <stdio.h>
#include <stdlib.h>

extern "C" {
double myexit(double arg)
{
    exit(arg);
    return 0;
}

double myprint(double arg)
{
    printf("%f", arg);
    return 0;
}
}