// High-speed 128-step Sine Lookup Table (sin * 100)
int get_sin(int a) {
    a = a & 127;
    int res;
    asm {
        JMP SKIP_SIN_TABLE
    SIN_TABLE:
        DB 0, 5, 10, 15, 20, 24, 29, 34, 38, 43, 47, 51, 56, 60, 63, 67
        DB 71, 74, 77, 80, 83, 86, 88, 90, 92, 94, 96, 97, 98, 99, 100, 100
        DB 100, 100, 100, 99, 98, 97, 96, 94, 92, 90, 88, 86, 83, 80, 77, 74
        DB 71, 67, 63, 60, 56, 51, 47, 43, 38, 34, 29, 24, 20, 15, 10, 5
        DB 0, 251, 246, 241, 236, 232, 227, 222, 218, 213, 209, 205, 200, 196, 193, 189
        DB 185, 182, 179, 176, 173, 170, 168, 166, 164, 162, 160, 159, 158, 157, 156, 156
        DB 156, 156, 156, 157, 158, 159, 160, 162, 164, 166, 168, 170, 173, 176, 179, 182
        DB 185, 189, 193, 196, 200, 205, 209, 213, 218, 222, 227, 232, 236, 241, 246, 251
    SKIP_SIN_TABLE:
        LHLD __VAR_a
        LXI D, SIN_TABLE
        DAD D
        MOV L, M
        MVI H, 0
        MOV A, L
        ANI 80H
        JZ SIN_POS
        MVI H, 255
    SIN_POS:
        SHLD __VAR_res
    }
    return res;
}

int get_cos(int a) {
    return get_sin(a + 32);
}

// Recursively draws the squares and triangles of the Pythagoras Tree
int draw_tree(int x, int y, int size, int angle, int depth, int skew) {
    if (depth == 0 || size < 2) return 0;
    
    int sin_a = get_sin(angle);
    int cos_a = get_cos(angle);
    
    // Calculate base of the square
    int p1x = x + divide(multiply(size, cos_a), 100);
    int p1y = y + divide(multiply(size, sin_a), 100);
    
    // "Forward" direction is 90 degrees CCW (Subtract 32 in our 128-step table)
    int fwd_angle = (angle - 32) & 127;
    int sin_f = get_sin(fwd_angle);
    int cos_f = get_cos(fwd_angle);
    
    // Calculate top edge of the square
    int p2x = p1x + divide(multiply(size, cos_f), 100);
    int p2y = p1y + divide(multiply(size, sin_f), 100);
    int p3x = x + divide(multiply(size, cos_f), 100);
    int p3y = y + divide(multiply(size, sin_f), 100);
    
    // Draw current square
    draw_line(x, y, p1x, p1y, 1);
    draw_line(p1x, p1y, p2x, p2y, 1);
    draw_line(p2x, p2y, p3x, p3y, 1);
    draw_line(p3x, p3y, x, y, 1);
    
    // Calculate asymmetric branching
    int left_angle = (angle - skew) & 127;
    int right_angle = (left_angle + 32) & 127; // Right angle is 90 deg (32 steps) from left
    
    int left_size = divide(multiply(size, get_cos(skew)), 100);
    int right_size = divide(multiply(size, get_sin(skew)), 100);

    int sin_l = get_sin(left_angle);
    int cos_l = get_cos(left_angle);
    
    // Calculate tip of the triangle
    int p4x = p3x + divide(multiply(left_size, cos_l), 100);
    int p4y = p3y + divide(multiply(left_size, sin_l), 100);
    
    // Draw the top triangle cap
    draw_line(p3x, p3y, p4x, p4y, 1);
    draw_line(p4x, p4y, p2x, p2y, 1);
    
    // Recursive calls for next fractal layer
    draw_tree(p3x, p3y, left_size, left_angle, depth - 1, skew);
    draw_tree(p4x, p4y, right_size, right_angle, depth - 1, skew);
    
    return 0;
}

int main() {
    int depth = 5;
    int skew = 16; // Start perfectly balanced (45 degrees = 16 steps)
    int needs_redraw = 1;
    
    while (1) {
        if (needs_redraw) {
            clear_screen(0);
            puts_at(1, 1, "PYTHAGORAS TREE");
            puts_at(1, 2, "RENDERING...   ");
            
            // Draw tree starting at center-left, with adjustable skew
            draw_tree(108, 220, 40, 0, depth, skew);
            
            puts_at(1, 2, "DONE. ARROWS   ");
            needs_redraw = 0;
        }
        
        // Custom blocking loop to read raw PS/2 codes
        int is_break = 0;
        int key = 0;
        while (key == 0) {
            int code = check_key();
            if (code != 0) {
                if (code == 240) { // 0xF0 (Break Code)
                    is_break = 1;
                } else if (code != 224) { // Ignore 0xE0 (Extended Prefix)
                    if (is_break == 1) {
                        is_break = 0; // Break sequence finished
                        key = code;   // Trigger action on key RELEASE!
                    } else {
                        // Ignore MAKE codes to prevent orphaned tail-byte bugs
                    }
                }
            }
        }
        
        if (key == 117) { // Up Arrow (0x75)
            if (depth < 7) { depth = depth + 1; needs_redraw = 1; }
        }
        if (key == 114) { // Down Arrow (0x72)
            if (depth > 1) { depth = depth - 1; needs_redraw = 1; }
        }
        if (key == 116) { // Right Arrow (0x74)
            if (skew < 28) { skew = skew + 2; needs_redraw = 1; }
        }
        if (key == 107) { // Left Arrow (0x6B)
            if (skew > 4) { skew = skew - 2; needs_redraw = 1; }
        }
        
        wait_vblank();
    }
    return 0;
}