#include "c_to_i8080.h"

#ifndef TOK_SWITCH
#define TOK_SWITCH 1000
#define TOK_CASE 1001
#define TOK_DEFAULT 1002
#define TOK_COLON 1003
#endif

#ifndef AST_SWITCH
#define AST_SWITCH 1000
#define AST_CASE 1001
#define AST_DEFAULT 1002
#endif

ASTNode* create_node(ASTNodeType type, const char *value) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = type;
    node->value = value ? strdup(value) : NULL;
    node->datatype = 2; // Default to 16-bit
    node->initializer_value = NULL;
    node->array_size = 0;
    node->is_reg = false;
    node->is_static = false;
    node->is_used = false;
    node->children = NULL;
    node->child_count = 0;
    node->child_capacity = 0;
    return node;
}

void add_child(ASTNode *parent, ASTNode *child) {
    if (parent->child_count >= parent->child_capacity) {
        parent->child_capacity = parent->child_capacity == 0 ? 4 : parent->child_capacity * 2;
        parent->children = realloc(parent->children, sizeof(ASTNode*) * parent->child_capacity);
    }
    parent->children[parent->child_count++] = child;
}

void free_ast(ASTNode *node) {
    if (!node) return;
    for (int i = 0; i < node->child_count; i++) {
        free_ast(node->children[i]);
    }
    free(node->initializer_value);
    free(node->children);
    free(node->value);
    free(node);
}

static Token* current;
static Token* peek() { return current; }
static Token* next() { return current++; }
static bool match(TokenType type) { return peek()->type == type; }

static const char* token_type_to_string(int type) {
    switch (type) {
        case TOK_EOF: return "EOF";
        case TOK_INT: return "'int'";
        case TOK_SHORT: return "'short'";
        case TOK_CHAR: return "'char'";
        case TOK_VOID: return "'void'";
        case TOK_RETURN: return "'return'";
        case TOK_IF: return "'if'";
        case TOK_ELSE: return "'else'";
        case TOK_WHILE: return "'while'";
        case TOK_FOR: return "'for'";
        case TOK_DO: return "'do'";
        case TOK_ASM: return "'asm'";
        case TOK_BREAK: return "'break'";
        case TOK_CONTINUE: return "'continue'";
        case TOK_REG: return "'reg'";
        case TOK_STATIC: return "'static'";
        case TOK_GOTO: return "'goto'";
        case TOK_IDENT: return "identifier";
        case TOK_NUMBER: return "number";
        case TOK_STRING: return "string";
        case TOK_LPAREN: return "'('";
        case TOK_RPAREN: return "')'";
        case TOK_LBRACE: return "'{'";
        case TOK_RBRACE: return "'}'";
        case TOK_LBRACKET: return "'['";
        case TOK_RBRACKET: return "']'";
        case TOK_SEMICOLON: return "';'";
        case TOK_COMMA: return "','";
        case TOK_ASSIGN: return "'='";
        case TOK_PLUS_ASSIGN: return "'+='";
        case TOK_MINUS_ASSIGN: return "'-='";
        case TOK_STAR_ASSIGN: return "'*='";
        case TOK_SLASH_ASSIGN: return "'/='";
        case TOK_AND_ASSIGN: return "'&='";
        case TOK_OR_ASSIGN: return "'|='";
        case TOK_XOR_ASSIGN: return "'^='";
        case TOK_SHL_ASSIGN: return "'<<='";
        case TOK_SHR_ASSIGN: return "'>>='";
        case TOK_MOD_ASSIGN: return "'%='";
        case TOK_PLUS: return "'+'";
        case TOK_MINUS: return "'-'";
        case TOK_STAR: return "'*'";
        case TOK_SLASH: return "'/'";
        case TOK_PERCENT: return "'%'";
        case TOK_INC: return "'++'";
        case TOK_DEC: return "'--'";
        case TOK_EQ: return "'=='";
        case TOK_NE: return "'!='";
        case TOK_LT: return "'<'";
        case TOK_LE: return "'<='";
        case TOK_GT: return "'>'";
        case TOK_GE: return "'>='";
        case TOK_AND: return "'&&'";
        case TOK_OR: return "'||'";
        case TOK_NOT: return "'!'";
        case TOK_AMPERSAND: return "'&'";
        case TOK_PIPE: return "'|'";
        case TOK_CARET: return "'^'";
        case TOK_TILDE: return "'~'";
        case TOK_SHL: return "'<<'";
        case TOK_SHR: return "'>>'";
        case TOK_SWITCH: return "'switch'";
        case TOK_CASE: return "'case'";
        case TOK_DEFAULT: return "'default'";
        case TOK_COLON: return "':'";
        default: return "unknown";
    }
}

