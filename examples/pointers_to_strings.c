#include <stdio.h>

int main(){
    char *ptr = "abc";
    char (*array_ptr)[4] = &"abc";

    // Printing the content (strings)
    printf("Content of ptr: %s\n", ptr);                // Direct - ptr points to first char
    printf("Content of array_ptr: %s\n", *array_ptr);   // Dereference to get the array

    // Printing addresses (for comparison)
    printf("Address in ptr: %p\n", (void*)ptr);
    printf("Address in array_ptr: %p\n", (void*)array_ptr);
    printf("Address of *array_ptr: %p\n", (void*)*array_ptr);

    // Printing individual characters
    printf("First char via ptr: %c\n", ptr[0]);
    printf("First char via array_ptr: %c\n", (*array_ptr)[0]);
    
    // Show the difference in pointer arithmetic
    printf("ptr + 1 points to: %c\n", *(ptr + 1));           // 'b'
    printf("array_ptr + 1 points to: %p\n", (void*)(array_ptr + 1));  // Past the array
    
    return 0;
}