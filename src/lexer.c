#include "c_to_i8080.h"

#ifndef TOK_SWITCH
#define TOK_SWITCH 1000
#define TOK_CASE 1001
#define TOK_DEFAULT 1002
#define TOK_COLON 1003
#endif

static bool is_keyword(const char *str) {
    const char *keywords[] = {"int", "short", "char", "void", "return", "if", "else", "while", "for", "do", "asm", "break", "continue", "reg", "register", "static", "switch", "case", "default", "goto", "struct", "union", NULL};
    for (int i = 0; keywords[i]; i++) {
        if (strcmp(str, keywords[i]) == 0) return true;
    }
    return false;
}

static TokenType get_keyword_type(const char *str) {
    if (strcmp(str, "int") == 0) return TOK_INT;
    if (strcmp(str, "short") == 0) return TOK_SHORT;
    if (strcmp(str, "char") == 0) return TOK_CHAR;
    if (strcmp(str, "void") == 0) return TOK_VOID;
    if (strcmp(str, "return") == 0) return TOK_RETURN;
    if (strcmp(str, "if") == 0) return TOK_IF;
    if (strcmp(str, "else") == 0) return TOK_ELSE;
    if (strcmp(str, "while") == 0) return TOK_WHILE;
    if (strcmp(str, "for") == 0) return TOK_FOR;
    if (strcmp(str, "do") == 0) return TOK_DO;
    if (strcmp(str, "asm") == 0) return TOK_ASM;
    if (strcmp(str, "break") == 0) return TOK_BREAK;
    if (strcmp(str, "continue") == 0) return TOK_CONTINUE;
    if (strcmp(str, "reg") == 0) return TOK_REG;
    if (strcmp(str, "register") == 0) return TOK_REG;
    if (strcmp(str, "static") == 0) return TOK_STATIC;
    if (strcmp(str, "switch") == 0) return TOK_SWITCH;
    if (strcmp(str, "case") == 0) return TOK_CASE;
    if (strcmp(str, "default") == 0) return TOK_DEFAULT;
    if (strcmp(str, "goto") == 0) return TOK_GOTO;
    if (strcmp(str, "struct") == 0) return TOK_STRUCT;
    if (strcmp(str, "union") == 0) return TOK_UNION;
    return TOK_IDENT;
}