static Token* expect(TokenType type) {
    if (!match(type)) {
        fprintf(stderr, "Expected %s but got %s at line %d\n", token_type_to_string(type), token_type_to_string(peek()->type), peek()->line);
        exit(1);
    }
    return next();
}

// Forward declarations
static ASTNode* parse_expression();
static ASTNode* parse_statement();
static ASTNode* parse_unary();

static ASTNode* parse_primary() {
    if (match(TOK_NUMBER)) {
        Token *tok = next();
        return create_node(AST_NUMBER, tok->value);
    }
    if (match(TOK_STRING)) {
        Token *tok = next();
        return create_node(AST_STRING, tok->value);
    }
    if (match(TOK_IDENT)) {
        Token *tok = next();
        if (match(TOK_LPAREN)) {
            // Function call
            next(); // consume '('
            ASTNode *call = create_node(AST_CALL, tok->value);
            if (!match(TOK_RPAREN)) {
                add_child(call, parse_expression());
                while (match(TOK_COMMA)) {
                    next();
                    add_child(call, parse_expression());
                }
            }
            expect(TOK_RPAREN);
            return call;
        } else if (match(TOK_LBRACKET)) {
            next(); // consume '['
            ASTNode *access = create_node(AST_ARRAY_ACCESS, tok->value);
            add_child(access, parse_expression());
            expect(TOK_RBRACKET);
            return access;
        }
        return create_node(AST_IDENT, tok->value);
    }
    if (match(TOK_LPAREN)) {
        next();
        ASTNode *expr = parse_expression();
        expect(TOK_RPAREN);
        return expr;
    }
    fprintf(stderr, "Unexpected token in primary expression at line %d\n", peek()->line);
    exit(1);
}

static ASTNode* parse_postfix() {
    ASTNode *left = parse_primary();
    while (match(TOK_INC) || match(TOK_DEC) || match(TOK_LPAREN)) {
        if (match(TOK_LPAREN)) {
            next();
            ASTNode *call = create_node(AST_INDIRECT_CALL, NULL);
            add_child(call, left);
            if (!match(TOK_RPAREN)) {
                add_child(call, parse_expression());
                while (match(TOK_COMMA)) {
                    next();
                    add_child(call, parse_expression());
                }
            }
            expect(TOK_RPAREN);
            left = call;
        } else {
            Token *op = next();
            ASTNodeType type = (strcmp(op->value, "++") == 0) ? AST_POST_INC : AST_POST_DEC;
            ASTNode *node = create_node(type, op->value);
            add_child(node, left);
            left = node;
        }
    }
    return left;
}

static ASTNode* parse_unary() {
    // Handle unary operators
    if (match(TOK_MINUS) || match(TOK_NOT) || match(TOK_TILDE) || match(TOK_INC) || match(TOK_DEC)) {
        Token *op = next();
        if (strcmp(op->value, "++") == 0) {
            ASTNode *node = create_node(AST_PRE_INC, "++");
            add_child(node, parse_unary());
            return node;
        }
        if (strcmp(op->value, "--") == 0) {
            ASTNode *node = create_node(AST_PRE_DEC, "--");
            add_child(node, parse_unary());
            return node;
        }
        ASTNode *node = create_node(AST_UNARY_OP, op->value);
        add_child(node, parse_unary());
        return node;
    }
    // Handle pointer dereference: *ptr
    if (match(TOK_STAR)) {
        next();
        ASTNode *node = create_node(AST_DEREF, "*");
        add_child(node, parse_unary());
        return node;
    }
    // Handle address-of: &var
    if (match(TOK_AMPERSAND)) {
        next();
        ASTNode *node = create_node(AST_ADDROF, "&");
        add_child(node, parse_unary());
        return node;
    }
    return parse_postfix();
}

