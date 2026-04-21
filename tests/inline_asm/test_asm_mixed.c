// Mixing C code with inline assembly
int add(int a, int b) {
    return a + b;
}

int main() {
    int x;
    int y;
    int sum;

    x = 10;
    y = 20;

    // Call C function
    sum = add(x, y);

    // Then use inline assembly for I/O
    asm {
        LDA __VAR_sum
        OUT 01H
    }

    // More C code
    if (sum > 25) {
        asm {
            MVI A, 01H
            OUT 02H
        }
    }

    return sum;
}
