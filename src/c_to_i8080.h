#ifndef C_TO_I8080_H
#define C_TO_I8080_H

// Visual Studio compatibility
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#define strdup _strdup
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdarg.h>
#include <time.h>

// Token types
typedef enum {
    TOK_EOF,
    TOK_INT, TOK_SHORT, TOK_CHAR, TOK_VOID, TOK_RETURN, TOK_IF, TOK_ELSE, TOK_WHILE, TOK_FOR, TOK_DO, TOK_ASM, TOK_BREAK, TOK_REG, TOK_STATIC, TOK_CONTINUE,
    TOK_IDENT, TOK_NUMBER, TOK_STRING,
    TOK_LPAREN, TOK_RPAREN, TOK_LBRACE, TOK_RBRACE, TOK_LBRACKET, TOK_RBRACKET,
    TOK_SEMICOLON, TOK_COMMA,
    TOK_ASSIGN, TOK_PLUS_ASSIGN, TOK_MINUS_ASSIGN, TOK_STAR_ASSIGN, TOK_SLASH_ASSIGN,
    TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_PERCENT, TOK_INC, TOK_DEC,
    TOK_EQ, TOK_NE, TOK_LT, TOK_LE, TOK_GT, TOK_GE,
    TOK_AND, TOK_OR, TOK_NOT,
    TOK_AMPERSAND, TOK_PIPE, TOK_CARET, TOK_TILDE,
    TOK_SHL, TOK_SHR
} TokenType;

typedef struct {
    TokenType type;
    char *value;
    int line;
} Token;

// AST Node types
typedef enum {
    AST_PROGRAM,
    AST_FUNCTION,
    AST_VARDECL,
    AST_BLOCK,
    AST_RETURN,
    AST_IF,
    AST_WHILE,
    AST_FOR,
    AST_DO_WHILE,
    AST_EXPR_STMT,
    AST_BINARY_OP,
    AST_COMPOUND_ASSIGN,
    AST_PRE_INC,
    AST_PRE_DEC,
    AST_POST_INC,
    AST_POST_DEC,
    AST_UNARY_OP,
    AST_ASSIGN,
    AST_CALL,
    AST_IDENT,
    AST_NUMBER,
    AST_STRING,
    AST_ARRAY_ACCESS, // arr[idx]
    AST_DEREF,      // *ptr (dereference)
    AST_ADDROF,     // &var (address-of)
    AST_ASM,        // asm { ... } (inline assembly)
    AST_BREAK,      // break;
    AST_CONTINUE    // continue;
} ASTNodeType;

typedef struct ASTNode {
    ASTNodeType type;
    char *value;
    int datatype;     // 1 for 8-bit, 2 for 16-bit
    int array_size;   // Size of the array (0 if not an array)
    bool is_reg;      // True if variable is declared with the 'reg' keyword
    bool is_static;   // True if variable is declared with the 'static' keyword
    bool is_used;     // Track if function is used (Dead Code Elimination)
    struct ASTNode **children;
    int child_count;
    int child_capacity;
} ASTNode;

// Symbol table
typedef struct Symbol {
    char *name;
    int address;  // Memory address for variable
    bool is_global;
    bool is_pointer;  // Whether this variable is a pointer type
    bool is_16bit;    // True for int/pointers, False for short/char
    int array_size;   // Size of the array (0 if not an array)
    bool is_reg;      // True if variable is allocated to a hardware register
    bool is_static;   // True if variable is statically allocated
    struct Symbol *next;
} Symbol;

typedef struct {
    Symbol *symbols;
    int next_address;  // Next available memory address for allocation
} SymbolTable;

// Compiler context
typedef struct {
    Token *tokens;
    int token_count;
    int current_token;
    SymbolTable *symtab;
    FILE *output;
    int label_counter;
    char *current_function;
    bool uses_mul;  // Track if multiplication is used
    bool uses_div;  // Track if division is used
    bool uses_mod;  // Track if modulo is used
    bool use_frame_pointer; // Track which memory model to use
    int org_address; // Starting memory address
    int stack_address; // Top of the stack address
    int current_break_label; // Label to jump to on 'break'
    int current_continue_label; // Label to jump to on 'continue'
    bool uses_bc; // Track if the BC register is used by a local variable
} Compiler;

// Lexer functions
Token* tokenize(const char *source, int *token_count);
void free_tokens(Token *tokens, int count);

// Parser functions
ASTNode* parse(Token *tokens, int token_count);
void free_ast(ASTNode *node);

// Code generator functions
void compile_to_i8080(ASTNode *ast, FILE *output, bool use_frame_pointer, int org_address, int stack_address);

// Utility functions
ASTNode* create_node(ASTNodeType type, const char *value);
void add_child(ASTNode *parent, ASTNode *child);
Symbol* add_symbol(SymbolTable *symtab, const char *name, bool is_global, bool is_pointer, bool is_16bit, int array_size, bool is_reg, bool is_static);
Symbol* find_symbol(SymbolTable *symtab, const char *name);

#endif // C_TO_I8080_H