static ASTNode* parse_multiplicative() {
    ASTNode *left = parse_unary();
    while (match(TOK_STAR) || match(TOK_SLASH) || match(TOK_PERCENT)) {
        Token *op = next();
        ASTNode *node = create_node(AST_BINARY_OP, op->value);
        add_child(node, left);
        add_child(node, parse_unary());
        left = node;
    }
    return left;
}

static ASTNode* parse_additive() {
    ASTNode *left = parse_multiplicative();
    while (match(TOK_PLUS) || match(TOK_MINUS)) {
        Token *op = next();
        ASTNode *node = create_node(AST_BINARY_OP, op->value);
        add_child(node, left);
        add_child(node, parse_multiplicative());
        left = node;
    }
    return left;
}

static ASTNode* parse_shift() {
    ASTNode *left = parse_additive();
    while (match(TOK_SHL) || match(TOK_SHR)) {
        Token *op = next();
        ASTNode *node = create_node(AST_BINARY_OP, op->value);
        add_child(node, left);
        add_child(node, parse_additive());
        left = node;
    }
    return left;
}

static ASTNode* parse_relational() {
    ASTNode *left = parse_shift();
    while (match(TOK_LT) || match(TOK_LE) || match(TOK_GT) || match(TOK_GE)) {
        Token *op = next();
        ASTNode *node = create_node(AST_BINARY_OP, op->value);
        add_child(node, left);
        add_child(node, parse_shift());
        left = node;
    }
    return left;
}

static ASTNode* parse_equality() {
    ASTNode *left = parse_relational();
    while (match(TOK_EQ) || match(TOK_NE)) {
        Token *op = next();
        ASTNode *node = create_node(AST_BINARY_OP, op->value);
        add_child(node, left);
        add_child(node, parse_relational());
        left = node;
    }
    return left;
}

static ASTNode* parse_bitwise_and() {
    ASTNode *left = parse_equality();
    while (match(TOK_AMPERSAND)) {
        Token *op = next();
        ASTNode *node = create_node(AST_BINARY_OP, op->value);
        add_child(node, left);
        add_child(node, parse_equality());
        left = node;
    }
    return left;
}

static ASTNode* parse_bitwise_xor() {
    ASTNode *left = parse_bitwise_and();
    while (match(TOK_CARET)) {
        Token *op = next();
        ASTNode *node = create_node(AST_BINARY_OP, op->value);
        add_child(node, left);
        add_child(node, parse_bitwise_and());
        left = node;
    }
    return left;
}

static ASTNode* parse_bitwise_or() {
    ASTNode *left = parse_bitwise_xor();
    while (match(TOK_PIPE)) {
        Token *op = next();
        ASTNode *node = create_node(AST_BINARY_OP, op->value);
        add_child(node, left);
        add_child(node, parse_bitwise_xor());
        left = node;
    }
    return left;
}

static ASTNode* parse_logical_and() {
    ASTNode *left = parse_bitwise_or();
    while (match(TOK_AND)) {
        Token *op = next();
        ASTNode *node = create_node(AST_BINARY_OP, op->value);
        add_child(node, left);
        add_child(node, parse_bitwise_or());
        left = node;
    }
    return left;
}

static ASTNode* parse_logical_or() {
    ASTNode *left = parse_logical_and();
    while (match(TOK_OR)) {
        Token *op = next();
        ASTNode *node = create_node(AST_BINARY_OP, op->value);
        add_child(node, left);
        add_child(node, parse_logical_and());
        left = node;
    }
    return left;
}

static ASTNode* parse_assignment() {
    ASTNode *left = parse_logical_or();
    if (match(TOK_ASSIGN) || match(TOK_PLUS_ASSIGN) || match(TOK_MINUS_ASSIGN) || 
        match(TOK_STAR_ASSIGN) || match(TOK_SLASH_ASSIGN) || match(TOK_MOD_ASSIGN) ||
        match(TOK_AND_ASSIGN) || match(TOK_OR_ASSIGN) || match(TOK_XOR_ASSIGN) ||
        match(TOK_SHL_ASSIGN) || match(TOK_SHR_ASSIGN)) {
        Token *op = next();
        ASTNode *node;
        if (op->type == TOK_ASSIGN) {
            node = create_node(AST_ASSIGN, "=");
        } else {
            char *op_val = NULL;
            if (op->type == TOK_PLUS_ASSIGN) op_val = "+";
            else if (op->type == TOK_MINUS_ASSIGN) op_val = "-";
            else if (op->type == TOK_STAR_ASSIGN) op_val = "*";
            else if (op->type == TOK_SLASH_ASSIGN) op_val = "/";
            else if (op->type == TOK_MOD_ASSIGN) op_val = "%";
            else if (op->type == TOK_AND_ASSIGN) op_val = "&";
            else if (op->type == TOK_OR_ASSIGN) op_val = "|";
            else if (op->type == TOK_XOR_ASSIGN) op_val = "^";
            else if (op->type == TOK_SHL_ASSIGN) op_val = "<<";
            else if (op->type == TOK_SHR_ASSIGN) op_val = ">>";
            node = create_node(AST_COMPOUND_ASSIGN, op_val);
        }
        add_child(node, left);
        add_child(node, parse_assignment());
        return node;
    }
    return left;
}

