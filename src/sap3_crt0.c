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

// Note: Signature matched to ROM API (int c, int x, int y)
void put_char_xy(int c, int x, int y) {
    asm { 
        POP H       ; Discard 'y' shadow backup
        POP H       ; Discard 'x' shadow backup
        POP H       ; Discard 'c' shadow backup
        JMP 0019H 
    }
}

int read_char_xy(int x, int y) {
    asm { 
        POP H       ; Discard 'y' shadow backup
        POP H       ; Discard 'x' shadow backup
        JMP 001CH 
    }
}

int read_pixel_xy(int x, int y) {
    asm { 
        POP H       ; Discard 'y' shadow backup
        POP H       ; Discard 'x' shadow backup
        JMP 001FH 
    }
}

int check_key() {
    asm { JMP 0022H } // 0 parameters = 0 shadow backups to discard!
}

// Note: Signature matched to ROM API (int x, int y, int color)
void put_pixel_xy(int x, int y, int color) {
    asm {
        POP H       ; Discard 'color' shadow backup
        POP H       ; Discard 'y' shadow backup
        POP H       ; Discard 'x' shadow backup
        JMP 0028H
    }
}

// Note: Signature matched to ROM API (int x0, int y0, int x1, int y1, int color)
void draw_line(int x0, int y0, int x1, int y1, int color) {
    asm {
        POP H       ; Discard 'color' shadow backup
        POP H       ; Discard 'y1' shadow backup
        POP H       ; Discard 'x1' shadow backup
        POP H       ; Discard 'y0' shadow backup
        POP H       ; Discard 'x0' shadow backup
        JMP 0025H
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
int puts_at(int x, int y, char *str) {
    int c = *str & 255;
    while (c) {
        put_char_xy(c, x, y);
        x = x + 1;
        str = str + 1;
        c = *str & 255;
    }
    return 0;
}

// Hardware accelerated 8-bit multiplication (a * b)
// Uses the SAP-3 custom instruction MUL B
int mul8(int a, int b) {
    int res;
    asm {
        LDA __VAR_a    ; Load lower byte of 'a'
        MOV B, A       ; Move to register B
        LDA __VAR_b    ; Load lower byte of 'b'
        MUL B          ; Hardware multiply (A * B -> HL)
        SHLD __VAR_res ; Store 16-bit result into 'res'
    }
    return res;
}