# CobaltC

A personal C compiler implementation written in modern C++, based on the book "Writing a C Compiler" by Nora Sandler. This project serves as a learning exercise in compiler construction and demonstrates the implementation of a complete C compiler from scratch.

## Getting Started

Clone the test suite repository to verify compiler functionality:

```bash
git clone https://github.com/nlsandler/writing-a-c-compiler-tests/
```

## TODO List

- [ ] Unit tests
- [ ] Tacky IR with std::variant
- [ ] Improved Error Reporting, store SourceRange or Token ranges for every AST node

### Extra Credit Features

- [ ] Bitwise Operations
- [ ] `typedef` support
- [ ] Compound Assignment
- [ ] Increment, Decrement operators
- [ ] Labels and goto statements
- [ ] `switch` statement

## Documentation

See [docs/gcc-reference.md](docs/gcc-reference.md) for GCC compilation examples and reference.

