int add(int a, int b) {
    return a + b;
}

int main() {
    int x;
    int y;
    int *ptr;
    int result;

    x = 10;
    y = 20;
    ptr = &x;

    result = add(*ptr, y);

    *ptr = 30;

    return result;
}
