// Returns the absolute value of an integer
int abs(int x) {
    if (x < 0) return -x;
    return x;
}

// Safe signed division wrapper
int divide(int a, int b) {
    int sign = 0;
    if (a < 0) { a = -a; sign = sign ^ 1; }
    if (b < 0) { b = -b; sign = sign ^ 1; }
    int res = a / b;
    if (sign) return -res;
    return res;
}

// Safe signed multiplication wrapper using native hardware '*'
int multiply(int a, int b) {
    int sign = 0;
    if (a < 0) { a = -a; sign = sign ^ 1; }
    if (b < 0) { b = -b; sign = sign ^ 1; }
    int res = a * b;
    if (sign) return -res;
    return res;
}

// High-speed 128-step Sine Lookup Table (sin * 100)
int get_sin(int a) {
    // Natively wrap positive and negative angles using bitwise AND!
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
        LHLD __VAR_a      ; Load 'a' into HL (0-127)
        LXI D, SIN_TABLE  ; Load table base address
        DAD D             ; HL = SIN_TABLE + a
        MOV L, M          ; Load the byte at that address
        MVI H, 0          ; Assume positive
        MOV A, L
        ANI 80H           ; Check sign bit
        JZ SIN_POS
        MVI H, 255        ; Sign-extend negative values
    SIN_POS:
        SHLD __VAR_res    ; Store to C variable
    }
    return res;
}

// Cosine is just sine shifted by 90 degrees (32 steps out of 128)
int get_cos(int a) {
    return get_sin(a + 32);
}

// Updates the direction text label on the screen
int update_dir_text(int dir) {
    if (dir == 0) puts_at(6, 1, "UP   ");
    if (dir == 1) puts_at(6, 1, "DOWN ");
    if (dir == 2) puts_at(6, 1, "RIGHT");
    if (dir == 3) puts_at(6, 1, "LEFT ");
    if (dir == 4) puts_at(6, 1, "SPIN ");
    return 0;
}

// Caches the newly projected frame into the 'old' buffer for erasing next loop
int save_old_coordinates(int *px, int *py, int *old_px, int *old_py) {
    for (reg int i = 0; i < 8; i = i + 1) {
        old_px[i] = px[i];
        old_py[i] = py[i];
    }
    return 0;
}

// Projects 3D Vertex (x,y,z) out to screen X/Y using 16-bit integer math
int project_vertex(int idx, int x, int y, int z, int ax, int ay, int *px_arr, int *py_arr) {
    // Rotate Y axis
    int sin_y = get_sin(ay);
    int cos_y = get_cos(ay);
    int x1 = divide(multiply(x, cos_y) + multiply(z, sin_y), 100);
    int z1 = divide(multiply(z, cos_y) - multiply(x, sin_y), 100);

    // Rotate X axis
    int sin_x = get_sin(ax);
    int cos_x = get_cos(ax);
    int y1 = divide(multiply(y, cos_x) - multiply(z1, sin_x), 100);
    int z2 = divide(multiply(z1, cos_x) + multiply(y, sin_x), 100);

    // Apply depth perspective and map to screen center (128x120)
    int z_dist = z2 + 150;
    px_arr[idx] = divide(multiply(x1, 128), z_dist) + 128;
    py_arr[idx] = divide(multiply(y1, 128), z_dist) + 120;

    return 0;
}

