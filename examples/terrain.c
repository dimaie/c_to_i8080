// terrain.c
// Procedural 3D wireframe scrolling terrain generator

// ---------------------------------------------------------
// Math & RNG
// ---------------------------------------------------------

int seed = 12345;
int rand() {
    // Safe 8-bit pseudo-random generator
    seed = multiply(seed & 255, 137) + 1;
    if (seed < 0) seed = -seed; 
    return seed;
}

int abs(int v) {
    if (v < 0) return -v;
    return v;
}

// ---------------------------------------------------------
// Terrain Generator
// ---------------------------------------------------------

// Generates a new back row of mountains by mutating the row in front of it
void generate_horizon(int *grid, int dest_row, int src_row, int width) {
    // 1. Inherit and Perturb (Random Walk)
    for (int i = 0; i < width; i = i + 1) {
        int delta = (rand() % 21) - 10; // Random bump between -10 and +10
        grid[dest_row + i] = grid[src_row + i] + delta;
        
        // Clamp heights to keep mountains on screen
        if (grid[dest_row + i] > 60) grid[dest_row + i] = 60;
        if (grid[dest_row + i] < -60) grid[dest_row + i] = -60;
    }
    
    // 2. Horizontal Smoothing (Fast 1D Box Blur)
    int left = grid[dest_row];
    for (reg int i = 1; i < width - 1; i = i + 1) {
        int center = grid[dest_row + i];
        int right = grid[dest_row + i + 1];
        // Average: (left + center*2 + right) / 4
        grid[dest_row + i] = (left + (center << 1) + right) >> 2;
        left = center;
    }
}

// ---------------------------------------------------------
// Graphics
// ---------------------------------------------------------

// Draw line safely, clipping to screen bounds to prevent ROM memory corruption
void draw_safe_line(int x0, int y0, int x1, int y1, int color) {
    // Fast path: fully on screen (hardware/ROM accelerated)
    if (x0 >= 0 && x0 <= 255 && y0 >= 0 && y0 <= 239 && 
        x1 >= 0 && x1 <= 255 && y1 >= 0 && y1 <= 239) {
        draw_line(x0, y0, x1, y1, color);
        return;
    }
    
    // Trivial rejection: both points off screen on the same side
    if (x0 < 0 && x1 < 0) return;
    if (x0 > 255 && x1 > 255) return;
    if (y0 < 0 && y1 < 0) return;
    if (y0 > 239 && y1 > 239) return;

    // Software fallback for crossing lines
    int dx = abs(x1 - x0);
    int sx = 1; if (x0 >= x1) sx = -1;
    int dy = -abs(y1 - y0);
    int sy = 1; if (y0 >= y1) sy = -1;
    int err = dx + dy;
    int e2;

    while (1) {
        // Only plot pixels that fall within the screen boundaries
        if (x0 >= 0 && x0 <= 255 && y0 >= 0 && y0 <= 239) {
            put_pixel_xy(x0, y0, color);
        }
        if (x0 == x1 && y0 == y1) break;
        
        e2 = err << 1;
        if (e2 >= dy) { err = err + dy; x0 = x0 + sx; }
        if (e2 <= dx) { err = err + dx; y0 = y0 + sy; }
    }
}

// Connects the projected 2D coordinates into a grid mesh
int draw_grid(int color, int *px, int *py) {
    // Draw horizontal connecting lines
    for (int z = 0; z < 12; z = z + 1) {
        int row = multiply(z, 10);
        for (reg int x = 0; x < 9; x = x + 1) {
            int idx = row + x;
            draw_safe_line(px[idx], py[idx], px[idx + 1], py[idx + 1], color);
        }
    }
    // Draw vertical connecting lines
    for (int z = 0; z < 11; z = z + 1) {
        int row = multiply(z, 10);
        for (reg int x = 0; x < 10; x = x + 1) {
            int idx = row + x;
            draw_safe_line(px[idx], py[idx], px[idx + 10], py[idx + 10], color);
        }
    }
    return 0;
}

// ---------------------------------------------------------
// Main Loop
// ---------------------------------------------------------

int main() {
    // 10x12 Grid = 120 Vertices
    int heights[120];
    int px[120];
    int py[120];
    int old_px[120];
    int old_py[120];
    
    clear_screen(0);
    set_cursor_style(0);
    puts_at(1, 1, "WARP TERRAIN");
    
    // Flatten terrain initially
    for (int i = 0; i < 120; i = i + 1) {
        heights[i] = 0;
        old_px[i] = 0;
        old_py[i] = 0;
    }
    
    int z_offset = 30; // Grid tile size
    int fov = 100;     // Reduced FOV to prevent signed 16-bit math overflows
    int cam_x = 0;     // Camera left/right position
    int cam_y = 50;    // Camera altitude
    int speed = 5;     // Scroll speed
    
    while(1) {
        // Interactive Speed Controls
        int key = check_key();
        if (key == 119 || key == 87) speed = speed + 1; // W key
        if (key == 115 || key == 83) speed = speed - 1; // S key
        if (key == 97  || key == 65) cam_x = cam_x - 5; // A key (Left)
        if (key == 100 || key == 68) cam_x = cam_x + 5; // D key (Right)
        if (key == 113 || key == 81) cam_y = cam_y + 2; // Q key (Up)
        if (key == 101 || key == 69) cam_y = cam_y - 2; // E key (Down)
        
        if (speed > 15) speed = 15;
        if (speed < 1) speed = 1;
        if (cam_x > 100) cam_x = 100;
        if (cam_x < -100) cam_x = -100;
        if (cam_y > 100) cam_y = 100;
        if (cam_y < -50) cam_y = -50;

        wait_vblank();
        
        // 1. Erase old grid (Draw Black)
        draw_grid(0, old_px, old_py);
        
        // 2. Project current grid (3D to 2D)
        int idx = 0;
        for (int z = 0; z < 12; z = z + 1) {
            int z_world = multiply(z, 30) + z_offset; 
            if (z_world < 20) z_world = 20; // Prevent div by zero & limit near perspective stretch
            
            for (reg int x = 0; x < 10; x = x + 1) {
                int x_world = multiply(x - 4, 30) - 15 - cam_x; // Safe math with steering
                int y_world = cam_y - heights[idx]; // Flip Y-axis so ground is at the bottom
                
                int sx = divide(multiply(x_world, fov), z_world) + 128;
                int sy = divide(multiply(y_world, fov), z_world) + 120;
                
                px[idx] = sx; py[idx] = sy;
                old_px[idx] = sx; old_py[idx] = sy;
                idx = idx + 1;
            }
        }
        
        // 3. Draw new grid (Draw White)
        draw_grid(1, px, py);
        
        // 4. Advance treadmill array
        z_offset = z_offset - speed; 
        if (z_offset <= 0) {
            z_offset = 30;
            
            // Shift all rows forward by 1
            for (int z = 0; z < 11; z = z + 1) {
                int dest_row = multiply(z, 10);
                int src_row = multiply(z + 1, 10);
                for (reg int x = 0; x < 10; x = x + 1) {
                    heights[dest_row + x] = heights[src_row + x];
                }
            }
            
            // Generate new mountains at the back
            generate_horizon(heights, 110, 100, 10);
        }
    }
    
    return 0;
}