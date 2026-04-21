// Test inline assembly
int main() {
    int x;
    
    x = 42;
    
    // Inline assembly to directly manipulate hardware
    asm {
        MVI A, 0xFF
        OUT 10H
        NOP
        NOP
    }
    
    return x;
}
