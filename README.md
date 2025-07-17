# CobaltC

A personal C compiler implementation written in modern C++, based on the book "Writing a C Compiler" by Nora Sandler. This project serves as a learning exercise in compiler construction and demonstrates the implementation of a complete C compiler from scratch.

**Status**: Work in progress - currently implemented Chapter 10

## Getting Started

Clone the test suite repository to verify compiler functionality:

```bash
git clone https://github.com/nlsandler/writing-a-c-compiler-tests/
```

## TODO List

- [ ] Add semantic analysis and tacky stage unit tests and validation stages (move type validation out from parser printer wiwch checks that every expression has a valid type)

- [ ] Unit tests
- [ ] Tacky IR with std::variant
- [ ] Improved Error Reporting, store SourceRange or Token ranges for every AST node
- [ ] Bitwise Operations
- [ ] `typedef` support
- [ ] Compound Assignment
- [ ] Increment, Decrement operators
- [ ] Labels and goto statements
- [ ] `switch` statement

## Documentation

See [docs/gcc-reference.md](docs/gcc-reference.md) for GCC compilation examples and reference.

## About

This compiler implementation follows the structure and approach outlined in "Writing a C Compiler", providing hands-on experience with lexical analysis, parsing, semantic analysis, and code generation phases of compilation.