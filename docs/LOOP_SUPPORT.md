# Loop Support - Complete Guide

The C to i8080 compiler now supports **all three** major loop types in C!

## ✅ Supported Loop Types

### 1. While Loop
**Syntax**:
```c
while (condition) {
    // body
}
```

**Execution**: Checks condition **before** each iteration

**Example**:
```c
int i = 0;
int sum = 0;
while (i < 10) {
    sum = sum + i;
    i = i + 1;
}
// sum = 45
```

**Generated Assembly**:
```assembly
    MVI A, 0
    STA __VAR_i
    MVI A, 0
    STA __VAR_sum
.L0:                        ; Loop start
    LDA __VAR_i
    PUSH PSW
    MVI A, 10
    MOV B, A
    POP PSW
    SUB B
    JM .L1                  ; Continue if i < 10
    JMP .L2                 ; Exit
.L1:
    ; Loop body
    LDA __VAR_sum
    PUSH PSW
    LDA __VAR_i
    MOV B, A
    POP PSW
    ADD B
    STA __VAR_sum
    LDA __VAR_i
    PUSH PSW
    MVI A, 1
    MOV B, A
    POP PSW
    ADD B
    STA __VAR_i
    JMP .L0                 ; Back to start
.L2:                        ; Loop end
```

---

### 2. For Loop (NEW! ✨)
**Syntax**:
```c
for (initialization; condition; increment) {
    // body
}
```

**Parts**:
- **Initialization**: Executed once before loop starts
- **Condition**: Checked before each iteration
- **Increment**: Executed after each iteration
- **Body**: The loop code

**Example 1: Basic for loop**
```c
int sum = 0;
int i;
for (i = 0; i < 10; i = i + 1) {
    sum = sum + i;
}
// sum = 45
```

**Example 2: For loop with variable declaration**
```c
int sum = 0;
for (int i = 0; i < 10; i = i + 1) {
    sum = sum + i;
}
// sum = 45
// Note: 'i' is scoped to the loop
```

**Example 3: Nested for loops**
```c
int total = 0;
for (int i = 0; i < 3; i = i + 1) {
    for (int j = 0; j < 4; j = j + 1) {
        total = total + 1;
    }
}
// total = 12
```

**Example 4: Empty parts allowed**
```c
int i = 0;
for (; i < 10;) {  // Empty init and increment
    i = i + 1;
}
```

**Generated Assembly**:
```assembly
    MVI A, 0
    STA __VAR_sum
    MVI A, 0
    STA __VAR_i
.L0:                        ; Condition check
    LDA __VAR_i
    PUSH PSW
    MVI A, 10
    MOV B, A
    POP PSW
    SUB B
    JM .L1
    JMP .L2
.L1:
    ; Loop body
    LDA __VAR_sum
    PUSH PSW
    LDA __VAR_i
    MOV B, A
    POP PSW
    ADD B
    STA __VAR_sum
.L3:                        ; Increment
    LDA __VAR_i
    PUSH PSW
    MVI A, 1
    MOV B, A
    POP PSW
    ADD B
    STA __VAR_i
    JMP .L0                 ; Back to condition
.L2:                        ; Loop end
```

---

### 3. Do-While Loop (NEW! ✨)
**Syntax**:
```c
do {
    // body
} while (condition);
```

**Execution**: **Always executes at least once** (checks condition after body)

**Example 1: Basic do-while**
```c
int count = 0;
int sum = 0;
do {
    sum = sum + count;
    count = count + 1;
} while (count < 5);
// sum = 10 (0+1+2+3+4)
```

**Example 2: Executes even when condition is initially false**
```c
int x = 10;
do {
    x = x + 1;
} while (x < 5);  // Condition is false, but body ran once
// x = 11
```

**Example 3: Input validation pattern**
```c
int input;
do {
    asm {
        IN 01H
        STA __VAR_input
    }
} while (input == 0);  // Keep reading until non-zero
```

