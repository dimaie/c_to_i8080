int main() {
    int x;
    int *ptr;
    int y;

    x = 42;
    ptr = &x;
    y = *ptr;

    *ptr = 100;

    return y;
}
