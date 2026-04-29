
char* to_alpha(int c, char* buffer) {
    int counter = 0;
    do {
        buffer[counter] = (c % 10) + 48;
        counter++;
        c /= 10;
    } while (c > 0);
    
    buffer[counter] = 0;
    
    int start = 0;
    int end = counter - 1;
    while (start < end) {
        int temp = buffer[start];
        buffer[start] = buffer[end];
        buffer[end] = temp;
        start++;
        end--;
    }
    return buffer;
}

int main() {
    clear_screen(0);
    set_cursor_style(1);
    int old_key = 0;
    while (1) {
        int key = check_key();
        
        if (key != 0 && key != old_key) {
            char buffer[6];
            to_alpha(key, buffer);
            print_string(buffer);
            print_string(" ");
            old_key = key;
        }
        for (int i = 0; i < 10000; i = i + 1) {
            asm {
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
            }
        }
    }
    return 0;
}