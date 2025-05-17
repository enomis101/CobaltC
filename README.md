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

## Link with a shared library

Compile with position-independent code the library file

```bash
gcc -c -Wall -fpic LIB.c
```

Create the shared library

```bash
gcc -shared -o LIB.so LIB.o
```

```bash
export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH
```

Link FILE.o with LIB.so to create an executable

```bash
gcc -o FILE FILE.o -L. -lLIB
```

### Alternative: Creating a static library

If you prefer to create a static library instead (which gets embedded into the executable):

Create a static library

```bash
ar rcs libprintlib.a printlib.o
```

```bash
gcc -o test test.o -L. -lprintlib -static
```
