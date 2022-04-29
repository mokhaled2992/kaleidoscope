#include <stdio.h>
#include <stdlib.h>

extern "C" {
double bar(double a, double b)
{
    return a + b;
}
}