Token* tokenize(const char *source, int *token_count) {
    int capacity = 256;
    Token *tokens = malloc(sizeof(Token) * capacity);
    int count = 0;
    int line = 1;
    const char *p = source;

    while (*p) {
        // Skip whitespace - FIX: cast to unsigned char
        while (isspace((unsigned char)*p)) {
            if (*p == '\n') line++;
            p++;
        }
        if (!*p) break;

        // Skip comments
        if (*p == '/' && *(p + 1) == '/') {
            while (*p && *p != '\n') p++;
            continue;
        }
        if (*p == '/' && *(p + 1) == '*') {
            p += 2;
            while (*p && !(*p == '*' && *(p + 1) == '/')) {
                if (*p == '\n') line++;
                p++;
            }
            if (*p) p += 2;
            continue;
        }

        if (count >= capacity) {
            capacity *= 2;
            tokens = realloc(tokens, sizeof(Token) * capacity);
        }

        Token *tok = &tokens[count++];
        tok->line = line;

        // Identifiers and keywords - FIX: cast to unsigned char
        if (isalpha((unsigned char)*p) || *p == '_') {
            const char *start = p;
            while (isalnum((unsigned char)*p) || *p == '_') p++;
            int len = (int)(p - start);
            tok->value = malloc(len + 1);
            strncpy(tok->value, start, len);
            tok->value[len] = '\0';
            tok->type = is_keyword(tok->value) ? get_keyword_type(tok->value) : TOK_IDENT;

            // Special handling for asm keyword - capture raw assembly block
            if (tok->type == TOK_ASM) {
                // Skip whitespace
                while (isspace((unsigned char)*p)) {
                    if (*p == '\n') line++;
                    p++;
                }
                // Expect opening brace
                if (*p == '{') {
                    p++; // Skip opening brace

                    // Add the opening brace token
                    if (count >= capacity) {
                        capacity *= 2;
                        tokens = realloc(tokens, sizeof(Token) * capacity);
                    }
                    Token *brace_tok = &tokens[count++];
                    brace_tok->type = TOK_LBRACE;
                    brace_tok->value = strdup("{");
                    brace_tok->line = line;

                    // Capture raw assembly text until closing brace
                    const char *asm_start = p;
                    int brace_depth = 1;
                    while (*p && brace_depth > 0) {
                        if (*p == '{') brace_depth++;
                        else if (*p == '}') {
                            brace_depth--;
                            if (brace_depth == 0) break; // Safely stop at the exact closing brace
                        }
                        
                        if (*p == '\n') line++;
                        p++;
                    }

                    // Create a STRING token with the raw assembly code
                    if (count >= capacity) {
                        capacity *= 2;
                        tokens = realloc(tokens, sizeof(Token) * capacity);
                    }
                    Token *asm_tok = &tokens[count++];
                    int asm_len = (int)(p - asm_start);
                    asm_tok->value = malloc(asm_len + 1);
                    strncpy(asm_tok->value, asm_start, asm_len);
                    asm_tok->value[asm_len] = '\0';
                    asm_tok->type = TOK_STRING;  // Reuse STRING type for asm content
                    asm_tok->line = line;

                    // Add closing brace token
                    if (*p == '}') {
                        if (count >= capacity) {
                            capacity *= 2;
                            tokens = realloc(tokens, sizeof(Token) * capacity);
                        }
                        Token *close_brace = &tokens[count++];
                        close_brace->type = TOK_RBRACE;
                        close_brace->value = strdup("}");
                        close_brace->line = line;
                        p++;
                    }
                }
            }
            continue;
        }

        // Numbers - FIX: cast to unsigned char
        if (isdigit((unsigned char)*p)) {
            const char *start = p;
            while (isdigit((unsigned char)*p)) p++;
            int len = (int)(p - start);
            tok->value = malloc(len + 1);
            strncpy(tok->value, start, len);
            tok->value[len] = '\0';
            tok->type = TOK_NUMBER;
            continue;
        }

        // String literals
        if (*p == '"') {
            p++;
            int str_capacity = 64;
            char *str_buf = malloc(str_capacity);
            int str_len = 0;
            while (*p && *p != '"') {
                if (str_len + 2 >= str_capacity) {
                    str_capacity *= 2;
                    str_buf = realloc(str_buf, str_capacity);
                }
                if (*p == '\\') {
                    p++;
                    switch (*p) {
                        case 'n': str_buf[str_len++] = '\n'; break;
                        case 'r': str_buf[str_len++] = '\r'; break;
                        case 't': str_buf[str_len++] = '\t'; break;
                        case '0': str_buf[str_len++] = '\0'; break;
                        case '\\': str_buf[str_len++] = '\\'; break;
                        case '"': str_buf[str_len++] = '"'; break;
                        default: str_buf[str_len++] = *p; break;
                    }
                } else {
                    str_buf[str_len++] = *p;
                }
                p++;
            }
            str_buf[str_len] = '\0';
            tok->value = str_buf;
            tok->type = TOK_STRING;
            if (*p) p++;
            continue;
        }

        // Character literals
        if (*p == '\'') {
            p++;
            char c_val = *p;
            if (*p == '\\') {
                p++;
                switch (*p) {
                    case 'n': c_val = '\n'; break;
                    case 'r': c_val = '\r'; break;
                    case 't': c_val = '\t'; break;
                    case '0': c_val = '\0'; break;
                    case '\\': c_val = '\\'; break;
                    case '\'': c_val = '\''; break;
                    default: c_val = *p; break;
                }
            }
            p++;
            if (*p == '\'') p++;
            
            char num_buf[16];
            sprintf(num_buf, "%d", (unsigned char)c_val);
            tok->value = strdup(num_buf);
            tok->type = TOK_NUMBER;
            continue;
        }

        // Three-character operators
        if (*p == '<' && *(p + 1) == '<' && *(p + 2) == '=') {
            tok->type = TOK_SHL_ASSIGN;
            tok->value = strdup("<<=");
            p += 3;
            continue;
        }
        if (*p == '>' && *(p + 1) == '>' && *(p + 2) == '=') {
            tok->type = TOK_SHR_ASSIGN;
            tok->value = strdup(">>=");
            p += 3;
            continue;
        }

        // Two-character operators
        if (*p == '=' && *(p + 1) == '=') {
            tok->type = TOK_EQ;
            tok->value = strdup("==");
            p += 2;
            continue;
        }
        if (*p == '!' && *(p + 1) == '=') {
            tok->type = TOK_NE;
            tok->value = strdup("!=");
            p += 2;
            continue;
        }
        if (*p == '<' && *(p + 1) == '=') {
            tok->type = TOK_LE;
            tok->value = strdup("<=");
            p += 2;
            continue;
        }
        if (*p == '>' && *(p + 1) == '=') {
            tok->type = TOK_GE;
            tok->value = strdup(">=");
            p += 2;
            continue;
        }
        if (*p == '&' && *(p + 1) == '&') {
            tok->type = TOK_AND;
            tok->value = strdup("&&");
            p += 2;
            continue;
        }
        if (*p == '|' && *(p + 1) == '|') {
            tok->type = TOK_OR;
            tok->value = strdup("||");
            p += 2;
            continue;
        }
        if (*p == '<' && *(p + 1) == '<') {
            tok->type = TOK_SHL;
            tok->value = strdup("<<");
            p += 2;
            continue;
        }
        if (*p == '>' && *(p + 1) == '>') {
            tok->type = TOK_SHR;
            tok->value = strdup(">>");
            p += 2;
            continue;
        }
        if (*p == '+' && *(p + 1) == '+') {
            tok->type = TOK_INC;
            tok->value = strdup("++");
            p += 2;
            continue;
        }
        if (*p == '-' && *(p + 1) == '-') {
            tok->type = TOK_DEC;
            tok->value = strdup("--");
            p += 2;
            continue;
        }
        if (*p == '+' && *(p + 1) == '=') {
            tok->type = TOK_PLUS_ASSIGN;
            tok->value = strdup("+=");
            p += 2;
            continue;
        }
        if (*p == '-' && *(p + 1) == '=') {
            tok->type = TOK_MINUS_ASSIGN;
            tok->value = strdup("-=");
            p += 2;
            continue;
        }
        if (*p == '*' && *(p + 1) == '=') {
            tok->type = TOK_STAR_ASSIGN;
            tok->value = strdup("*=");
            p += 2;
            continue;
        }
        if (*p == '/' && *(p + 1) == '=') {
            tok->type = TOK_SLASH_ASSIGN;
            tok->value = strdup("/=");
            p += 2;
            continue;
        }
        if (*p == '%' && *(p + 1) == '=') {
            tok->type = TOK_MOD_ASSIGN;
            tok->value = strdup("%=");
            p += 2;
            continue;
        }
        if (*p == '&' && *(p + 1) == '=') {
            tok->type = TOK_AND_ASSIGN;
            tok->value = strdup("&=");
            p += 2;
            continue;
        }
        if (*p == '|' && *(p + 1) == '=') {
            tok->type = TOK_OR_ASSIGN;
            tok->value = strdup("|=");
            p += 2;
            continue;
        }
        if (*p == '^' && *(p + 1) == '=') {
            tok->type = TOK_XOR_ASSIGN;
            tok->value = strdup("^=");
            p += 2;
            continue;
        }

        if (*p == '-' && *(p + 1) == '>') {
            tok->type = TOK_ARROW;
            tok->value = strdup("->");
            p += 2;
            continue;
        }

        // Single-character tokens
        tok->value = malloc(2);
        tok->value[0] = *p;
        tok->value[1] = '\0';

        switch (*p) {
            case '(': tok->type = TOK_LPAREN; break;
            case ')': tok->type = TOK_RPAREN; break;
            case '{': tok->type = TOK_LBRACE; break;
            case '}': tok->type = TOK_RBRACE; break;
            case '[': tok->type = TOK_LBRACKET; break;
            case ']': tok->type = TOK_RBRACKET; break;
            case '.': tok->type = TOK_DOT; break;
            case ':': tok->type = TOK_COLON; break;
            case ';': tok->type = TOK_SEMICOLON; break;
            case ',': tok->type = TOK_COMMA; break;
            case '=': tok->type = TOK_ASSIGN; break;
            case '+': tok->type = TOK_PLUS; break;
            case '-': tok->type = TOK_MINUS; break;
            case '*': tok->type = TOK_STAR; break;
            case '/': tok->type = TOK_SLASH; break;
            case '%': tok->type = TOK_PERCENT; break;
            case '<': tok->type = TOK_LT; break;
            case '>': tok->type = TOK_GT; break;
            case '!': tok->type = TOK_NOT; break;
            case '&': tok->type = TOK_AMPERSAND; break;
            case '|': tok->type = TOK_PIPE; break;
            case '^': tok->type = TOK_CARET; break;
            case '~': tok->type = TOK_TILDE; break;
            default:
                // Skip unknown characters (non-ASCII, invalid UTF-8, etc.)
                if ((unsigned char)*p < 32 || (unsigned char)*p > 126) {
                    fprintf(stderr, "Warning: Skipping non-ASCII character (code %d) at line %d\n", 
                            (unsigned char)*p, line);
                } else {
                    fprintf(stderr, "Error: Unknown character '%c' at line %d\n", *p, line);
                }
                free(tok->value);
                count--;  // Don't add this token
                p++;
                continue;
        }
        p++;
    }

    // Add EOF token
    if (count >= capacity) {
        capacity++;
        tokens = realloc(tokens, sizeof(Token) * capacity);
    }
    tokens[count].type = TOK_EOF;
    tokens[count].value = NULL;
    tokens[count].line = line;
    count++;

    *token_count = count;
    return tokens;
}

void free_tokens(Token *tokens, int count) {
    for (int i = 0; i < count; i++) {
        free(tokens[i].value);
    }
    free(tokens);
}
