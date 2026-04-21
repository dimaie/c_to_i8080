// Clears the 9600 bytes (0x2580) of Graphics RAM natively via HL and DE registers
int clear_gfx_ram() {
    asm {
        LXI H, 4000H
        LXI D, 2580H
    CLEAR_LOOP:
        MVI M, 00H
        INX H
        DCX D
        MOV A, D
        ORA E
        JNZ CLEAR_LOOP
    }
    return 0;
}

// Sets a single pixel handling the SAP-3's MSB-first packing using inline asm.
// Circumvents the 8-bit math limitations by computing offsets natively.
int put_pixel(int x, int y) {
    int bit_mask;
    int rem;
    
    asm {
        LDA __VAR_x
        ANI 07H
        STA __VAR_rem
    }
    
    if (rem == 0) bit_mask = 128;
    if (rem == 1) bit_mask = 64;
    if (rem == 2) bit_mask = 32;
    if (rem == 3) bit_mask = 16;
    if (rem == 4) bit_mask = 8;
    if (rem == 5) bit_mask = 4;
    if (rem == 6) bit_mask = 2;
    if (rem == 7) bit_mask = 1;
    
    asm {
        ; Calculate y * 40 + x / 8
        LXI H, 0
        LDA __VAR_y
        MOV C, A
        MVI B, 0
        MVI D, 40
    MUL_LOOP:
        MOV A, C
        ORA A
        JZ MUL_DONE
        DAD D
        DCR C
        JMP MUL_LOOP
    MUL_DONE:
        
        LXI D, 4000H
        DAD D
        
        LDA __VAR_x
        RRC
        RRC
        RRC
        ANI 1FH
        MOV E, A
        MVI D, 0
        DAD D
        
        LDA __VAR_bit_mask
        ORA M
        MOV M, A
    }
    return 0;
}

// Plots the 8 octant symmetry points for Bresenham's algorithm
int draw_circle_points(int xc, int yc, int x, int y) {
    put_pixel(xc + x, yc + y);
    put_pixel(xc - x, yc + y);
    put_pixel(xc + x, yc - y);
    put_pixel(xc - x, yc - y);
    put_pixel(xc + y, yc + x);
    put_pixel(xc - y, yc + x);
    put_pixel(xc + y, yc - x);
    put_pixel(xc - y, yc - x);
    return 0;
}

// Bresenham's Circle Algorithm 
// Translated to midpoint equations scaled to prevent exceeding signed 8-bit limit.
int draw_circle(int xc, int yc, int r) {
    int x = 0;
    int y = r;
    int d = 1 - r;
    int dx;
    
    draw_circle_points(xc, yc, x, y);
    
    while (!(y < x)) {
        x = x + 1;
        if (0 < d) {
            y = y - 1;
            dx = x - y;
            d = d + dx + dx + 5;
        } else {
            d = d + x + x + 3;
        }
        draw_circle_points(xc, yc, x, y);
    }
    return 0;
}

int main() {
    int xc = 160;
    int yc = 120;
    int r = 20;
    int i;
    
    clear_gfx_ram();
    
    // Draw 10 concentric circles, increasing radius by 5 each time
    for (i = 0; i < 10; i = i + 1) {
        draw_circle(xc, yc, r);
        r = r + 5;
    }
    
    // Infinite loop to halt the program
    while (1) {}
    
    return 0;
}