**Generated Assembly**:
```assembly
    MVI A, 0
    STA __VAR_count
    MVI A, 0
    STA __VAR_sum
.L0:                        ; Loop start (body first)
    LDA __VAR_sum
    PUSH PSW
    LDA __VAR_count
    MOV B, A
    POP PSW
    ADD B
    STA __VAR_sum
    LDA __VAR_count
    PUSH PSW
    MVI A, 1
    MOV B, A
    POP PSW
    ADD B
    STA __VAR_count
    ; Now check condition
    LDA __VAR_count
    PUSH PSW
    MVI A, 5
    MOV B, A
    POP PSW
    SUB B
    JM .L0                  ; Jump back if count < 5
.L1:                        ; Loop end
```

---

## Comparison Table

| Feature | while | for | do-while |
|---------|-------|-----|----------|
| Checks condition | Before body | Before body | After body |
| Minimum executions | 0 | 0 | 1 |
| Init section | No | Yes | No |
| Increment section | No | Yes | No |
| Best for | Unknown iterations | Known iterations | At least one iteration |

---

## When to Use Each Loop

### Use `while` when:
- Number of iterations is unknown
- Condition-based loops (wait for input, etc.)
- Simple loop logic

```c
// Wait for ready signal
while (port_status != 0xFF) {
    asm {
        IN 01H
        STA __VAR_port_status
    }
}
```

### Use `for` when:
- Number of iterations is known
- Need initialization and increment
- Iterating with a counter

```c
// Process 10 items
for (int i = 0; i < 10; i = i + 1) {
    process_item(i);
}
```

### Use `do-while` when:
- Body must execute at least once
- Menu loops
- Input validation
- Retry logic

```c
// Menu that shows at least once
do {
    show_menu();
    asm {
        IN 01H
        STA __VAR_choice
    }
} while (choice != 0);
```

---

## Advanced Examples

### Example 1: Nested Loops with Different Types
```c
int main() {
    int outer;
    int total = 0;

    // Outer for loop
    for (outer = 0; outer < 3; outer = outer + 1) {
        int inner = 0;

        // Inner do-while loop
        do {
            total = total + 1;
            inner = inner + 1;
        } while (inner < 4);
    }

    return total;  // Returns 12
}
```

### Example 2: All Three Loops Together
```c
int main() {
    int result = 0;
    int i;

    // While loop
    i = 0;
    while (i < 3) {
        result = result + 1;
        i = i + 1;
    }

    // For loop
    for (i = 0; i < 3; i = i + 1) {
        result = result + 1;
    }

    // Do-while loop
    i = 0;
    do {
        result = result + 1;
        i = i + 1;
    } while (i < 3);

    return result;  // Returns 9
}
```

### Example 3: For Loop with Inline Assembly
```c
void delay() {
    for (int i = 0; i < 100; i = i + 1) {
        asm {
            NOP
            NOP
            NOP
        }
    }
}
```

### Example 4: Do-While with Hardware I/O
```c
int read_sensor() {
    int value;

    do {
        asm {
            IN 10H
            STA __VAR_value
        }
    } while (value == 0xFF);  // Wait for non-error value

    return value;
}
```

---

## Test Files

| File | Tests |
|------|-------|
| `test_for_loop.c` | Basic for loop |
| `test_for_with_decl.c` | For with variable declaration |
| `test_do_while.c` | Basic do-while loop |
| `test_nested_loops.c` | Nested for and do-while |
| `test_all_loops.c` | All three loop types |

---

## Summary

✅ **while** - Condition before body, 0+ executions
✅ **for** - Init, condition, increment, 0+ executions
✅ **do-while** - Condition after body, 1+ executions

All loops fully supported with:
- Nested loops
- Variable declarations (for loop init)
- Inline assembly inside loops
- Pointers in loops
- Function calls in loops

Perfect for any i8080 programming need! 🚀
