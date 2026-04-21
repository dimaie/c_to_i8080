// Test for loop with variable declaration in init
int main() {
    int sum = 0;

    // For loop with declaration in initialization
    for (int i = 0; i < 5; i = i + 1) {
        sum = sum + i;
    }

    return sum;  // Should return 10 (0+1+2+3+4)
}
