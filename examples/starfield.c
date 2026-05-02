// A simple 16-bit Pseudo-Random Number Generator (LCG)
int seed = 12345;
int rand() {
    // seed = (seed * 137 + 1)
    seed = seed * 137 + 1;
    if (seed < 0) {
        seed = -seed; // Keep it positive for easy modulo
    }
    return seed;
}

int main() {
    int num_stars = 50;
    
    // 3D coordinates
    int x[50];
    int y[50];
    int z[50];
    
    // 2D Screen coordinates for erasing
    int old_px[50];
    int old_py[50];
    
    clear_screen(0);
    set_cursor_style(0); // Hide cursor
    puts_at(1, 1, "WARP SPEED ACTIVATED");
    
    // 1. Initialize Stars with random positions
    for (int i = 0; i < num_stars; i = i + 1) {
        x[i] = (rand() % 400) - 200; // -200 to +200
        y[i] = (rand() % 400) - 200; // -200 to +200
        z[i] = (rand() % 200) + 1;   // 1 to 200 depth
        
        old_px[i] = -1; // Initialize off-screen
        old_py[i] = -1;
    }
    
    while (1) {
        wait_vblank(); // Sync to 60 FPS
        
        // Fast array traversal using pointers
        int *px = x;
        int *py = y;
        int *pz = z;
        int *p_oldx = old_px;
        int *p_oldy = old_py;
        
        for (reg int i = 0; i < num_stars; i = i + 1) {
            // 2. Erase old star
            if (*p_oldx >= 0 && *p_oldx < 256 && *p_oldy >= 0 && *p_oldy < 240) {
                put_pixel_xy(*p_oldx, *p_oldy, 0); // Color 0 = Black
            }
            
            // 3. Move star closer (decrease Z depth)
            *pz = *pz - 4;
            
            // 4. Reset star if it passes the camera (Z <= 0)
            if (*pz <= 0) {
                *px = (rand() % 400) - 200;
                *py = (rand() % 400) - 200;
                *pz = 200; // Send back to the horizon
            }
            
            // 5. Project 3D to 2D screen coordinates
            // screen_x = (x * FOV) / z + screen_center_x
            int sx = divide(multiply(*px, 64), *pz) + 128;
            int sy = divide(multiply(*py, 64), *pz) + 120;
            
            // 6. Draw new star and save coordinates
            if (sx >= 0 && sx < 256 && sy >= 0 && sy < 240) {
                put_pixel_xy(sx, sy, 1); // Color 1 = White
            }
            *p_oldx = sx;
            *p_oldy = sy;
            
            // Advance pointers to next star
            px = px + 1; py = py + 1; pz = pz + 1; p_oldx = p_oldx + 1; p_oldy = p_oldy + 1;
        }
    }
    return 0;
}