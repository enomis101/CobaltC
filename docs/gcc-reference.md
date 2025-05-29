# GCC Compilation Reference

This document provides reference examples for compiling C programs to assembly using GCC. These examples are useful for comparison and understanding the output that CobaltC should generate.

## Compiling to Assembly

### Basic compilation:
```bash
gcc -O0 -S program.c -o program.s
```

### Optional Additional Flags
You might find these additional flags useful:
- `-fno-asynchronous-unwind-tables` - Produces cleaner assembly by removing unnecessary exception handling information
- `-fverbose-asm` - Adds helpful comments to the assembly output
- `-masm=intel` - Uses Intel syntax instead of the default AT&T syntax

### Example with all flags:
```bash
gcc -O0 -S -fno-asynchronous-unwind-tables -fverbose-asm program.c -o program.s
```

## Optimization Levels

- `-O0` - No optimization (default, best for debugging)
- `-O1` - Basic optimization
- `-O2` - Standard optimization
- `-O3` - Aggressive optimization

## Assembly Syntax Options

- Default: AT&T syntax (operands: source, destination)
- `-masm=intel` - Intel syntax (operands: destination, source)

## Common Use Cases

### Debugging compiler output:
```bash
gcc -O0 -S -fverbose-asm -masm=intel program.c -o program.s
```

### Clean assembly for comparison:
```bash
gcc -O0 -S -fno-asynchronous-unwind-tables program.c -o program.s
```

## Working with Shared Libraries

### Creating and linking with a shared library:

1. Compile with position-independent code:
```bash
gcc -c -Wall -fpic LIB.c
```

2. Create the shared library:
```bash
gcc -shared -o LIB.so LIB.o
```

3. Set library path for runtime linking:
```bash
export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH
```

4. Link your object file with the shared library:
```bash
gcc -o FILE FILE.o -L. -lLIB
```

### Alternative: Creating a static library

If you prefer to create a static library (which gets embedded into the executable):

1. Create a static library:
```bash
ar rcs libprintlib.a printlib.o
```

2. Link with the static library:
```bash
gcc -o test test.o -L. -lprintlib -static
```
