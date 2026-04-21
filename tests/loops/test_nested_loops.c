// Test nested loops with different types
int main() {
    int outer;
    int total;

    total = 0;

    // Outer for loop
    for (outer = 0; outer < 3; outer = outer + 1) {
        int inner = 0;

        // Inner do-while loop
        do {
            total = total + 1;
            inner = inner + 1;
        } while (inner < 4);
    }

    return total;  // Should return 12 (3 * 4)
}
