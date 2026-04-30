# C to i8080 Compiler Features & Implementation Document

## 1. Data Types and Variables
### Supported Types
The compiler distinguishes between 8-bit and 16-bit data types.
*   **`char`**: 8-bit integer.
*   **`int`** / **`short`**: 16-bit integer.
*   **`void`**: Used for function return types indicating no return value.
*   **Pointers**: E.g., `int *ptr` or `char *ptr`. Always 16-bit addresses.
*   **Arrays**: E.g., `int arr[10]`.

### Storage Modifiers
*   **`reg` / `register`**: Instructs the compiler to allocate the variable directly to the i8080's `BC` register pair instead of RAM, providing massive speedups for loop counters (e.g., `reg int i = 0;`). The compiler tracks its usage (`uses_bc`) and automatically preserves `BC` on the stack across function boundaries.
*   **`static`**: Forces the variable to be allocated at a fixed memory address, skipping the stack frame or shadow-stack pushing/popping. Preserves the value across function calls.

### Implementation Details
*   **Parser**: Detects these types and sets the `datatype` field on the `ASTNode` (1 for 8-bit, 2 for 16-bit). For arrays, the `array_size` property is populated. Pointer declarations prepend a `*` to the variable's string value as a simple metadata hack.
*   **Codegen**: 8-bit types are loaded/stored using the `A` register (and zero-extended to `HL`). 16-bit types use `LHLD`/`SHLD` or the `HL` register pair directly.

---

## 2. Expressions and Operators
### Arithmetic & Bitwise
*   **Math**: `+`, `-`, `*`, `/`, `%`
*   **Bitwise**: `&`, `|`, `^`, `~`, `<<`, `>>`
*   **Unary/Postfix**: `++`, `--`, `-`, `!` (Logical NOT)
*   **Assignment**: `=`, `+=`, `-=`, `*=`, `/=`

### Comparison & Logical
*   **Relational**: `==`, `!=`, `<`, `<=`, `>`, `>=`
*   **Logical**: `&&`, `||` (Implemented with short-circuit-like label branching)

### Implementation Details
*   **Binary Operations**: The left operand is compiled into `HL`, pushed to the stack, then the right operand is compiled into `HL`. The left is popped into `DE`, and the operation is performed between `DE` and `HL`.
*   **Hardware Math Acceleration**: For multiplication (`*`), the compiler generates `MUL B` (a custom non-standard opcode `0xED` on the SAP-3 architecture) between `A` and `B` instead of a slow software loop.
*   **Software Routines**: Division (`/`) and Modulo (`%`) emit calls to `__div` and `__mod`. The compiler tracks if these are used (`compiler->uses_div`, `compiler->uses_mod`) and only injects the subroutine assembly at the end of the file if necessary, saving ROM space.

---

## 3. Pointers and Arrays
### Capabilities
*   **Address-Of (`&`)**: Fetches the memory address of a variable.
*   **Dereference (`*`)**: Reads or writes to the memory address pointed to.
*   **Array Access (`arr[idx]`)**: Decays arrays into pointers and calculates offsets natively.

### Implementation Details
*   **Array Decay**: When an array is accessed (`AST_ARRAY_ACCESS`), the compiler automatically loads its base address.
*   **Offset Calculation**: It evaluates the index into `HL`. If the target type is 16-bit, it emits a `DAD H` (HL = HL * 2) to scale the index, adds it to the base address (`DAD D`), and dereferences the location.
*   *(Note: Explicit pointer arithmetic like `ptr + 1` does NOT auto-scale by the data type in this compiler. Adding 1 to an `int*` increments the address by 1 byte, not 2).*

---

## 4. Control Flow
### Supported Structures
*   **`if` / `else`**: Standard conditional branching.
*   **`while (cond)`**: Condition is evaluated before the loop body.
*   **`do { ... } while (cond)`**: Body executes at least once; evaluated at the end.
*   **`for (init; cond; inc)`**: Supports inline variable declarations (e.g., `for (int i=0;...)`).
*   **`break` / `continue`**: Jumps out of loops or skips to the next iteration.

### Implementation Details
*   **Label Generation**: `codegen.c` uses a monotonic `label_counter` to generate unique jump labels (e.g., `L1`, `L2`).
*   **Tracking Branches**: The compiler tracks the nearest loop boundaries using `compiler->current_break_label` and `compiler->current_continue_label`. This allows `break` and `continue` to know exactly which `L{x}` label to jump to, even in nested structures.

---

## 5. Memory Models & Function Calling
The compiler supports two distinct memory allocation strategies, configurable via the `use_frame_pointer` flag:

### Model A: Frame Pointer (Stack-Based)
*   Standard C approach. Allocates local space dynamically on the hardware stack.
*   Uses `__FP` (Frame Pointer) to track the base of the current function's variables.
*   **Pros**: Fully re-entrant, supports deep recursion safely.
*   **Cons**: Slower execution and larger code size due to stack math (`DAD D`, etc.).

### Model B: Direct Memory (Static + Shadow Stack)
*   Assigns every local variable to a fixed, absolute memory address named `__VAR_{func}_{name}`.
*   **Pros**: Incredibly fast. Resolves variables instantly via `LDA` / `STA` / `LHLD` / `SHLD`. 
*   **Recursion Fix**: To prevent nested calls from overwriting static addresses, the function *Prologue* pushes the current static variable values onto a "Shadow Stack", and the *Epilogue* pops them back, restoring the previous caller's state.

### Parameters & Returns
*   **Calling**: Arguments are pushed onto the stack right-to-left.
*   **Receiving**: The function reads arguments out of the stack and writes them to their local variables (either relative to `__FP` or to static `__VAR_` addresses).
*   **Return Values**: Always passed back through the `HL` register (or `A` extended to `HL`). The caller cleans up the arguments from the stack.

---

## 6. Advanced/Special Features

### Inline Assembly (`asm { ... }`)
*   **Parser**: The `lexer.c` looks for `asm` followed by `{`. It treats the entire block as a raw string until the closing `}`, bypassing standard syntax tokenization.
*   **Variable Substitution**: Inside the block, if the code references `__VAR_{name}`, the code generator intercepts it and automatically substitutes the fully-qualified static variable name (e.g., `__VAR_main_name`). 

### String Literals
*   Strings (e.g., `"Hello"`) are processed directly inline.
*   The code generator handles this by generating a jump instruction to skip over the data, then dumps the string into the binary as raw ASCII bytes (`DB`) ending in a `0` (null terminator), and puts the pointer address into `HL`.

### Dead Code Elimination (DCE)
*   Implemented via `mark_function_used()` in `codegen.c`.
*   The compiler starts at `main` and recursively traverses the AST, marking every function invoked via `AST_CALL`.
*   Any function defined in the C code that is not marked as `is_used` is entirely skipped during the assembly generation phase, preventing unused library functions from bloating the ROM.
*   The DCE algorithm also scans raw inline assembly strings to protect functions that are manually invoked via `CALL func_name` inside `asm { }` blocks!