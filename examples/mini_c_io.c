#include <stdio.h>
#include "mini_c_math.h"

int main(void)
{
    int n = DEMO_N;
    int total = sum_squares(n);
    int code = 0;
    printf("sum_squares(%d) = %d\n", n, total);
    if(total == 55)
    {
        printf("status: %s\n", "ok");
    }
    else
    {
        printf("status: %s\n", "failed");
        code = 1;
    }
    return code;
}

int square(int x)
{
    return x * x;
}

int sum_squares(int n)
{
    int i = 1;
    int total = 0;
    while(i <= n)
    {
        total = total + square(i);
        i = i + 1;
    }
    return total;
}
