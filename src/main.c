#include "c_to_i8080.h"
#include <stdarg.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [options] <input1.c> [input2.c ...] [-o output.asm]\n", argv[0]);
        fprintf(stderr, "Options:\n");
        fprintf(stderr, "  -mstack    Use stack-based memory allocation (frame pointer)\n");
        fprintf(stderr, "  -mdirect   Use direct memory addressing with shadow stack (default)\n");
        fprintf(stderr, "  -org <addr> Set origin address (default: 0x0100)\n");
        fprintf(stderr, "  -stack <addr> Set stack top address (default: 0xFFFF)\n");
        return 1;
    }

    bool use_frame_pointer = false;
    int org_address = 0x0100;
    int stack_address = 0xFFFF;
    int input_count = 0;
    char *input_files[100];
    const char *output_file = "output.asm";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-mstack") == 0) {
            use_frame_pointer = true;
        } else if (strcmp(argv[i], "-mdirect") == 0) {
            use_frame_pointer = false;
        } else if (strcmp(argv[i], "-org") == 0 && i + 1 < argc) {
            org_address = (int)strtol(argv[++i], NULL, 0);
        } else if (strcmp(argv[i], "-stack") == 0 && i + 1 < argc) {
            stack_address = (int)strtol(argv[++i], NULL, 0);
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Error: Unknown or malformed option '%s'\n", argv[i]);
            return 1;
        } else {
            if (input_count < 100) input_files[input_count++] = argv[i];
        }
    }

    if (input_count == 0) {
        fprintf(stderr, "Error: No input files specified\n");
        return 1;
    }

    // Calculate total size of all input files
    long total_size = 0;
    for (int i = 0; i < input_count; i++) {
        FILE *f = fopen(input_files[i], "rb");
        if (!f) {
            fprintf(stderr, "Error: Cannot open file %s\n", input_files[i]);
            return 1;
        }
        fseek(f, 0, SEEK_END);
        total_size += ftell(f) + 1; // +1 for newline separation
        fclose(f);
    }

    // Concatenate all input files
    char *source = malloc(total_size + 1);
    char *ptr = source;
    for (int i = 0; i < input_count; i++) {
        FILE *f = fopen(input_files[i], "rb");
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);
        fread(ptr, 1, size, f);
        ptr += size;
        *ptr++ = '\n'; // Isolate files with a newline
        fclose(f);
    }
    *ptr = '\0';

    printf("Compiling %d file(s) to i8080 assembly...\n", input_count);

    // Tokenize
    int token_count;
    Token *tokens = tokenize(source, &token_count);
    printf("Tokenization complete: %d tokens\n", token_count);

    // Parse
    ASTNode *ast = parse(tokens, token_count);
    printf("Parsing complete\n");

    // Generate code
    FILE *out = fopen(output_file, "w");
    if (!out) {
        fprintf(stderr, "Error: Cannot create output file %s\n", output_file);
        return 1;
    }

    compile_to_i8080(ast, out, use_frame_pointer, org_address, stack_address);
    fclose(out);

    printf("Code generation complete: %s\n", output_file);

    // Cleanup
    free_tokens(tokens, token_count);
    free_ast(ast);
    free(source);

    return 0;
}
