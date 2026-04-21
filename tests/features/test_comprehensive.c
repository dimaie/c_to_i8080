// Comprehensive example showing all optimizations
int multiply(int a, int b) {
    return a * b;
}

int main() {
    int x;
    int y;
    int *ptr;
    int result;

    x = 10;
    y = 5;

    // Use pointer operations
    ptr = &x;
    *ptr = 15;

    // Use multiplication (triggers __mul inclusion)
    result = multiply(x, y);

    return result;
}
