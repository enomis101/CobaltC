# TODO 100% coverage

## **Not Covered At All:**

1. **Pointer declarations and operations**
   - Pointer variable declarations (`int* x;`)
   - Multiple pointer levels (`int** x;`)
   - Address-of operator (`&variable`)
   - Dereference operator (`*pointer`)

2. **Array declarations and operations**
   - Array variable declarations (`int arr[10];`)
   - Multi-dimensional arrays (`int arr[5][10];`)
   - Array subscript expressions (`arr[index]`)

3. **Cast expressions**
   - Simple casts (`(int)x`)
   - Pointer casts (`(int*)ptr`)
   - Complex type casts

4. **Initializers**
   - Single initializers for arrays
   - Compound initializers (`{1, 2, 3}`)
   - Nested compound initializers for multi-dimensional arrays
   - Trailing commas in initializer lists

5. **Abstract declarators**
   - Used in cast expressions
   - Function pointer types in casts

6. **Complex type specifiers**
   - `long long` combinations
   - `unsigned long` combinations
   - `signed` keyword variations
   - `double` type usage

## **Partially Covered (Missing Edge Cases):**

7. **Parameter lists**
   - `void` parameter lists
   - Empty parameter lists `()`
   - Complex parameter types (pointers, arrays)

8. **Unary operators**
   - Complement operator (`~`)
   - Logical NOT operator (`!`)

9. **Binary operators**
   - All comparison operators (`!=`, `<=`, `>=`)
   - Modulo operator (`%`)
   - All combinations and precedence

10. **Function declarations**
    - Function pointers as parameters
    - Complex return types
    - Functions returning pointers/arrays

11. **Postfix expressions**
    - Array subscripting
    - Function calls with complex arguments

## **Error Handling Cases:**

12. **Type specifier errors**
    - Multiple identical specifiers
    - Invalid combinations (`signed double`)
    - Missing type specifiers

13. **Declarator errors**
    - Invalid array sizes (negative, zero)
    - Complex nested declarators
    - Function pointer syntax errors

14. **Expression parsing errors**
    - Malformed cast expressions
    - Invalid primary expressions
    - Precedence edge cases

15. **Parser context tracking**
    - Context stack functionality
    - Error reporting with context

## **Complex Parsing Scenarios:**

16. **Right associativity**
    - Conditional operator (`? :`) associativity
    - Assignment operator chaining (partially covered)

17. **Precedence combinations**
    - Mixed arithmetic, logical, and comparison operators
    - Conditional operator with complex expressions

18. **Declaration combinations**
    - Mixed storage classes and types
    - Function pointers with storage classes
    - Static/extern with complex types

19. **Statement parsing edge cases**
    - Empty for loop components
    - Nested conditional statements
    - Complex loop body statements

20. **Utility functions**
    - `parse_array_size()` with different constant types
    - `process_declarator()` with complex declarators
    - `process_abstract_declarator()` functionality