// Draws the 12 lines that make up the wireframe cube
int draw_cube_edges(int color, int use_old, int *px, int *py, int *old_px, int *old_py) {
    int p0x; int p0y; int p1x; int p1y; int p2x; int p2y; int p3x; int p3y;
    int p4x; int p4y; int p5x; int p5y; int p6x; int p6y; int p7x; int p7y;

    if (use_old) {
        p0x = old_px[0]; p0y = old_py[0];
        p1x = old_px[1]; p1y = old_py[1];
        p2x = old_px[2]; p2y = old_py[2];
        p3x = old_px[3]; p3y = old_py[3];
        p4x = old_px[4]; p4y = old_py[4];
        p5x = old_px[5]; p5y = old_py[5];
        p6x = old_px[6]; p6y = old_py[6];
        p7x = old_px[7]; p7y = old_py[7];
    } else {
        p0x = px[0]; p0y = py[0];
        p1x = px[1]; p1y = py[1];
        p2x = px[2]; p2y = py[2];
        p3x = px[3]; p3y = py[3];
        p4x = px[4]; p4y = py[4];
        p5x = px[5]; p5y = py[5];
        p6x = px[6]; p6y = py[6];
        p7x = px[7]; p7y = py[7];
    }

    // Bottom face
    draw_line(p0x, p0y, p1x, p1y, color);
    draw_line(p1x, p1y, p2x, p2y, color);
    draw_line(p2x, p2y, p3x, p3y, color);
    draw_line(p3x, p3y, p0x, p0y, color);

    // Top face
    draw_line(p4x, p4y, p5x, p5y, color);
    draw_line(p5x, p5y, p6x, p6y, color);
    draw_line(p6x, p6y, p7x, p7y, color);
    draw_line(p7x, p7y, p4x, p4y, color);

    // Connecting walls
    draw_line(p0x, p0y, p4x, p4y, color);
    draw_line(p1x, p1y, p5x, p5y, color);
    draw_line(p2x, p2y, p6x, p6y, color);
    draw_line(p3x, p3y, p7x, p7y, color);
    
    return 0;
}

int main() {
    int ax = 0;
    int ay = 0;
    int ax_step = 1;
    int ay_step = 1;
    int dir = 4;
    int last_dir = -1;
    
    int px[8];
    int py[8];
    int old_px[8];
    int old_py[8];
    
    clear_screen(0); // 0 = Clear both text and graphics layers
    
    // Draw static "DIR: " label at top left (x=1, y=1)
    puts_at(1, 1, "DIR: ");

    // Initialize old coordinates to 0 to prevent erratic first-frame erasure
    for (reg int i = 0; i < 8; i = i + 1) {
        old_px[i] = 0;
        old_py[i] = 0;
    }

    while (1) {
        int key = check_key();
        
        // Use raw PS/2 Scan Codes from the hardware (Port 0x00)
        // W (29) or Up Arrow (117)
        if (key == 29 || key == 117) { ax_step = 1; ay_step = 0; dir = 0; }
        // S (27) or Down Arrow (114)
        if (key == 27 || key == 114) { ax_step = -1; ay_step = 0; dir = 1; }
        // D (35) or Right Arrow (116)
        if (key == 35 || key == 116) { ax_step = 0; ay_step = 1; dir = 2; }
        // A (28) or Left Arrow (107)
        if (key == 28 || key == 107) { ax_step = 0; ay_step = -1; dir = 3; }
        
        if (dir != last_dir) {
            update_dir_text(dir);
            last_dir = dir;
        }

        // Calculate current rotation coordinates for the 8 corners (Size 30)
        project_vertex(0, -30, -30, -30, ax, ay, px, py);
        project_vertex(1,  30, -30, -30, ax, ay, px, py);
        project_vertex(2,  30,  30, -30, ax, ay, px, py);
        project_vertex(3, -30,  30, -30, ax, ay, px, py);
        project_vertex(4, -30, -30,  30, ax, ay, px, py);
        project_vertex(5,  30, -30,  30, ax, ay, px, py);
        project_vertex(6,  30,  30,  30, ax, ay, px, py);
        project_vertex(7, -30,  30,  30, ax, ay, px, py);
        
        draw_cube_edges(0, 1, px, py, old_px, old_py); // Erase Old Cube
        draw_cube_edges(1, 0, px, py, old_px, old_py); // Draw New Cube
        save_old_coordinates(px, py, old_px, old_py);
        
        ax = ax + ax_step;
        ay = ay + ay_step; 
    }
    return 0;
}