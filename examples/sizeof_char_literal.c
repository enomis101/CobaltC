#include <stdio.h>

int main() {
    printf("Size of 'a': %zu\n", sizeof('a'));        // Prints 4 (int size)
    printf("Size of char: %zu\n", sizeof(char));      // Prints 1
    
    char c = 'a';
    printf("Size of c: %zu\n", sizeof(c));            // Prints 1
    
    return 0;
}