// Clears the 7680 bytes of Graphics RAM using native pointers
int clear_gfx_ram() {
    int *vram = 16384; // Base address 0x4000
    for (int i = 0; i < 3840; i = i + 1) {
        *vram = 0;
        vram = vram + 2; 
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
                put_pixel_xy(x + col, y + row, color);
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
    
    clear_screen(0);
    puts_at(1, 1, "VSYNC: ON ");
    
    int x = 0;
    int y = 110;
    int current_frame = 0;
    int use_vsync = 1;
    int toggle_cooldown = 0;
    
    while (1) {
        int key = check_key();
        
        if (toggle_cooldown > 0) {
            toggle_cooldown = toggle_cooldown - 1;
        } else if (key != 0) {
            // Toggle VSYNC and ignore further key codes (like PS/2 break codes) for 30 frames
            use_vsync = use_vsync ^ 1;
            if (use_vsync) {
                puts_at(1, 1, "VSYNC: ON ");
            } else {
                puts_at(1, 1, "VSYNC: OFF");
            }
            toggle_cooldown = 30; 
        }

        // Wait for VBLANK to ensure smooth, tear-free rendering at 60 FPS
        if (use_vsync) wait_vblank();
        
        // 1. Erase old sprite
        if (current_frame == 0) draw_sprite(x, y, frame1, 0);
        if (current_frame == 1) draw_sprite(x, y, frame2, 0);
        
        // 2. Update position
        x = x + 6;
        if (x > 256) x = -16;
        
        // 3. Toggle animation frame
        current_frame = current_frame ^ 1;
        
        // 4. Draw new sprite
        if (current_frame == 0) draw_sprite(x, y, frame1, 1);
        if (current_frame == 1) draw_sprite(x, y, frame2, 1);
    }
    return 0;
}