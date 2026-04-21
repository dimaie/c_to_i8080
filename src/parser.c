#include "c_to_i8080.h"

ASTNode* create_node(ASTNodeType type, const char *value) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = type;
    node->value = value ? strdup(value) : NULL;
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
    free(node->children);
    free(node->value);
    free(node);
}

static Token* current;
static Token* peek() { return current; }
static Token* next() { return current++; }
static bool match(TokenType type) { return peek()->type == type; }
static Token* expect(TokenType type) {
    if (!match(type)) {
        fprintf(stderr, "Expected token type %d but got %d at line %d\n", type, peek()->type, peek()->line);
        exit(1);
    }
    return next();
}

// Forward declarations
static ASTNode* parse_expression();
static ASTNode* parse_statement();

static ASTNode* parse_primary() {
    if (match(TOK_NUMBER)) {
        Token *tok = next();
        return create_node(AST_NUMBER, tok->value);
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

static ASTNode* parse_unary() {
    // Handle unary operators
    if (match(TOK_MINUS) || match(TOK_NOT) || match(TOK_TILDE)) {
        Token *op = next();
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
    return parse_primary();
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

static ASTNode* parse_relational() {
    ASTNode *left = parse_additive();
    while (match(TOK_LT) || match(TOK_LE) || match(TOK_GT) || match(TOK_GE)) {
        Token *op = next();
        ASTNode *node = create_node(AST_BINARY_OP, op->value);
        add_child(node, left);
        add_child(node, parse_additive());
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

static ASTNode* parse_assignment() {
    ASTNode *left = parse_equality();
    if (match(TOK_ASSIGN)) {
        next();
        ASTNode *node = create_node(AST_ASSIGN, "=");
        add_child(node, left);
        add_child(node, parse_assignment());
        return node;
    }
    return left;
}

static ASTNode* parse_expression() {
    return parse_assignment();
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
    // Variable declaration
    if (match(TOK_INT) || match(TOK_CHAR)) {
        next();
        // Check for pointer declaration
        bool is_pointer = false;
        if (match(TOK_STAR)) {
            is_pointer = true;
            next();
        }
        Token *name = expect(TOK_IDENT);
        ASTNode *vardecl = create_node(AST_VARDECL, name->value);
        // Store pointer info in the node value (hacky but simple)
        if (is_pointer) {
            char *ptr_name = malloc(strlen(name->value) + 2);
            sprintf(ptr_name, "*%s", name->value);
            free(vardecl->value);
            vardecl->value = ptr_name;
        }
        if (match(TOK_ASSIGN)) {
            next();
            add_child(vardecl, parse_expression());
        }
        expect(TOK_SEMICOLON);
        return vardecl;
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

    // For statement: for (init; condition; increment) body
    if (match(TOK_FOR)) {
        next();
        expect(TOK_LPAREN);
        ASTNode *for_stmt = create_node(AST_FOR, NULL);

        // Initialization (can be variable declaration or expression)
        if (match(TOK_INT) || match(TOK_CHAR)) {
            next();
            bool is_pointer = false;
            if (match(TOK_STAR)) {
                is_pointer = true;
                next();
            }
            Token *name = expect(TOK_IDENT);
            ASTNode *vardecl = create_node(AST_VARDECL, name->value);
            if (is_pointer) {
                char *ptr_name = malloc(strlen(name->value) + 2);
                sprintf(ptr_name, "*%s", name->value);
                free(vardecl->value);
                vardecl->value = ptr_name;
            }
            if (match(TOK_ASSIGN)) {
                next();
                add_child(vardecl, parse_expression());
            }
            add_child(for_stmt, vardecl);
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
    ASTNode *expr = create_node(AST_EXPR_STMT, NULL);
    add_child(expr, parse_expression());
    expect(TOK_SEMICOLON);
    return expr;
}

static ASTNode* parse_function() {
    // Return type
    if (!match(TOK_INT) && !match(TOK_CHAR) && !match(TOK_VOID)) {
        return NULL;
    }
    next();

    Token *name = expect(TOK_IDENT);
    ASTNode *func = create_node(AST_FUNCTION, name->value);

    expect(TOK_LPAREN);
    // Parse parameters (simplified - just skip them for now)
    while (!match(TOK_RPAREN) && !match(TOK_EOF)) {
        next();
    }
    expect(TOK_RPAREN);

    add_child(func, parse_block());
    return func;
}

ASTNode* parse(Token *tokens, int token_count) {
    current = tokens;
    ASTNode *program = create_node(AST_PROGRAM, NULL);

    while (!match(TOK_EOF)) {
        ASTNode *func = parse_function();
        if (func) {
            add_child(program, func);
        } else {
            break;
        }
    }

    return program;
}
