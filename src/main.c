#include "c_to_i8080.h"
#include <stdarg.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [options] <input.c> [output.asm]\n", argv[0]);
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
    const char *input_file = NULL;
    const char *output_file = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-mstack") == 0) {
            use_frame_pointer = true;
        } else if (strcmp(argv[i], "-mdirect") == 0) {
            use_frame_pointer = false;
        } else if (strcmp(argv[i], "-org") == 0 && i + 1 < argc) {
            org_address = (int)strtol(argv[++i], NULL, 0);
        } else if (strcmp(argv[i], "-stack") == 0 && i + 1 < argc) {
            stack_address = (int)strtol(argv[++i], NULL, 0);
        } else if (input_file == NULL) {
            if (argv[i][0] == '-') {
                fprintf(stderr, "Error: Unknown or malformed option '%s'\n", argv[i]);
                return 1;
            }
            input_file = argv[i];
        } else if (output_file == NULL) {
            output_file = argv[i];
        } else {
            fprintf(stderr, "Warning: Ignoring extra argument '%s'\n", argv[i]);
        }
    }

    if (input_file == NULL) {
        fprintf(stderr, "Error: No input file specified\n");
        return 1;
    }
    
    if (output_file == NULL) {
        output_file = "output.asm";
    }

     // Read input file
    FILE *f = fopen(input_file, "rb");
    if (!f) {
        fprintf(stderr, "Error: Cannot open file %s\n", input_file);
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *source = malloc(size + 1);
    size_t bytes_read = fread(source, 1, size, f);
    source[bytes_read] = '\0';
    fclose(f);

    printf("Compiling %s to i8080 assembly...\n", input_file);

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
