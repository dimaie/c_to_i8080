#include "c_to_i8080.h"
#include <stdarg.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input.c> [output.asm]\n", argv[0]);
        return 1;
    }

    const char *input_file = argv[1];
    const char *output_file = argc >= 3 ? argv[2] : "output.asm";

     // Read input file
    FILE *f = fopen(input_file, "r");
    if (!f) {
        fprintf(stderr, "Error: Cannot open file %s\n", input_file);
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *source = malloc(size + 1);
    fread(source, 1, size, f);
    source[size] = '\0';
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

    compile_to_i8080(ast, out);
    fclose(out);

    printf("Code generation complete: %s\n", output_file);

    // Cleanup
    free_tokens(tokens, token_count);
    free_ast(ast);
    free(source);

    return 0;
}
