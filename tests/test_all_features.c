// test_all_features.c
// Tests for all compiler features on SAP-3 architecture
// Demonstrates integration with the SAP-3 System Monitor (ROM API)

// ---------------------------------------------------------
// Testing Framework
// ---------------------------------------------------------

void print_newline() {
    print_char(13); // CR
    print_char(10); // LF
}

void assert_test(int condition, char *test_name) {
    if (condition) {
        print_string("[PASS] ");
    } else {
        print_string("[FAIL] ");
    }
    print_string(test_name);
    print_newline();
}

void wait_for_key() {
    print_string("Press any key to continue...");
    read_key();
    print_newline();}

int initial_sp;
int current_sp;

int get_sp() {
    int sp_val;
    asm {
        LXI H, 0
        DAD SP
        SHLD __VAR_sp_val
    }
    return sp_val;
}

void check_sp(char *test_name) {
    if (current_sp != initial_sp) {
        print_string("[FATAL] Stack mismatch after ");
        print_string(test_name);
        print_newline();
    } else {
        print_string("[PASS] Stack integrity maintained after ");
        print_string(test_name);
        print_newline();
    }
}

// ---------------------------------------------------------
// Feature Tests
// ---------------------------------------------------------

void test_types() {
    char c = 200;
    int i = 30000;
    short s = 25000;
    
    assert_test(c == 200 && i == 30000 && s == 25000, "1. Data Types (char, int, short)");
}

int static_counter() {
    static int c = 0;
    c = c + 1;
    return c;
}

void test_modifiers() {
    reg int r = 42;
    r = r + 1;
    assert_test(r == 43, "2. Storage Modifiers (reg BC)");

    static_counter();
    assert_test(static_counter() == 2, "2. Storage Modifiers (static)");
}

void test_arithmetic() {
    int a = 10;
    int b = 3;
    assert_test(a + b == 13, "3. Arithmetic Add (+)");
    assert_test(a - b == 7, "3. Arithmetic Sub (-)");
    assert_test(a * b == 30, "3. Arithmetic Mul (* via MUL B)");
    assert_test(a / b == 3, "3. Arithmetic Div (/)");
    assert_test(a % b == 1, "3. Arithmetic Mod (%)");
}

void test_bitwise() {
    int x = 10; // binary: 1010
    int y = 12; // binary: 1100
    assert_test((x & y) == 8,  "4. Bitwise AND (&)");
    assert_test((x | y) == 14, "4. Bitwise OR (|)");
    assert_test((x ^ y) == 6,  "4. Bitwise XOR (^)");
    assert_test((~0) == -1,    "4. Bitwise NOT (~)");
    assert_test((1 << 3) == 8, "4. Bitwise SHL (<<)");
    assert_test((16 >> 2) == 4,"4. Bitwise SHR (>>)");
}

void test_unary() {
    int x = 5;
    int y = x++;
    assert_test(y == 5 && x == 6, "5. Postfix Increment (x++)");
    
    x = 5; y = ++x;
    assert_test(y == 6 && x == 6, "5. Prefix Increment (++x)");
    
    x = 5; y = x--;
    assert_test(y == 5 && x == 4, "5. Postfix Decrement (x--)");
    
    x = 5; y = --x;
    assert_test(y == 4 && x == 4, "5. Prefix Decrement (--x)");
    
    assert_test(-x == -4, "5. Unary Minus (-)");
    assert_test(!0 == 1 && !x == 0, "5. Logical NOT (!)");
}

void test_assignment() {
    int x = 10;
    x += 5; assert_test(x == 15, "6. Compound Assign (+=)");
    x -= 3; assert_test(x == 12, "6. Compound Assign (-=)");
    x *= 2; assert_test(x == 24, "6. Compound Assign (*=)");
    x /= 4; assert_test(x == 6,  "6. Compound Assign (/=)");
}

void test_comparison() {
    assert_test(5 == 5, "7. Relational Equal (==)");
    assert_test(5 != 4, "7. Relational Not Equal (!=)");
    assert_test(3 < 5,  "7. Relational Less (<)");
    assert_test(5 <= 5, "7. Relational Less Equal (<=)");
    assert_test(6 > 4,  "7. Relational Greater (>)");
    assert_test(6 >= 6, "7. Relational Greater Equal (>=)");
    assert_test((1 == 1) && (2 == 2), "7. Logical AND (&&)");
    assert_test((1 == 0) || (2 == 2), "7. Logical OR (||)");
}