static ASTNode* parse_expression() {
    return parse_assignment();
}

static Token* parse_declarator(int *pointer_level, bool *is_func_ptr) {
    Token *name = NULL;
    *pointer_level = 0;
    if (is_func_ptr) *is_func_ptr = false;
    
    while (match(TOK_STAR)) {
        (*pointer_level)++;
        next();
    }
    
    if (match(TOK_LPAREN)) {
        next(); // '('
        while (match(TOK_STAR)) {
            (*pointer_level)++;
            if (is_func_ptr) *is_func_ptr = true;
            next();
        }
        name = expect(TOK_IDENT);
        expect(TOK_RPAREN);
        
        if (match(TOK_LPAREN)) {
            next(); // '('
            int paren_depth = 1;
            while (paren_depth > 0 && !match(TOK_EOF)) {
                if (match(TOK_LPAREN)) paren_depth++;
                else if (match(TOK_RPAREN)) paren_depth--;
                next();
            }
        }
    } else {
        name = expect(TOK_IDENT);
    }
    return name;
}

static void parse_initializer(ASTNode *vardecl, bool is_static, bool is_array) {
    if (match(TOK_LBRACE)) {
        next();
        if (is_static) {
            char init_buf[4096] = {0};
            int count = 0;
            while (!match(TOK_RBRACE) && !match(TOK_EOF)) {
                ASTNode *init_expr = parse_expression();
                if (init_expr->type == AST_NUMBER) {
                    if (count > 0) strcat(init_buf, ",");
                    strcat(init_buf, init_expr->value);
                } else if (init_expr->type == AST_UNARY_OP && strcmp(init_expr->value, "-") == 0 && init_expr->children[0]->type == AST_NUMBER) {
                    if (count > 0) strcat(init_buf, ",");
                    strcat(init_buf, "-");
                    strcat(init_buf, init_expr->children[0]->value);
                } else {
                    fprintf(stderr, "Error: Static arrays must be initialized with constant numbers at line %d\n", peek()->line);
                    exit(1);
                }
                free_ast(init_expr);
                count++;
                if (match(TOK_COMMA)) next();
                else break;
            }
            expect(TOK_RBRACE);
            if (vardecl->array_size == 0) vardecl->array_size = count;
            else {
                while (count < vardecl->array_size) {
                    if (count > 0) strcat(init_buf, ",");
                    strcat(init_buf, "0");
                    count++;
                }
            }
            vardecl->initializer_value = strdup(init_buf);
        } else {
            int count = 0;
            while (!match(TOK_RBRACE) && !match(TOK_EOF)) {
                ASTNode *init_expr = parse_expression();
                add_child(vardecl, init_expr);
                count++;
                if (match(TOK_COMMA)) next();
                else break;
            }
            expect(TOK_RBRACE);
            if (vardecl->array_size == 0) vardecl->array_size = count;
        }
    } else {
        ASTNode *init_expr = parse_expression();
        if (is_array && init_expr->type == AST_STRING) {
            int len = strlen(init_expr->value);
            if (vardecl->array_size == 0) vardecl->array_size = len + 1;
            if (is_static) {
                char init_buf[4096] = {0};
                for (int i = 0; i < len; i++) {
                    char num[16];
                    sprintf(num, "%d,", (unsigned char)init_expr->value[i]);
                    strcat(init_buf, num);
                }
                strcat(init_buf, "0");
                int count = len + 1;
                while (count < vardecl->array_size) {
                    strcat(init_buf, ",0");
                    count++;
                }
                vardecl->initializer_value = strdup(init_buf);
            } else {
                for (int i = 0; i < len; i++) {
                    char num[16];
                    sprintf(num, "%d", (unsigned char)init_expr->value[i]);
                    add_child(vardecl, create_node(AST_NUMBER, num));
                }
                add_child(vardecl, create_node(AST_NUMBER, "0"));
            }
            free_ast(init_expr);
        } else if (is_static) {
            if (init_expr->type == AST_NUMBER) {
                vardecl->initializer_value = strdup(init_expr->value);
            } else if (init_expr->type == AST_UNARY_OP && strcmp(init_expr->value, "-") == 0 && init_expr->children[0]->type == AST_NUMBER) {
                char buffer[32];
                sprintf(buffer, "-%s", init_expr->children[0]->value);
                vardecl->initializer_value = strdup(buffer);
            } else if (init_expr->type == AST_IDENT) {
                vardecl->initializer_value = strdup(init_expr->value);
            } else if (init_expr->type == AST_ADDROF && init_expr->children[0]->type == AST_IDENT) {
                vardecl->initializer_value = strdup(init_expr->children[0]->value);
            } else {
                fprintf(stderr, "Error: Static variables must be initialized with constant numbers or function names at line %d\n", peek()->line);
                exit(1);
            }
            free_ast(init_expr);
        } else {
            add_child(vardecl, init_expr);
        }
    }
}

