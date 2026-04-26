// Clears the 9600 bytes of Graphics RAM using native pointers
int clear_gfx_ram() {
    int *vram = 16384; // Base address 0x4000
    for (int i = 0; i < 4800; i = i + 1) {
        *vram = 0;
        vram = vram + 2; 
    }
    return 0;
}

// Sets or clears a single pixel safely 
int put_pixel(int x, int y, int color) {
    if (x < 0) return 0;
    if (x >= 320) return 0;
    if (y < 0) return 0;
    if (y >= 240) return 0;
    
    int *vram_addr = 16384 + (y << 5) + (y << 3) + (x >> 3);
    int rem = x & 7;
    int mask = 128 >> rem;
    
    if (color) {
        *vram_addr = *vram_addr | mask;
    } else {
        *vram_addr = *vram_addr & (~mask);
    }
    return 0;
}

// Draws a 16x16 sprite from an array
int draw_sprite(int x, int y, int *sprite, int color) {
    for (int row = 0; row < 16; row = row + 1) {
        int row_data = sprite[row]; // Native Array Access!
        
        for (int col = 0; col < 16; col = col + 1) {
            // If the 16-bit number shifted left has its MSB set to 1, 
            // the compiler natively treats it as negative (< 0). 
            // This is a super fast way to test bits!
            if ((row_data << col) < 0) {
                put_pixel(x + col, y + row, color);
            }
        }
    }
    return 0;
}

int main() {
    // Look ma, no assembly! Native array declarations:
    int frame1[16];
    int frame2[16];
    
    // Frame 1: Legs apart, arms out
    frame1[0] = 384;   //       **
    frame1[1] = 960;   //     ****
    frame1[2] = 960;   //     ****
    frame1[3] = 384;   //       **
    frame1[4] = 384;   //       **
    frame1[5] = 4080;  //     ********
    frame1[6] = 6552;  //    **  **  **
    frame1[7] = 12684; //  **   **   **
    frame1[8] = 8580;  //  *    **    *
    frame1[9] = 384;   //       **
    frame1[10]= 960;   //     ****
    frame1[11]= 1632;  //    **  **
    frame1[12]= 3120;  //   **    **
    frame1[13]= 6168;  //  **      **
    frame1[14]= 12300; // **        **
    frame1[15]= 24582; // **        **
    
    // Frame 2: Legs together, arms in
    frame2[0] = 384;   //       **
    frame2[1] = 960;   //     ****
    frame2[2] = 960;   //     ****
    frame2[3] = 384;   //       **
    frame2[4] = 384;   //       **
    frame2[5] = 1008;  //       ******
    frame2[6] = 1424;  //      * ** *
    frame2[7] = 2376;  //     *  **  *
    frame2[8] = 4484;  //    *   **   *
    frame2[9] = 384;   //       **
    frame2[10]= 384;   //       **
    frame2[11]= 384;   //       **
    frame2[12]= 960;   //      ****
    frame2[13]= 384;   //       **
    frame2[14]= 960;   //      ****
    frame2[15]= 1536;  //      **
    
    clear_gfx_ram();
    
    int x = 0;
    int y = 110;
    int current_frame = 0;
    
    while (1) {
        // 1. Erase old sprite
        if (current_frame == 0) draw_sprite(x, y, frame1, 0);
        if (current_frame == 1) draw_sprite(x, y, frame2, 0);
        
        // 2. Update position
        x = x + 6;
        if (x > 320) x = -16;
        
        // 3. Toggle animation frame
        current_frame = current_frame ^ 1;
        
        // 4. Draw new sprite
        if (current_frame == 0) draw_sprite(x, y, frame1, 1);
        if (current_frame == 1) draw_sprite(x, y, frame2, 1);
        
        // 5. Short delay
        for (int d = 0; d < 300; d = d + 1) {
            asm { NOP }
        }
    }
    return 0;
}