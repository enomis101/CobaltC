// math_utils.h - Math utility functions
#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#define PI 3.14159265359
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// Function declarations
float add_numbers(float x, float y);
float multiply_numbers(float x, float y);

// Inline function (will be expanded)
static inline int double_value(int n) {
    return n * 2;
}

#endif // MATH_UTILS_H
