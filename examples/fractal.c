int main() {
    int max_iter = 15;
    int needs_redraw = 1;
    
    while (1) {
        if (needs_redraw) {
            clear_screen(0);
            puts_at(1, 1, "MANDELBROT SET");
            puts_at(1, 2, "RENDERING...  ");
            
            // Leverage Y-axis symmetry: only calculate top half (120 lines)
            for (int py = 0; py < 120; py = py + 1) {
                int y0 = (py >> 1) - 60; 
                
                for (int px = 0; px < 256; px = px + 1) {
                    int x0 = (px >> 1) - 80; 
                    
                    int x = 0;
                    int y = 0;
                    int iter = 0;
                    int abs_x;
                    int abs_y;
                    
                    while (iter < max_iter) {
                        abs_x = x; if (abs_x < 0) abs_x = -abs_x;
                        abs_y = y; if (abs_y < 0) abs_y = -abs_y;
                        
                        if (abs_x > 150 || abs_y > 150) {
                            break;
                        }
                        
                        // Massive speedup: Use hardware mul8 directly on pre-calculated abs values
                        int x2 = mul8(abs_x, abs_x) >> 5;
                        int y2 = mul8(abs_y, abs_y) >> 5;
                        
                        // Escape if distance squared > 4.0 (128 in Q5)
                        if (x2 + y2 > 128) {
                            break;
                        }
                        
                        int xy = mul8(abs_x, abs_y) >> 5;
                        if ((x ^ y) < 0) xy = -xy; // Rapid sign restoration!
                        y = xy + xy + y0;
                        x = x2 - y2 + x0;
                        
                        iter = iter + 1;
                    }
                    
                    // Dither pixel shading (Core reverted to black for massive speedup)
                    if (iter == max_iter) {
                        // Core (Black) - Do nothing, saving thousands of pixel drawing calls!
                    } else if (iter > 5) {
                        if (((px + py) & 1) == 0) {
                            put_pixel_xy(px, py, 1);
                            put_pixel_xy(px, 239 - py, 1);
                        }
                    } else if (iter > 2) {
                        if ((px & 1) == 0) {
                            if ((py & 1) == 0) put_pixel_xy(px, py, 1);
                            if (((239 - py) & 1) == 0) put_pixel_xy(px, 239 - py, 1);
                        }
                    }
                }
            }
            
            puts_at(1, 2, "DONE. UP/DWN  ");
            needs_redraw = 0;
        }
        
        // Check keyboard to interactively increase/decrease detail
        int key = check_key();
        if (key == 117 || key == 29) { // Up Arrow or W
            max_iter = max_iter + 5;
            needs_redraw = 1;
        }
        if (key == 114 || key == 27) { // Down Arrow or S
            max_iter = max_iter - 5;
            if (max_iter < 5) max_iter = 5;
            needs_redraw = 1;
        }
        
        wait_vblank();
    }
    return 0;
}