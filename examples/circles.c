// Clears the 9600 bytes (0x2580) of Graphics RAM natively using C pointers
int clear_gfx_ram() {
    int *vram = 16384; // Base address 0x4000
    
    // Clear 2 bytes at a time (9600 bytes / 2 = 4800)
    for (int i = 0; i < 4800; i = i + 1) {
        *vram = 0;
        vram = vram + 2; // Pointer arithmetic doesn't scale by type, so we add 2 explicitly
    }
    return 0;
}

// Sets a single pixel handling the SAP-3's MSB-first packing.
// Leverages 16-bit math compiler support and native bitwise operators.
int put_pixel(int x, int y) {
    int *vram_addr = 16384 + (y << 5) + (y << 3) + (x >> 3); // Emulate y * 40 + x / 8
    int rem = x & 7; // Calculate modulo using bitwise AND (much faster than division loop)
    int mask = 128 >> rem;
    
    *vram_addr = *vram_addr | mask;
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
    
    draw_circle_points(xc, yc, x, y);
    
    while (!(y < x)) {
        x = x + 1;
        if (0 < d) {
            y = y - 1;
            d = d + x - y + x - y + 5;
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
    
    clear_gfx_ram();
    
    // Draw 10 concentric circles, increasing radius by 5 each time
    for (int i = 0; i < 10; i = i + 1) {
        draw_circle(xc, yc, r);
        r = r + 5;
    }
    
    // Infinite loop to halt the program
    while (1) {}
    
    return 0;
}