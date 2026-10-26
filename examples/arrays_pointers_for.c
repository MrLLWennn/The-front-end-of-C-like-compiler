#include <stdio.h>

struct Summary
{
    int count;
    double total;
};

void scale(double *values, int count, double factor)
{
    for(int i = 0; i < count; ++i)
    {
        values[i] *= factor;
    }
}

double sum(double values[], int count)
{
    double total = 0.0;
    for(double *p = values; p < values + count; ++p)
    {
        total += *p;
    }
    return total;
}

int main(void)
{
    double values[] = {1.0, 2.0, 3.0, 4.0};
    char label[] = "verified";
    scale(values, 4, 1.5);
    struct Summary result;
    result.count = 4;
    result.total = sum(values, 4);
    struct Summary *view = &result;
    printf("count=%d total=%.1f label=%s\n", view->count, view->total, label);
    return 0;
}
