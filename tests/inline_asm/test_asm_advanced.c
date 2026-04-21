// Advanced inline assembly example
int main() {
    int port_value;
    int result;

    port_value = 0x80;

    // Read from I/O port
    asm {
        IN 20H
        STA __VAR_result
    }

    // Complex assembly block
    asm {
        LDA __VAR_port_value
        OUT 30H
        MVI B, 10
        LOOP:
            DCR B
            JNZ LOOP
    }

    return result;
}
