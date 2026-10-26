#include <stdio.h>
#include "typed_structs.h"

int main(void)
{
    struct Sample sample = make_sample(1.5f, 'A');
    double score = add_offset(half(sample.value), 2);
    show_sample(sample);
    printf("score=%.2f\n", score);
    return 0;
}

struct Sample make_sample(float input, char grade)
{
    struct Sample sample;
    sample.value = input * 2.0;
    sample.grade = grade;
    sample.accepted = input > 0.0f;
    return sample;
}

void show_sample(struct Sample sample)
{
    printf("value=%.2f grade=%c accepted=%d\n", sample.value, sample.grade, sample.accepted);
}

double add_offset(double value, int offset)
{
    return value + offset;
}

float half(float value)
{
    return value * 0.5f;
}
