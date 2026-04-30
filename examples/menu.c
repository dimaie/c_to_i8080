// menu.c
// Interactive menu demonstrating inverse text and function pointers

// ---------------------------------------------------------
// Global State
// ---------------------------------------------------------
char *items[10];
int types[10];   // 0 = Action Callback, 1 = Submenu Transition
int targets[10]; // Stores either a Function Pointer or a Menu ID
int num_items = 0;

// Helper to load a menu item into the current screen state
void set_item(int idx, char *name, int type, int target) {
    items[idx] = name;
    types[idx] = type;
    targets[idx] = target;
}

// ---------------------------------------------------------
// Action Callbacks
// ---------------------------------------------------------
int action_hello() {
    clear_screen(0);
    puts_at(10, 10, "Hello, World!");
    puts_at(10, 12, "Inverse mode makes menus pop!");
    puts_at(10, 14, "Press SPACE to return...");
    read_key();
    return 0;
}

int action_about() {
    clear_screen(0);
    puts_at(10, 10, "SAP-3 OS v1.0");
    puts_at(10, 11, "Written in Custom C");
    puts_at(10, 13, "Press SPACE to return...");
    read_key();
    return 0;
}

int action_sound() {
    clear_screen(0);
    set_inverse(1);
    puts_at(10, 10, " * BEEP BOOP * ");
    set_inverse(0);
    puts_at(10, 12, "Press SPACE to return...");
    read_key();
    return 0;
}

int action_calibrate() {
    clear_screen(0);
    puts_at(10, 10, "Calibrating...");
    for(int i = 0; i < 20; i = i + 1) {
        for(reg int j = 0; j < 500; j = j + 1) {
            // Hardware-accelerated empty loop delay
        }
        puts_at(10 + i, 11, ".");
    }
    puts_at(10, 13, "Done. Press SPACE to return...");
    read_key();
    return 0;
}

// ---------------------------------------------------------
// Menu Loading & Rendering
// ---------------------------------------------------------
void load_menu(int menu_id) {
    if (menu_id == 0) {
        num_items = 3;
        set_item(0, "1. Say Hello", 0, action_hello);
        set_item(1, "2. Settings", 1, 1); // 1 = Target Menu ID
        set_item(2, "3. About OS", 0, action_about);
    } else if (menu_id == 1) {
        num_items = 3;
        set_item(0, "1. Test Sound", 0, action_sound);
        set_item(1, "2. Calibrate", 0, action_calibrate);
        set_item(2, "3. Back to Main", 1, 0); // 0 = Target Menu ID
    }
}

void draw_menu(int menu_id, int selected) {
    clear_screen(0);
    
    // Draw Header
    set_inverse(1);
    if (menu_id == 0) puts_at(20, 2, "   MAIN MENU   ");
    if (menu_id == 1) puts_at(20, 2, "   SETTINGS    ");
    set_inverse(0);

    // Draw Items
    for (int i = 0; i < num_items; i = i + 1) {
        if (i == selected) {
            set_inverse(1);
            puts_at(18, 6 + i * 2, "->");
        } else {
            set_inverse(0);
            puts_at(18, 6 + i * 2, "  ");
        }
        
        puts_at(21, 6 + i * 2, items[i]);
        set_inverse(0); // Ensure it's off after each item
    }

    // Draw Footer
    puts_at(15, 25, "Controls: W/S=Move, SPACE=Select");
}

// ---------------------------------------------------------
// Main Event Loop
// ---------------------------------------------------------
int main() {
    int current_menu = 0;
    int selected = 0;

    load_menu(current_menu);
    draw_menu(current_menu, selected);

    while (1) {
        int key = read_key(); // Blocks until key is pressed

        if (key == 119 || key == 87) { // 'w' or 'W'
            if (selected > 0) {
                selected = selected - 1;
            } else {
                selected = num_items - 1; // wrap around
            }
            draw_menu(current_menu, selected);
        }
        
        if (key == 115 || key == 83) { // 's' or 'S'
            if (selected < num_items - 1) {
                selected = selected + 1;
            } else {
                selected = 0; // wrap around
            }
            draw_menu(current_menu, selected);
        }
        
        if (key == 32 || key == 13) { // SPACE or ENTER
            if (types[selected] == 0) {
                // It's an action! 
                // Cast the untyped target value into a C function pointer and invoke it dynamically.
                int (*func)() = targets[selected];
                func();
                
                // Redraw the menu after the action returns
                draw_menu(current_menu, selected);
            } else if (types[selected] == 1) {
                // It's a submenu!
                // Target is the menu ID to load.
                current_menu = targets[selected];
                selected = 0;
                load_menu(current_menu);
                draw_menu(current_menu, selected);
            }
        }
    }
    
    return 0;
}