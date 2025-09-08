#include <stdio.h>

int main() {
    char (*array_ptr1)[4] = &"abc";
    char (*array_ptr2)[4] = &"abc";
    
    // Let's also test with regular string pointers for comparison
    char *str_ptr1 = "abc";
    char *str_ptr2 = "abc";
    
    printf("array_ptr1 points to: %p\n", (void*)array_ptr1);
    printf("array_ptr2 points to: %p\n", (void*)array_ptr2);
    printf("Are they the same? %s\n", 
           (array_ptr1 == array_ptr2) ? "Yes" : "No");
    
    printf("\nstr_ptr1 points to: %p\n", (void*)str_ptr1);
    printf("str_ptr2 points to: %p\n", (void*)str_ptr2);
    printf("Are they the same? %s\n", 
           (str_ptr1 == str_ptr2) ? "Yes" : "No");
    
    return 0;
}