static ASTNode* parse_block() {
    expect(TOK_LBRACE);
    ASTNode *block = create_node(AST_BLOCK, NULL);
    while (!match(TOK_RBRACE) && !match(TOK_EOF)) {
        add_child(block, parse_statement());
    }
    expect(TOK_RBRACE);
    return block;
}

static ASTNode* parse_statement() {
    bool is_reg = false;
    bool is_static = false;
    if (match(TOK_REG)) {
        is_reg = true;
        next();
    } else if (match(TOK_STATIC)) {
        is_static = true;
        next();
    }

    // Variable declaration
    if (match(TOK_INT) || match(TOK_SHORT) || match(TOK_CHAR) || match(TOK_VOID)) {
        bool target_16bit = true;
        if (match(TOK_SHORT)) {
            next();
            if (match(TOK_INT)) next(); // optional "short int"
        } else if (match(TOK_CHAR)) {
            target_16bit = false;
            next();
        } else if (match(TOK_VOID)) {
            target_16bit = true;
            next();
        } else {
            next();
        }
        
        bool is_array_prefix = false;
        int prefix_array_size = 0;
        if (match(TOK_LBRACKET)) {
            is_array_prefix = true;
            next();
            if (match(TOK_NUMBER)) {
                Token *num = next();
                prefix_array_size = atoi(num->value);
            }
            expect(TOK_RBRACKET);
        }        
        
        int pointer_level = 0;
        Token *name = parse_declarator(&pointer_level, NULL);
        
        ASTNode *vardecl = create_node(AST_VARDECL, name->value);
        vardecl->datatype = target_16bit ? 2 : 1;
        vardecl->is_reg = is_reg;
        vardecl->is_static = is_static;
        if (pointer_level > 0) {
            char *ptr_name = malloc(strlen(name->value) + pointer_level + 1);
            ptr_name[0] = '\0';
            for (int i = 0; i < pointer_level; i++) strcat(ptr_name, "*");
            strcat(ptr_name, name->value);
            free(vardecl->value);
            vardecl->value = ptr_name;
        }
        bool is_array = is_array_prefix;
        vardecl->array_size = prefix_array_size;
        if (match(TOK_LBRACKET)) {
            is_array = true;
            next();
            if (match(TOK_NUMBER)) {
                Token *num = next();
                vardecl->array_size = atoi(num->value);
            }
            expect(TOK_RBRACKET);
        }
        if (match(TOK_ASSIGN)) {
            next();
            parse_initializer(vardecl, is_static, is_array);
        }
        expect(TOK_SEMICOLON);
        return vardecl;
    } else if (is_reg || is_static) {
        fprintf(stderr, "Error: Expected type after modifier keyword at line %d\n", peek()->line);
        exit(1);
    }

    // Inline assembly block
    if (match(TOK_ASM)) {
        next();  // consume 'asm'
        expect(TOK_LBRACE);  // consume '{'

        // The lexer has placed the raw assembly code in a STRING token
        Token *asm_token = expect(TOK_STRING);
        ASTNode *asm_node = create_node(AST_ASM, asm_token->value);

        expect(TOK_RBRACE);  // consume '}'
        return asm_node;
    }

    // Return statement
    if (match(TOK_RETURN)) {
        next();
        ASTNode *ret = create_node(AST_RETURN, NULL);
        if (!match(TOK_SEMICOLON)) {
            add_child(ret, parse_expression());
        }
        expect(TOK_SEMICOLON);
        return ret;
    }

    // Break statement
    if (match(TOK_BREAK)) {
        next();
        expect(TOK_SEMICOLON);
        return create_node(AST_BREAK, NULL);
    }

    // Continue statement
    if (match(TOK_CONTINUE)) {
        next();
        expect(TOK_SEMICOLON);
        return create_node(AST_CONTINUE, NULL);
    }

    // If statement
    if (match(TOK_IF)) {
        next();
        expect(TOK_LPAREN);
        ASTNode *if_stmt = create_node(AST_IF, NULL);
        add_child(if_stmt, parse_expression());
        expect(TOK_RPAREN);
        add_child(if_stmt, parse_statement());
        if (match(TOK_ELSE)) {
            next();
            add_child(if_stmt, parse_statement());
        }
        return if_stmt;
    }

    // While statement
    if (match(TOK_WHILE)) {
        next();
        expect(TOK_LPAREN);
        ASTNode *while_stmt = create_node(AST_WHILE, NULL);
        add_child(while_stmt, parse_expression());
        expect(TOK_RPAREN);
        add_child(while_stmt, parse_statement());
        return while_stmt;
    }

    // Do-while statement
    if (match(TOK_DO)) {
        next();
        ASTNode *do_while = create_node(AST_DO_WHILE, NULL);
        add_child(do_while, parse_statement());  // body
        expect(TOK_WHILE);
        expect(TOK_LPAREN);
        add_child(do_while, parse_expression());  // condition
        expect(TOK_RPAREN);
        expect(TOK_SEMICOLON);
        return do_while;
    }

    // Goto statement
    if (match(TOK_GOTO)) {
        next();
        Token *name = expect(TOK_IDENT);
        expect(TOK_SEMICOLON);
        return create_node(AST_GOTO, name->value);
    }

    // Label statement
    if (match(TOK_IDENT) && (current + 1)->type == TOK_COLON) {
        Token *name = next(); // consume ident
        next(); // consume colon
        ASTNode *label_node = create_node(AST_LABEL, name->value);
        add_child(label_node, parse_statement());
        return label_node;
    }

    // Switch statement
    if (match(TOK_SWITCH)) {
        next();
        expect(TOK_LPAREN);
        ASTNode *switch_stmt = create_node(AST_SWITCH, NULL);
        add_child(switch_stmt, parse_expression());
        expect(TOK_RPAREN);
        add_child(switch_stmt, parse_statement());
        return switch_stmt;
    }

    // Case label
    if (match(TOK_CASE)) {
        next();
        ASTNode *case_stmt = create_node(AST_CASE, NULL);
        add_child(case_stmt, parse_expression());
        expect(TOK_COLON);
        add_child(case_stmt, parse_statement());
        return case_stmt;
    }

    // Default label
    if (match(TOK_DEFAULT)) {
        next();
        expect(TOK_COLON);
        ASTNode *default_stmt = create_node(AST_DEFAULT, NULL);
        add_child(default_stmt, parse_statement());
        return default_stmt;
    }

    // For statement: for (init; condition; increment) body
    if (match(TOK_FOR)) {
        next();
        expect(TOK_LPAREN);
        ASTNode *for_stmt = create_node(AST_FOR, NULL);

        bool is_for_reg = false;
        bool is_for_static = false;
        if (match(TOK_REG)) {
            is_for_reg = true;
            next();
        } else if (match(TOK_STATIC)) {
            is_for_static = true;
            next();
        }

        // Initialization (can be variable declaration or expression)
        if (match(TOK_INT) || match(TOK_SHORT) || match(TOK_CHAR) || match(TOK_VOID)) {
            bool target_16bit = true;
            if (match(TOK_SHORT)) {
                next();
                if (match(TOK_INT)) next(); // optional "short int"
            } else if (match(TOK_CHAR)) {
                target_16bit = false;
                next();
            } else if (match(TOK_VOID)) {
                target_16bit = true;
                next();
            } else {
                next();
            }

            bool is_array_prefix = false;
            int prefix_array_size = 0;
            if (match(TOK_LBRACKET)) {
                is_array_prefix = true;
                next();
                if (match(TOK_NUMBER)) {
                    Token *num = next();
                    prefix_array_size = atoi(num->value);
                }
                expect(TOK_RBRACKET);
            }

            int pointer_level = 0;
            Token *name = parse_declarator(&pointer_level, NULL);
            
            ASTNode *vardecl = create_node(AST_VARDECL, name->value);
            vardecl->datatype = target_16bit ? 2 : 1;
            vardecl->is_reg = is_for_reg;
            vardecl->is_static = is_for_static;
            if (pointer_level > 0) {
                char *ptr_name = malloc(strlen(name->value) + pointer_level + 1);
                ptr_name[0] = '\0';
                for (int i = 0; i < pointer_level; i++) strcat(ptr_name, "*");
                strcat(ptr_name, name->value);
                free(vardecl->value);
                vardecl->value = ptr_name;
            }
            bool is_array = is_array_prefix;
            vardecl->array_size = prefix_array_size;
            if (match(TOK_LBRACKET)) {
                is_array = true;
                next();
                if (match(TOK_NUMBER)) {
                    Token *num = next();
                    vardecl->array_size = atoi(num->value);
                }
                expect(TOK_RBRACKET);
            }
            if (match(TOK_ASSIGN)) {
                next();
                parse_initializer(vardecl, is_for_static, is_array);
            }
            add_child(for_stmt, vardecl);
        } else if (is_for_reg || is_for_static) {
            fprintf(stderr, "Error: Expected type after modifier keyword at line %d\n", peek()->line);
            exit(1);
        } else if (!match(TOK_SEMICOLON)) {
            add_child(for_stmt, parse_expression());
        } else {
            add_child(for_stmt, create_node(AST_EXPR_STMT, NULL));  // empty init
        }
        expect(TOK_SEMICOLON);

        // Condition
        if (!match(TOK_SEMICOLON)) {
            add_child(for_stmt, parse_expression());
        } else {
            // No condition means infinite loop (treat as true)
            add_child(for_stmt, create_node(AST_NUMBER, "1"));
        }
        expect(TOK_SEMICOLON);

        // Increment
        if (!match(TOK_RPAREN)) {
            add_child(for_stmt, parse_expression());
        } else {
            add_child(for_stmt, create_node(AST_EXPR_STMT, NULL));  // empty increment
        }
        expect(TOK_RPAREN);

        // Body
        add_child(for_stmt, parse_statement());
        return for_stmt;
    }

    // Block
    if (match(TOK_LBRACE)) {
        return parse_block();
    }

    // Expression statement
    if (match(TOK_SEMICOLON)) {
        next();
        return create_node(AST_EXPR_STMT, NULL); // Safe empty statement parsing
    }
    
    ASTNode *expr = create_node(AST_EXPR_STMT, NULL);
    add_child(expr, parse_expression());
    expect(TOK_SEMICOLON);
    return expr;
}

