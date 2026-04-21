// Comprehensive loop test - all three types
int main() {
    int result;
    int i;

    result = 0;

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

    return result;  // Should return 9 (3+3+3)
}
