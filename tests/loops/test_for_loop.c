// Test for loop
int main() {
    int sum;
    int i;

    sum = 0;

    // Classic for loop: sum numbers from 0 to 9
    for (i = 0; i < 10; i = i + 1) {
        sum = sum + i;
    }

    return sum;  // Should return 45
}
