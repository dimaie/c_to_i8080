// test_sintesizer.c
// Hardware Background Sequencer Demonstration

// Hardware Registers for Audio Synthesizer (Using decimal addresses)
char *SYNTH_WAVETABLE  = 24576; // 0x6000
char *SYNTH_SEQ_RAM    = 25600; // 0x6400
char *SYNTH_PITCH_LOW  = 49168; // 0xC010
char *SYNTH_PITCH_HIGH = 49169; // 0xC011
char *SYNTH_VOLUME     = 49170; // 0xC012
char *SYNTH_CTRL       = 49171; // 0xC013
char *SYNTH_SEQ_EN     = 49172; // 0xC014

// A crude but effective software delay for holding the note
void delay(int ms) {
    for (int i = 0; i < ms; i++) {
        for (reg int j = 0; j < 100; j++) {
            // busy wait
        }
    }
}

// Generates a smooth 1KB triangle wave mathematically and loads it into memory
void load_triangle_wave() {
    for (int i = 0; i < 1024; i++) {
        if (i < 512) {
            SYNTH_WAVETABLE[i] = i >> 1;                 // Ramp up: 0 to 255
        } else {
            SYNTH_WAVETABLE[i] = 255 - ((i - 512) >> 1); // Ramp down: 255 to 0
        }
    }
}

// Populates the hardware sequencer RAM with a background melody
// Format per step: [Pitch Low, Pitch High, Volume, Duration in 60Hz frames]
void load_background_melody() {
    // Tetris Theme (Korobeiniki)
    // 58 notes/rests * 4 bytes + 4 byte loop marker = 236 bytes!
    // Nearly fills the entire 256-byte sequencer RAM.
    static char melody[] = { 
        // Part A
        3, 2, 120, 20,   // E5 (Quarter)
        130, 1, 120, 10, // B4 (Eighth)
        153, 1, 120, 10, // C5 (Eighth)
        203, 1, 120, 20, // D5 (Quarter)
        153, 1, 120, 10, // C5 (Eighth)
        130, 1, 120, 10, // B4 (Eighth)
        88, 1, 120, 20,  // A4 (Quarter)
        88, 1, 120, 10,  // A4 (Eighth)
        153, 1, 120, 10, // C5 (Eighth)
        3, 2, 120, 20,   // E5 (Quarter)
        203, 1, 120, 10, // D5 (Eighth)
        153, 1, 120, 10, // C5 (Eighth)
        130, 1, 120, 30, // B4 (Dotted Quarter)
        153, 1, 120, 10, // C5 (Eighth)
        203, 1, 120, 20, // D5 (Quarter)
        3, 2, 120, 20,   // E5 (Quarter)
        153, 1, 120, 20, // C5 (Quarter)
        88, 1, 120, 20,  // A4 (Quarter)
        88, 1, 120, 20,  // A4 (Quarter)
        0, 0, 0, 15,     // Rest
        
        // Part B
        203, 1, 120, 20, // D5
        34, 2, 120, 10,  // F5
        176, 2, 120, 20, // A5
        100, 2, 120, 10, // G5
        34, 2, 120, 10,  // F5
        3, 2, 120, 30,   // E5
        153, 1, 120, 10, // C5
        3, 2, 120, 20,   // E5
        203, 1, 120, 10, // D5
        153, 1, 120, 10, // C5
        130, 1, 120, 20, // B4
        130, 1, 120, 10, // B4
        153, 1, 120, 10, // C5
        203, 1, 120, 20, // D5
        3, 2, 120, 20,   // E5
        153, 1, 120, 20, // C5
        88, 1, 120, 20,  // A4
        88, 1, 120, 20,  // A4
        0, 0, 0, 15,     // Rest
        
        // Part B (Repeat)
        203, 1, 120, 20, // D5
        34, 2, 120, 10,  // F5
        176, 2, 120, 20, // A5
        100, 2, 120, 10, // G5
        34, 2, 120, 10,  // F5
        3, 2, 120, 30,   // E5
        153, 1, 120, 10, // C5
        3, 2, 120, 20,   // E5
        203, 1, 120, 10, // D5
        153, 1, 120, 10, // C5
        130, 1, 120, 20, // B4
        130, 1, 120, 10, // B4
        153, 1, 120, 10, // C5
        203, 1, 120, 20, // D5
        3, 2, 120, 20,   // E5
        153, 1, 120, 20, // C5
        88, 1, 120, 20,  // A4
        88, 1, 120, 20,  // A4
        0, 0, 0, 40,     // Longer Rest

        0, 0, 0, 0 // LOOP MARKER
    };

    // Copy the entire melody into the hardware sequencer's RAM
    for (int i = 0; i < 236; i++) {
        SYNTH_SEQ_RAM[i] = melody[i];
    }
}

int main() {
    print_string("Audio Synthesizer Test\r\n");
    print_string("Generating Triangle Wavetable...\r\n");
    
    load_triangle_wave();
    
    print_string("Loading melody to Sequencer RAM...\r\n");
    load_background_melody();
    
    // Transfer control from the CPU to the Hardware Sequencer!
    *SYNTH_SEQ_EN = 1;
    
    print_string("Music is now playing entirely in hardware!\r\n");
    print_string("The CPU is completely free.\r\n");
    print_string("Press 'Q' to quit.\r\n");

    int dots = 0;
    while (1) {
        int key = check_key(); // Non-blocking check for PS/2 Scan Code
        
        // PS/2 Scan Code for 'Q' is 21 (0x15)
        if (key == 21) break;
        
        // Prove the CPU is not blocked by doing arbitrary work
        delay(5);
        dots++;
        if ((dots & 127) == 0) print_char('.'); // Use fast bitwise check for modulo
    }
    
    *SYNTH_SEQ_EN = 0; // Stop the background music
    print_string("\r\nTest ended.\r\n");
    return 0;
}