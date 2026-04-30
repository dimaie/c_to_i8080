#include "c_to_i8080.h"

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
    if (match(TOK_ASSIGN) || match(TOK_PLUS_ASSIGN) || match(TOK_MINUS_ASSIGN) || match(TOK_STAR_ASSIGN) || match(TOK_SLASH_ASSIGN)) {
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

static Token* parse_declarator(bool *is_pointer, bool *is_func_ptr) {
    Token *name = NULL;
    *is_pointer = false;
    if (is_func_ptr) *is_func_ptr = false;
    
    if (match(TOK_STAR)) {
        *is_pointer = true;
        next();
        name = expect(TOK_IDENT);
    } else if (match(TOK_LPAREN)) {
        next(); // '('
        if (match(TOK_STAR)) {
            *is_pointer = true;
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
        bool is_pointer = false;
        Token *name = parse_declarator(&is_pointer, NULL);
        
        ASTNode *vardecl = create_node(AST_VARDECL, name->value);
        vardecl->datatype = target_16bit ? 2 : 1;
        vardecl->is_reg = is_reg;
        vardecl->is_static = is_static;
        // Store pointer info in the node value (hacky but simple)
        if (is_pointer) {
            char *ptr_name = malloc(strlen(name->value) + 2);
            sprintf(ptr_name, "*%s", name->value);
            free(vardecl->value);
            vardecl->value = ptr_name;
        }
        if (match(TOK_LBRACKET)) {
            next();
            Token *num = expect(TOK_NUMBER);
            vardecl->array_size = atoi(num->value);
            expect(TOK_RBRACKET);
        }
        if (match(TOK_ASSIGN)) {
            next();
            ASTNode *init_expr = parse_expression();
            if (is_static) {
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
            bool is_pointer = false;
            Token *name = parse_declarator(&is_pointer, NULL);
            
            ASTNode *vardecl = create_node(AST_VARDECL, name->value);
            vardecl->datatype = target_16bit ? 2 : 1;
            vardecl->is_reg = is_for_reg;
            vardecl->is_static = is_for_static;
            if (is_pointer) {
                char *ptr_name = malloc(strlen(name->value) + 2);
                sprintf(ptr_name, "*%s", name->value);
                free(vardecl->value);
                vardecl->value = ptr_name;
            }
            if (match(TOK_LBRACKET)) {
                next();
                Token *num = expect(TOK_NUMBER);
                vardecl->array_size = atoi(num->value);
                expect(TOK_RBRACKET);
            }
            if (match(TOK_ASSIGN)) {
                next();
                ASTNode *init_expr = parse_expression();
                if (is_for_static) {
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
    
    bool is_pointer = false;
    bool is_func_ptr = false;
    Token *name = parse_declarator(&is_pointer, &is_func_ptr);

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
                bool p_is_pointer = false;
                Token *pname = parse_declarator(&p_is_pointer, NULL);
                
                ASTNode *param = create_node(AST_VARDECL, pname->value);
                param->datatype = p_target_16bit ? 2 : 1;
                param->is_reg = is_param_reg;
                if (p_is_pointer) {
                    char *ptr_name = malloc(strlen(pname->value) + 2);
                    sprintf(ptr_name, "*%s", pname->value);
                    free(param->value);
                    param->value = ptr_name;
                }
                if (match(TOK_LBRACKET)) {
                    next();
                    if (match(TOK_NUMBER)) next(); // Skip optional size
                    expect(TOK_RBRACKET);
                    if (!p_is_pointer) {
                        char *ptr_name = malloc(strlen(pname->value) + 2);
                        sprintf(ptr_name, "*%s", pname->value);
                        free(param->value);
                        param->value = ptr_name;
                    }
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
        if (is_pointer) {
            char *ptr_name = malloc(strlen(name->value) + 2);
            sprintf(ptr_name, "*%s", name->value);
            free(vardecl->value);
            vardecl->value = ptr_name;
        }
        if (match(TOK_LBRACKET)) {
            next();
            Token *num = expect(TOK_NUMBER);
            vardecl->array_size = atoi(num->value);
            expect(TOK_RBRACKET);
        }
        if (match(TOK_ASSIGN)) {
            next();
            ASTNode *init_expr = parse_expression();
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
                fprintf(stderr, "Error: Global variables must be initialized with constant numbers or function names at line %d\n", peek()->line);
                exit(1);
            }
            free_ast(init_expr); // Handled at compile time
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
