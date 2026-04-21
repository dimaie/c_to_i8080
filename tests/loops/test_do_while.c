// Test do-while loop
int main() {
    int count;
    int sum;

    count = 0;
    sum = 0;

    // Do-while loop: executes at least once
    do {
        sum = sum + count;
        count = count + 1;
    } while (count < 5);

    return sum;  // Should return 10 (0+1+2+3+4)
}