void test_pointers() {
    int val = 42;
    int *ptr = &val;
    assert_test(*ptr == 42, "8. Pointers (& and *)");
    
    *ptr = 99;
    assert_test(val == 99, "8. Pointer Dereference Assign");

    int arr[3];
    arr[0] = 10; arr[1] = 20; arr[2] = 30;
    assert_test(arr[0] == 10 && arr[1] == 20 && arr[2] == 30, "8. Arrays (arr[idx])");
    
    int *aptr = arr;
    assert_test(*aptr == 10, "8. Array Decay to Pointer");

    int *aptr2 = aptr + 2;
    assert_test(*aptr2 == 20, "8. Pointer Math (int +2 bytes)");

    char carr[3];
    carr[0] = 5; carr[1] = 15; carr[2] = 25;
    assert_test(carr[0] == 5 && carr[1] == 15 && carr[2] == 25, "8. Arrays (char arr[idx])");
    
    char *cptr = carr;
    char *cptr2 = cptr + 1;
    assert_test(*cptr2 == 15, "8. Pointer Math (char +1 byte)");

    short sarr[3];
    sarr[0] = 100; sarr[1] = 200; sarr[2] = 300;
    assert_test(sarr[0] == 100 && sarr[1] == 200 && sarr[2] == 300, "8. Arrays (short arr[idx])");
    
    short *sptr = sarr;
    short *sptr2 = sptr + 2;
    assert_test(*sptr2 == 200, "8. Pointer Math (short +2 bytes)");
}

void test_control_flow() {
    int a = 0;
    if (1) a = 1; else a = 2;
    assert_test(a == 1, "9. Control Flow: if / else");

    int sum = 0;
    int i = 0;
    while (i < 5) { sum += i; i++; }
    assert_test(sum == 10, "9. Control Flow: while");

    sum = 0; i = 0;
    do { sum += i; i++; } while (i < 5);
    assert_test(sum == 10, "9. Control Flow: do-while");

    sum = 0;
    for (int j = 0; j < 5; j++) sum += j;
    assert_test(sum == 10, "9. Control Flow: for loop");

    sum = 0;
    for (int k = 0; k < 10; k++) {
        if (k == 2) continue;
        if (k == 5) break;
        sum += k;
    }
    assert_test(sum == 8, "9. Control Flow: break & continue");
}

int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

void test_recursion() {
    assert_test(factorial(5) == 120, "10. Memory Model: Safe Recursion");
}

void test_inline_asm() {
    int asm_val = 0;
    asm {
        LXI H, 1337
        SHLD __VAR_asm_val
    }
    assert_test(asm_val == 1337, "11. Inline Assembly (__VAR_ sub)");
}

char pass_char(char c) { return c + 1; }
int pass_int(int i) { return i + 1; }
short pass_short(short s) { return s + 1; }
int* pass_ptr(int *p) { return p + 1; }
int pass_arr(int arr[]) { return arr[0] + arr[1]; }
int pass_many(char c, int i, short s, int *p) { return c + i + s + *p; }

void test_functions() {
    assert_test(pass_char(10) == 11, "12. Functions: char param/return");
    assert_test(pass_int(1000) == 1001, "12. Functions: int param/return");
    assert_test(pass_short(500) == 501, "12. Functions: short param/return");
    
    int val = 42;
    int *p = &val;
    assert_test(pass_ptr(p) == p + 1, "12. Functions: pointer param/return");
    
    int arr[2];
    arr[0] = 5; arr[1] = 10;
    assert_test(pass_arr(arr) == 15, "12. Functions: array param");
    
    assert_test(pass_many(10, 1000, 500, p) == 1552, "12. Functions: multiple mixed params");
}

// Uncalled function to test Dead Code Elimination silently
void dead_code_function() {
    int unused = 0;
    unused = unused + 1;
}

// ---------------------------------------------------------
// Main Execution
// ---------------------------------------------------------

int main() {
    clear_screen(1);
    set_cursor_xy(0, 0);

    initial_sp = get_sp();

    print_string("================================="); print_newline();
    print_string(" SAP-3 COMPILER FEATURE TESTS");    print_newline();
    print_string("================================="); print_newline();
    print_newline();
    
    wait_for_key();
    
    test_types();
    current_sp = get_sp();
    check_sp("test_types");
    wait_for_key();
    test_modifiers();
    current_sp = get_sp();
    check_sp("test_modifiers");
    wait_for_key();
    test_arithmetic();
    current_sp = get_sp();
    check_sp("test_arithmetic");
    wait_for_key();
    test_bitwise();
    current_sp = get_sp();
    check_sp("test_bitwise");
    wait_for_key();
    test_unary();
    current_sp = get_sp();
    check_sp("test_unary");
    wait_for_key();
    test_assignment();
    current_sp = get_sp();
    check_sp("test_assignment");
    wait_for_key();
    test_comparison();
    current_sp = get_sp();
    check_sp("test_comparison");
    wait_for_key();
    test_pointers();
    current_sp = get_sp();
    check_sp("test_pointers");
    wait_for_key();
    test_control_flow();
    current_sp = get_sp();
    check_sp("test_control_flow");
    wait_for_key();
    test_recursion();
    current_sp = get_sp();
    check_sp("test_recursion");
    wait_for_key();
    test_inline_asm();
    current_sp = get_sp();
    check_sp("test_inline_asm");
    wait_for_key();
    test_functions();
    current_sp = get_sp();
    check_sp("test_functions");
    
    print_newline();
    print_string("ALL TESTS EXECUTED.");
    print_newline();
    wait_for_key();
    
    return 0;
}