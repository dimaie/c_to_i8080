# Quick Reference - C to i8080 Compiler

## Usage

```bash
c_to_i8080.exe <input.c> [output.asm]
```

## Supported C Language Features

### Data Types
```c
int x;          // Integer variable
int *ptr;       // Pointer to integer
```

### Operators

**Arithmetic:**
- `+` Addition (inline)
- `-` Subtraction (inline)
- `*` Multiplication (calls __mul if used)
- `/` Division (calls __div if used)
- `%` Modulo (calls __mod if implemented)

**Comparison:**
- `==` Equal
- `!=` Not equal
- `<` Less than
- `<=` Less than or equal
- `>` Greater than
- `>=` Greater than or equal

**Logical:**
- `&&` Logical AND
- `||` Logical OR
- `!` Logical NOT

**Bitwise:**
- `&` Bitwise AND
- `|` Bitwise OR
- `^` Bitwise XOR
- `~` Bitwise NOT

**Pointer:**
- `&var` Address-of
- `*ptr` Dereference

**Assignment:**
- `=` Assignment
- `*ptr = value` Assignment through pointer

### Control Flow
```c
if (condition) {
    // code
} else {
    // code
}

while (condition) {
    // code
}

for (init; condition; increment) {
    // code
}

do {
    // code
} while (condition);
```

### Inline Assembly
```c
asm {
    MVI A, 0xFF
    OUT 10H
    NOP
}
```

Access C variables using `__VAR_` prefix:
```c
int value = 42;
asm {
    LDA __VAR_value
    OUT 01H
}
```

### Functions
```c
int add(int a, int b) {
    return a + b;
}

int main() {
    int result = add(5, 3);
    return result;
}
```

### Pointers
```c
int x = 10;
int *ptr = &x;      // Get address of x
int y = *ptr;       // Read value through pointer
*ptr = 20;          // Write value through pointer
```

## Generated Assembly Structure

### Memory Layout
```
0x0100: Program entry point
...     Program code
...     Runtime functions (__mul, __div if needed)
...     Variable declarations

0x8000: Local variables start here
```

### Variable Naming
Local variables are prefixed with `__VAR_`:
```assembly
__VAR_x:    .DS 2   ; variable
__VAR_ptr:  .DS 2   ; pointer
```

### Runtime Functions

**__mul** (included only if `*` is used):
- Input: A = first operand, B = second operand
- Output: A = result
- ~42 bytes

**__div** (included only if `/` is used):
- Input: A = dividend, B = divisor
- Output: A = quotient
- ~45 bytes

## Examples

### Simple Arithmetic
```c
int main() {
    int x = 10;
    int y = 20;
    int sum = x + y;
    return sum;
}
```

### Pointer Usage
```c
int main() {
    int value = 42;
    int *ptr = &value;
    *ptr = 100;
    return *ptr;
}
```

### Function Call
```c
int add(int a, int b) {
    return a + b;
}

int main() {
    int result = add(10, 20);
    return result;
}
```

### Conditional
```c
int main() {
    int x = 10;
    int result;

    if (x > 5) {
        result = 1;
    } else {
        result = 0;
    }

    return result;
}
```

## Optimization Tips

1. **Avoid multiplication/division** when possible to save ~87 bytes
2. **Use addition/subtraction** - they're inline and very fast
3. **Minimize variables** - each uses 2 bytes
4. **Use pointers** for indirect access efficiently
5. **Keep expressions simple** - reduces temporary storage

## Limitations

- Only 8-bit integer operations
- No arrays (yet)
- No structs (yet)
- No global variables (functions only)
- No string operations
- No for loops (use while instead)
- Limited to local variables in functions

## Performance Characteristics

| Operation | Instructions | Bytes | Cycles |
|-----------|--------------|-------|--------|
| Load var | 1 (LDA) | 3 | 13 |
| Store var | 1 (STA) | 3 | 13 |
| Add | 1 (ADD) | 1 | 4 |
| Subtract | 1 (SUB) | 1 | 4 |
| Multiply | ~14 (call) | 3 + 42 | ~100+ |
| Divide | ~15 (call) | 3 + 45 | ~120+ |
| Compare | 4-6 | 8-12 | 20-30 |
| Deref ptr | 4 | 6 | 20 |
| Addr-of | 3 | 6 | 17 |

## Common Issues

**Issue**: Warnings about non-ASCII characters
**Solution**: Save your .c file as plain ASCII or UTF-8 without BOM

**Issue**: Variable not found
**Solution**: Declare variables at the start of the function

**Issue**: Output too large
**Solution**: Reduce number of variables or avoid multiplication/division

## Testing Your Code

```powershell
# Compile
.\c_to_i8080.exe mycode.c output.asm

# Check output size
Get-Item output.asm

# Check for runtime functions
Get-Content output.asm | Select-String "__mul|__div"
```
