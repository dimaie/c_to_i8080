// test_advanced.c
// Advanced features: Control Flow, Recursion, Inline ASM, Functions, Function Pointers
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
    print_newline();
}

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
int pass_arr(int arr[]) { return arr + arr; }
int pass_many(char c, int i, short s, int *p) { return c + i + s + *p; }

void test_functions() {
    assert_test(pass_char(10) == 11, "12. Functions: char param/return");
    assert_test(pass_int(1000) == 1001, "12. Functions: int param/return");
    assert_test(pass_short(500) == 501, "12. Functions: short param/return");
    
    int val = 42;
    int *p = &val;
    assert_test(pass_ptr(p) == p + 1, "12. Functions: pointer param/return");
    
    int arr;
    arr = 5; arr = 10;
    assert_test(pass_arr(arr) == 15, "12. Functions: array param");
    
    assert_test(pass_many(10, 1000, 500, p) == 1552, "12. Functions: multiple mixed params");
}

int add_op(int a, int b) { return a + b; }
int sub_op(int a, int b) { return a - b; }

// A higher-order function that takes a function pointer as its first parameter
int execute_op(int (*op)(int, int), int a, int b) {
    return op(a, b);
}

void test_function_pointers() {
    int (*fp)(int, int) = add_op;
    assert_test(fp(20, 10) == 30, "13. Function Pointers: direct assignment & call");
    
    fp = sub_op;
    assert_test(fp(20, 10) == 10, "13. Function Pointers: reassignment");
    
    int (*fp_addr)(int, int) = &add_op;
    assert_test(fp_addr(10, 5) == 15, "13. Function Pointers: address-of (&)");

    assert_test((*fp_addr)(10, 5) == 15, "13. Function Pointers: explicit deref (*fp)()");

    // Passing functions as arguments
    assert_test(execute_op(add_op, 50, 25) == 75, "13. Function Pointers: pass as parameter (add)");
    assert_test(execute_op(sub_op, 50, 25) == 25, "13. Function Pointers: pass as parameter (sub)");
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
    print_string(" SAP-3 ADVANCED FEATURE TESTS");     print_newline();
    print_string("================================="); print_newline();
    print_newline();
    
    wait_for_key();
    
    test_control_flow();
    current_sp = get_sp(); check_sp("test_control_flow"); wait_for_key();
    
    test_recursion();
    current_sp = get_sp(); check_sp("test_recursion"); wait_for_key();
    
    test_inline_asm();
    current_sp = get_sp(); check_sp("test_inline_asm"); wait_for_key();
    
    test_functions();
    current_sp = get_sp(); check_sp("test_functions"); wait_for_key();
    
    test_function_pointers();
    current_sp = get_sp(); check_sp("test_function_pointers");
    
    print_newline();
    print_string("ADVANCED TESTS EXECUTED.");
    print_newline();
    wait_for_key();
    
    return 0;
}