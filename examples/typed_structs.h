#ifndef TYPED_STRUCTS_H
#define TYPED_STRUCTS_H
#include <stdbool.h>

struct Sample
{
    double value;
    char grade;
    bool accepted;
};

struct Sample make_sample(float input, char grade);
void show_sample(struct Sample sample);
double add_offset(double value, int offset);
float half(float value);
#endif
