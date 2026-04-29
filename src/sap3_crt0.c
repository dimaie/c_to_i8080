// sap3_crt0.c - SAP-3 C Runtime and ROM API Library

// 1. Global Assembly Block
// Overrides the compiler's default origin and stack configurations.
asm {
    ORG 2100H        ; Target the SAP-3 Program RAM (0x2000 - 0x3FFF)
    LXI SP, 3FFFH    ; Initialize Stack Pointer to the very top of Program RAM
    CALL main        ; Start the C program
    HLT              ; Halt the processor when main() returns
}

// 2. ROM API Wrappers
// These use a "Tail-Call" optimization to JMP directly to the ROM.
// Because the C compiler pushes a shadow-stack backup for every parameter
// during the prologue, we must POP them off the hardware stack before jumping!

void print_char(int c) {
    asm { 
        POP H       ; Discard 'c' shadow backup
        JMP 0010H 
    }
}

int read_key() {
    asm { JMP 0013H } // 0 parameters = 0 shadow backups to discard!
}

void clear_screen(int layer) {
    asm { 
        POP H       ; Discard 'layer' shadow backup
        JMP 0016H 
    }
}

void print_string(char* str) {
    asm { 
        POP H       ; Discard 'str' shadow backup
        JMP 0019H 
    }
}

void set_cursor_xy(int x, int y) {
    asm { 
        POP H       ; Discard 'y' shadow backup
        POP H       ; Discard 'x' shadow backup
        JMP 001CH 
    }
}

int get_cursor_xy() {
    asm { JMP 001FH }
}

int read_char() {
    asm { JMP 0022H }
}

void set_cursor_style(int style) {
    asm {
        POP H       ; Discard 'style' shadow backup
        JMP 0025H
    }
}

int read_pixel_xy(int x, int y) {
    asm { 
        POP H       ; Discard 'y' shadow backup
        POP H       ; Discard 'x' shadow backup
        JMP 0028H 
    }
}

int check_key() {
    asm { JMP 002BH } // 0 parameters = 0 shadow backups to discard!
}

// Note: Signature matched to ROM API (int x0, int y0, int x1, int y1, int color)
void draw_line(int x0, int y0, int x1, int y1, int color) {
    asm {
        POP H       ; Discard 'color' shadow backup
        POP H       ; Discard 'y1' shadow backup
        POP H       ; Discard 'x1' shadow backup
        POP H       ; Discard 'y0' shadow backup
        POP H       ; Discard 'x0' shadow backup
        JMP 002EH
    }
}

// Note: Signature matched to ROM API (int x, int y, int color)
void put_pixel_xy(int x, int y, int color) {
    asm {
        POP H       ; Discard 'color' shadow backup
        POP H       ; Discard 'y' shadow backup
        POP H       ; Discard 'x' shadow backup
        JMP 0031H
    }
}

// Blocks CPU execution until the VGA beam enters the Vertical Blanking period.
int wait_vblank() {
    asm {
    WAIT_VBLANK_LOW:
        IN 09H
        ANI 01H
        JNZ WAIT_VBLANK_LOW   ; Wait until VBLANK is 0 (actively drawing screen)
    WAIT_VBLANK_HIGH:
        IN 09H
        ANI 01H
        JZ WAIT_VBLANK_HIGH   ; Wait until VBLANK is 1 (drawing finished, beam resetting)
    }
    return 0;
}

// Writes a null-terminated string to the Text RAM using the ROM API
void puts_at(int x, int y, char *str) {
    set_cursor_xy(x, y);
    print_string(str);
}