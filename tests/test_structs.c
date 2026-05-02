// test_structs.c

void print_newline() { print_char(13); print_char(10); }

void assert_test(int condition, char *test_name) {
    if (condition) print_string("[PASS] ");
    else print_string("[FAIL] ");
    print_string(test_name);
    print_newline();
}

struct Point {
    int x;
    int y;
};

struct Rect {
    struct Point p1;
    struct Point p2;
    int active;
};

union IntBytes {
    int i;
    char bytes[2];
};

int main() {
    clear_screen(1);
    set_cursor_xy(0, 0);

    // 1. Basic struct allocation and member access
    struct Point pt;
    pt.x = 100;
    pt.y = 200;
    assert_test(pt.x == 100 && pt.y == 200, "15. Structs: Member Access (.)");
    
    // 2. Nested structs
    struct Rect r;
    r.p1.x = 10;
    r.p2.y = 20;
    assert_test(r.p1.x == 10 && r.p2.y == 20, "15. Structs: Nested Members (r.p1.x)");
    
    // 3. Struct pointers and arrow operator
    struct Rect *ptr = &r;
    ptr->active = 1;
    ptr->p1.y = 15;
    assert_test(ptr->active == 1 && r.p1.y == 15, "15. Structs: Pointer Member Access (->)");
    
    // 4. Decayed pointer math
    struct Point *pptr = &r.p1;
    assert_test(pptr->x == 10, "15. Structs: Nested Address-of (&r.p1)");
    
    // 5. Array of structs
    struct Rect rects[3];
    rects[1].active = 1;
    rects[1].p2.y = 99;
    assert_test(rects[1].active == 1 && rects[1].p2.y == 99, "15. Structs: Array of structs (arr[idx].member)");

    // 6. Struct Initialization
    struct Point p_init = { 50, 60 };
    assert_test(p_init.x == 50 && p_init.y == 60, "15. Structs: Local Initialization");

    // 7. Array of Structs Initialization (flattened syntax)
    struct Rect r_init[2] = { { {1, 2}, {3, 4}, 1 }, { {5, 6}, {7, 8}, 0 } };
    assert_test(r_init[1].p1.x == 5 && r_init[0].active == 1, "15. Structs: Array initialization");

    // 8. Basic Union Behavior
    union IntBytes u;
    u.i = 258; // 258 = 0x0102. Low byte = 2, High byte = 1
    assert_test(u.bytes[0] == 2 && u.bytes[1] == 1, "16. Unions: Shared Memory Access");

    // 9. Union Initializations (Arrays of Unions)
    union IntBytes u_arr[2] = { {258}, {515} }; // 515 = 0x0203. Low byte = 3, High byte = 2
    assert_test(u_arr[1].bytes[0] == 3 && u_arr[1].bytes[1] == 2, "16. Unions: Array initialization");

    return 0;
}