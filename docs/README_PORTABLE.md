# C to i8080 Compiler - Portable Version

A C compiler that translates C code to Intel 8080 assembly language.

## Files Included

- `c_to_i8080.h` - Header file with type definitions
- `main.c` - Main program entry point
- `lexer.c` - Tokenizer/lexer
- `parser.c` - Parser (builds AST)
- `codegen.c` - Code generator (emits i8080 assembly)

## Building

### Windows (Visual Studio)
```bash
cl /Fe:c_to_i8080.exe main.c lexer.c parser.c codegen.c
```

### Windows (MinGW/GCC)
```bash
gcc -o c_to_i8080.exe main.c lexer.c parser.c codegen.c -std=c99
```

### Linux/Mac
```bash
gcc -o c_to_i8080 main.c lexer.c parser.c codegen.c -std=c99
```

### Visual Studio IDE
1. Create new C++ Console App project
2. Add all 5 files to the project
3. Build (Ctrl+Shift+B)

## Usage

```bash
c_to_i8080 input.c output.asm
```

## Supported C Features

### Data Types
- `int` - Integer variables
- `int *` - Pointers

### Operators
- Arithmetic: `+`, `-`, `*`, `/`, `%`
- Comparison: `==`, `!=`, `<`, `<=`, `>`, `>=`
- Logical: `&&`, `||`, `!`
- Bitwise: `&`, `|`, `^`, `~`
- Pointer: `&` (address-of), `*` (dereference)

### Control Flow
- `if` / `else`
- `while` loops
- `return` statements

### Functions
- Function declarations with parameters
- Function calls
- Return values

### Inline Assembly
```c
asm {
    MVI A, 0xFF
    OUT 10H
}
```
Access C variables with `__VAR_` prefix:
```c
int value = 42;
asm {
    LDA __VAR_value
    OUT 01H
}
```

## Example

**Input** (`test.c`):
```c
int add(int a, int b) {
    return a + b;
}

int main() {
    int x = 10;
    int y = 20;
    int result = add(x, y);
    return result;
}
```

**Compile**:
```bash
c_to_i8080 test.c output.asm
```

**Output** (`output.asm`):
```assembly
; Generated i8080 assembly code

    .ORG 0100H

    ; Entry point
    LXI SP, STACK_TOP
    CALL main
    HLT

add:
    LDA __VAR_a
    PUSH PSW
    LDA __VAR_b
    MOV B, A
    POP PSW
    ADD B
    RET

; Local variables for add
__VAR_a:    .DS 2   ; variable
__VAR_b:    .DS 2   ; variable

main:
    MVI A, 10
    STA __VAR_x
    MVI A, 20
    STA __VAR_y
    LDA __VAR_x
    PUSH PSW
    LDA __VAR_y
    PUSH PSW
    CALL add
    LXI H, 4
    DAD SP
    SPHL
    STA __VAR_result
    LDA __VAR_result
    RET

; Local variables for main
__VAR_x:      .DS 2   ; variable
__VAR_y:      .DS 2   ; variable
__VAR_result: .DS 2   ; variable

; Stack space
    .ORG 0FFFFH
STACK_TOP:
```

## Key Features

✅ **Direct Memory Addressing** - Efficient variable access
✅ **Conditional Runtime Functions** - Only includes math functions when used
✅ **Full Pointer Support** - `int *ptr`, `&var`, `*ptr`
✅ **Inline Assembly** - Embed raw i8080 code with `asm { }`
✅ **Optimized Code Generation** - Small, fast output

## Memory Layout

```
0x0100 - 0x7FFF: Program code
0x8000+:         Local variables
0xFFFF:          Stack top (grows downward)
```

## Limitations

- 8-bit arithmetic only (0-255)
- No arrays
- No structures
- No strings
- No global variables
- No for loops (use while instead)

## Version

Optimized version with:
- Direct memory addressing for variables
- Conditional runtime library inclusion
- Pointer operations
- Inline assembly support

## License

Educational/Open Source

---

For full documentation, see: https://github.com/yourusername/c-to-i8080
