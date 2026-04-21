# Inline Assembly Support

## Overview

The C to i8080 compiler now supports inline assembly using the `asm { }` syntax, allowing you to embed raw i8080 assembly code directly within your C programs.

## Syntax

```c
asm {
    <assembly instructions>
    <one per line or separated>
}
```

## Features

- ✅ Full i8080 assembly instruction support
- ✅ Access to C variables using `__VAR_` prefix
- ✅ Labels and jumps within asm blocks
- ✅ I/O operations (IN/OUT)
- ✅ Mix with regular C code
- ✅ Preserves formatting and comments in assembly

## Basic Example

```c
int main() {
    int x = 42;

    asm {
        MVI A, 0xFF
        OUT 10H
        NOP
    }

    return x;
}
```

### Generated Output:
```assembly
main:
    MVI A, 42
    STA __VAR_x
    ; Inline assembly

        MVI A, 0xFF
        OUT 10H
        NOP

    LDA __VAR_x
    RET

; Local variables for main
__VAR_x:    .DS 2   ; variable
```

## Accessing C Variables

C variables are accessible in inline assembly using the `__VAR_` prefix:

```c
int main() {
    int port_value;
    int result;

    port_value = 0x80;

    // Read from I/O and store in C variable
    asm {
        IN 20H
        STA __VAR_result
    }

    // Load C variable and output to port
    asm {
        LDA __VAR_port_value
        OUT 30H
    }

    return result;
}
```

## Labels and Loops

You can use labels and jumps within asm blocks:

```c
asm {
    MVI B, 10
DELAY_LOOP:
    DCR B
    JNZ DELAY_LOOP
}
```

## Common Use Cases

### 1. Hardware I/O

```c
// Read from input port
asm {
    IN 01H
    STA __VAR_input_value
}

// Write to output port
asm {
    LDA __VAR_output_value
    OUT 02H
}
```

### 2. Performance-Critical Code

```c
// Fast memory copy
asm {
    LXI H, SOURCE_ADDR
    LXI D, DEST_ADDR
    MVI B, 100
COPY_LOOP:
    MOV A, M
    STAX D
    INX H
    INX D
    DCR B
    JNZ COPY_LOOP
}
```

### 3. Direct Register Manipulation

```c
// Save and restore registers
asm {
    PUSH PSW
    PUSH B
    PUSH D
    PUSH H

    ; ... your code ...

    POP H
    POP D
    POP B
    POP PSW
}
```

### 4. Interrupt Handlers

```c
void interrupt_handler() {
    asm {
        PUSH PSW
        PUSH B
        PUSH D
        PUSH H
    }

    // C code for interrupt handling

    asm {
        POP H
        POP D
        POP B
        POP PSW
        EI
        RET
    }
}
```

### 5. Timing Delays

```c
void delay() {
    asm {
        MVI B, 255
    OUTER:
        MVI C, 255
    INNER:
        DCR C
        JNZ INNER
        DCR B
        JNZ OUTER
    }
}
```

## Advanced Example: Mixing C and Assembly

```c
int calculate(int a, int b) {
    return a * b;
}

int main() {
    int x = 10;
    int y = 5;
    int result;

    // Use C function
    result = calculate(x, y);

    // Use assembly for I/O
    asm {
        LDA __VAR_result
        OUT 01H
    }

    // Conditional assembly
    if (result > 30) {
        asm {
            MVI A, 0xFF
            OUT 02H
        }
    } else {
        asm {
            MVI A, 00H
            OUT 02H
        }
    }

    return result;
}
```

## Variable Naming Convention

When accessing C variables from inline assembly:

| C Declaration | Assembly Name |
|---------------|---------------|
| `int x;` | `__VAR_x` |
| `int count;` | `__VAR_count` |
| `int *ptr;` | `__VAR_ptr` |

Each variable is allocated 2 bytes (16-bit):
```assembly
__VAR_x:     .DS 2
__VAR_count: .DS 2
__VAR_ptr:   .DS 2
```

## Best Practices

### 1. Document Your Assembly
```c
asm {
    ; Initialize serial port
    MVI A, 80H
    OUT 10H

    ; Set baud rate
    MVI A, 03H
    OUT 11H
}
```

### 2. Preserve Registers
```c
asm {
    PUSH PSW    ; Save accumulator and flags
    PUSH B      ; Save B and C

    ; Your code here

    POP B       ; Restore B and C
    POP PSW     ; Restore accumulator and flags
}
```

### 3. Use C Variables for Data
```c
int port_config = 0x80;

asm {
    LDA __VAR_port_config
    OUT 10H
}
```

### 4. Keep Assembly Blocks Focused
```c
// Good: Small, focused asm block
asm {
    IN 01H
    STA __VAR_input
}

// Less ideal: Large, complex asm block
// Consider writing a separate .asm file instead
```

## Limitations

- ⚠️ No automatic register allocation
- ⚠️ No type checking on variable access
- ⚠️ Manual register preservation required
- ⚠️ Labels are local to the asm block (use unique names)
- ⚠️ No syntax checking of assembly instructions

## Comparison: C vs Assembly

### Task: Read Port, Increment, Write Port

**C Code:**
```c
int value;
asm { IN 01H }
asm { STA __VAR_value }
value = value + 1;
asm { LDA __VAR_value }
asm { OUT 02H }
```

**Pure Assembly (More Efficient):**
```c
asm {
    IN 01H
    INR A
    OUT 02H
}
```

## Integration with Generated Code

The compiler seamlessly integrates inline assembly with generated code:

**C Input:**
```c
int main() {
    int x = 10;
    asm { NOP }
    int y = 20;
    return x + y;
}
```

**Generated Output:**
```assembly
main:
    MVI A, 10
    STA __VAR_x
    ; Inline assembly
     NOP 
    MVI A, 20
    STA __VAR_y
    LDA __VAR_x
    PUSH PSW
    LDA __VAR_y
    MOV B, A
    POP PSW
    ADD B
    RET

; Local variables for main
__VAR_x:    .DS 2   ; variable
__VAR_y:    .DS 2   ; variable
```

## Testing

Test files demonstrating inline assembly:
- `test_asm_basic.c` - Basic inline assembly
- `test_asm_advanced.c` - Advanced features (I/O, loops)
- `test_asm_mixed.c` - Mixing C and assembly

Compile and inspect:
```bash
c_to_i8080.exe test_asm_basic.c output.asm
```

## Summary

Inline assembly gives you:
- **Direct hardware access** for I/O operations
- **Performance optimization** for critical code
- **Low-level control** when needed
- **Flexibility** to mix high and low-level code

Use inline assembly when you need precise control over the i8080 processor, but prefer C for general program logic and maintainability.
