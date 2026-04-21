// Comprehensive example demonstrating all compiler features
// - Direct memory addressing
// - Conditional runtime functions
// - Pointer operations
// - Inline assembly

// Function using multiplication (includes __mul)
int multiply(int a, int b) {
    return a * b;
}

// Function with pointers
int increment_via_pointer(int *ptr) {
    *ptr = *ptr + 1;
    return *ptr;
}

int main() {
    int value;
    int *pointer;
    int result;
    int port_status;

    // Initialize variables (direct memory access)
    value = 10;
    pointer = &value;

    // Use pointers
    result = increment_via_pointer(pointer);

    // Use multiplication (triggers __mul inclusion)
    result = multiply(result, 3);

    // Inline assembly for hardware I/O
    asm {
        ; Read status from hardware port
        IN 01H
        STA __VAR_port_status
    }

    // Conditional logic
    if (port_status > 0) {
        asm {
            ; Send ready signal
            MVI A, 0FFH
            OUT 02H
        }

        result = result + 10;
    } else {
        asm {
            ; Send error signal
            MVI A, 00H
            OUT 02H
        }
    }

    // Output result to hardware
    asm {
        LDA __VAR_result
        OUT 03H
    }

    // Return via pointer dereference
    return *pointer;
}
