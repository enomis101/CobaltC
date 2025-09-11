#include <stdio.h>

int main() {
              
       char *str_ptr1 = "abc";
       char *str_ptr2 = "abc";
       static char* static_str_ptr = "abc";
       printf("\nstr_ptr1 points to: %p\n", (void*)str_ptr1);
       printf("str_ptr2 points to: %p\n", (void*)str_ptr2);
       printf("static_str_ptr points to: %p\n", (void*)static_str_ptr);

    
    return 0;
}