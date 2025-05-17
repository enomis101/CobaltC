# CobaltC

```bash
git clone https://github.com/nlsandler/writing-a-c-compiler-tests/
```

## Missing Features

- Bitwise Operations
- `typedef` support
- Compound Assignment
- Increment, Decrement
- Labels and goto
- `switch` statement

## Compiling to assembly with GCC

Optional Additional Flags
You might find these additional flags useful:

-fno-asynchronous-unwind-tables - Produces cleaner assembly by removing unnecessary exception handling information
-fverbose-asm - Adds helpful comments to the assembly output
-masm=intel - Uses Intel syntax instead of the default AT&T syntax

```bash
gcc -O0 -S [-fno-asynchronous-unwind-tables -fverbose-asm] program.c -o program.s
```