static ASTNode* parse_top_level() {
    // Return type or global variable type
    if (!match(TOK_INT) && !match(TOK_SHORT) && !match(TOK_CHAR) && !match(TOK_VOID)) {
        return NULL;
    }
    
    bool target_16bit = true;
    if (match(TOK_SHORT)) {
        next();
        if (match(TOK_INT)) next(); // optional "short int"
    } else if (match(TOK_CHAR)) {
        target_16bit = false;
        next();
    } else if (match(TOK_VOID)) {
        target_16bit = true;
        next();
    } else {
        next();
    }
    
    bool is_array_prefix = false;
    int prefix_array_size = 0;
    if (match(TOK_LBRACKET)) {
        is_array_prefix = true;
        next();
        if (match(TOK_NUMBER)) {
            Token *num = next();
            prefix_array_size = atoi(num->value);
        }
        expect(TOK_RBRACKET);
    }
    
    int pointer_level = 0;
    bool is_func_ptr = false;
    Token *name = parse_declarator(&pointer_level, &is_func_ptr);

    if (!is_func_ptr && match(TOK_LPAREN)) {
        // It is a function
        ASTNode *func = create_node(AST_FUNCTION, name->value);
        next(); // consume '('
        while (!match(TOK_RPAREN) && !match(TOK_EOF)) {
            bool is_param_reg = false;
            if (match(TOK_REG)) {
                is_param_reg = true;
                next();
            }
            if (match(TOK_INT) || match(TOK_SHORT) || match(TOK_CHAR) || match(TOK_VOID)) {
                bool p_target_16bit = true;
                if (match(TOK_SHORT)) {
                    next();
                    if (match(TOK_INT)) next(); // optional "short int"
                } else if (match(TOK_CHAR)) {
                    p_target_16bit = false;
                    next();
                } else if (match(TOK_VOID)) {
                    p_target_16bit = true;
                    next();
                } else {
                    next();
                }

                bool is_p_array_prefix = false;
                if (match(TOK_LBRACKET)) {
                    is_p_array_prefix = true;
                    next();
                    if (match(TOK_NUMBER)) next(); // Skip optional size
                    expect(TOK_RBRACKET);
                }

                int p_pointer_level = 0;
                Token *pname = parse_declarator(&p_pointer_level, NULL);
                
                ASTNode *param = create_node(AST_VARDECL, pname->value);
                param->datatype = p_target_16bit ? 2 : 1;
                param->is_reg = is_param_reg;
                if (p_pointer_level > 0) {
                    char *ptr_name = malloc(strlen(pname->value) + p_pointer_level + 1);
                    ptr_name[0] = '\0';
                    for (int i = 0; i < p_pointer_level; i++) strcat(ptr_name, "*");
                    strcat(ptr_name, pname->value);
                    free(param->value);
                    param->value = ptr_name;
                }
                if (match(TOK_LBRACKET) || is_p_array_prefix) {
                    if (!is_p_array_prefix) {
                        next();
                        if (match(TOK_NUMBER)) next(); // Skip optional size
                        expect(TOK_RBRACKET);
                    }
                    char *ptr_name = malloc(strlen(param->value) + 2);
                    sprintf(ptr_name, "*%s", param->value);
                    free(param->value);
                    param->value = ptr_name;
                }
                add_child(func, param);
            } else {
                fprintf(stderr, "Expected parameter type at line %d\n", peek()->line);
                exit(1);
            }
            if (match(TOK_COMMA)) {
                next();
            } else {
                break;
            }
        }
        expect(TOK_RPAREN);

        add_child(func, parse_block());
        return func;
    } else {
        // It is a global variable
        ASTNode *vardecl = create_node(AST_VARDECL, name->value);
        vardecl->datatype = target_16bit ? 2 : 1;
        if (pointer_level > 0) {
            char *ptr_name = malloc(strlen(name->value) + pointer_level + 1);
            ptr_name[0] = '\0';
            for (int i = 0; i < pointer_level; i++) strcat(ptr_name, "*");
            strcat(ptr_name, name->value);
            free(vardecl->value);
            vardecl->value = ptr_name;
        }
        bool is_array = is_array_prefix;
        vardecl->array_size = prefix_array_size;
        if (match(TOK_LBRACKET)) {
            is_array = true;
            next();
            if (match(TOK_NUMBER)) {
                Token *num = next();
                vardecl->array_size = atoi(num->value);
            }
            expect(TOK_RBRACKET);
        }
        if (match(TOK_ASSIGN)) {
            next();
            parse_initializer(vardecl, true, is_array);
        }
        expect(TOK_SEMICOLON);
        return vardecl;
    }
}

ASTNode* parse(Token *tokens, int token_count) {
    current = tokens;
    ASTNode *program = create_node(AST_PROGRAM, NULL);

    while (!match(TOK_EOF)) {
        if (match(TOK_ASM)) {
            next(); // consume 'asm'
            expect(TOK_LBRACE);
            Token *asm_tok = expect(TOK_STRING);
            ASTNode *asm_node = create_node(AST_ASM, asm_tok->value);
            expect(TOK_RBRACE);
            add_child(program, asm_node);
        } else {
            ASTNode *node = parse_top_level();
            if (node) {
                add_child(program, node);
            } else {
                fprintf(stderr, "Error: Unexpected token '%s' at top level (line %d)\n", peek()->value ? peek()->value : "EOF", peek()->line);
                exit(1);
            }
        }
    }

    return program;
}
