// main.c - Test file for preprocessing and line tracking
#include <stdio.h>
#include "math_utils.h"
#include "config.h"

#define SQUARE(x) ((x) * (x))

int main() {
    printf("Starting program\n");
    
    #ifdef DEBUG
    printf("Debug mode enabled\n");
    #endif
    
    int a = 5;
    int b = SQUARE(a);
    printf("Square of %d is %d\n", a, b);
    
    float result = add_numbers(3.14f, 2.86f);
    printf("Addition result: %.2f\n", result);
    
    #if VERSION > 1
    printf("Version 2 features enabled\n");
    #endif
    
    print_config();
    
    return 0;
}
