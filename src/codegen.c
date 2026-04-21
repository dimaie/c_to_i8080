#include "c_to_i8080.h"

static Compiler *compiler;

Symbol* add_symbol(SymbolTable *symtab, const char *name, bool is_global, bool is_pointer) {
    Symbol *sym = malloc(sizeof(Symbol));
    sym->name = strdup(name);
    sym->is_global = is_global;
    sym->is_pointer = is_pointer;
    if (!is_global) {
        // Allocate memory address for local variable
        sym->address = symtab->next_address;
        symtab->next_address += 2;  // Assuming 16-bit values (2 bytes)
    } else {
        sym->address = 0;
    }
    sym->next = symtab->symbols;
    symtab->symbols = sym;
    return sym;
}

Symbol* find_symbol(SymbolTable *symtab, const char *name) {
    Symbol *sym = symtab->symbols;
    while (sym) {
        if (strcmp(sym->name, name) == 0) return sym;
        sym = sym->next;
    }
    return NULL;
}

static int next_label() {
    return compiler->label_counter++;
}

static void emit(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(compiler->output, fmt, args);
    va_end(args);
}

static void compile_expression(ASTNode *node);

static void compile_binary_op(ASTNode *node) {
    // Compile left operand (result in A)
    compile_expression(node->children[0]);

    // Save A to stack
    emit("\tPUSH PSW\n");

    // Compile right operand (result in A)
    compile_expression(node->children[1]);

    // Move A to B
    emit("\tMOV B, A\n");

    // Pop left operand back to A
    emit("\tPOP PSW\n");

    const char *op = node->value;
    if (strcmp(op, "+") == 0) {
        emit("\tADD B\n");
    } else if (strcmp(op, "-") == 0) {
        emit("\tSUB B\n");
    } else if (strcmp(op, "*") == 0) {
        // Simplified multiplication (8-bit)
        compiler->uses_mul = true;
        emit("\tCALL __mul\n");
    } else if (strcmp(op, "/") == 0) {
        compiler->uses_div = true;
        emit("\tCALL __div\n");
    } else if (strcmp(op, "==") == 0) {
        emit("\tSUB B\n");
        emit("\tJNZ L%d\n", next_label());
        emit("\tMVI A, 1\n");
        emit("\tJMP L%d\n", next_label());
        emit("L%d:\n", compiler->label_counter - 2);
        emit("\tMVI A, 0\n");
        emit("L%d:\n", compiler->label_counter - 1);
    } else if (strcmp(op, "!=") == 0) {
        emit("\tSUB B\n");
        emit("\tJZ L%d\n", next_label());
        emit("\tMVI A, 1\n");
        emit("\tJMP L%d\n", next_label());
        emit("L%d:\n", compiler->label_counter - 2);
        emit("\tMVI A, 0\n");
        emit("L%d:\n", compiler->label_counter - 1);
    } else if (strcmp(op, "<") == 0) {
        emit("\tSUB B\n");
        emit("\tJM L%d\n", next_label());
        emit("\tMVI A, 0\n");
        emit("\tJMP L%d\n", next_label());
        emit("L%d:\n", compiler->label_counter - 2);
        emit("\tMVI A, 1\n");
        emit("L%d:\n", compiler->label_counter - 1);
    }
}

static void compile_unary_op(ASTNode *node) {
    compile_expression(node->children[0]);

    const char *op = node->value;
    if (strcmp(op, "-") == 0) {
        emit("\tCMA\n");
        emit("\tINR A\n");  // Two's complement
    } else if (strcmp(op, "!") == 0) {
        emit("\tORA A\n");
        emit("\tJNZ L%d\n", next_label());
        emit("\tMVI A, 1\n");
        emit("\tJMP L%d\n", next_label());
        emit("L%d:\n", compiler->label_counter - 2);
        emit("\tMVI A, 0\n");
        emit("L%d:\n", compiler->label_counter - 1);
    } else if (strcmp(op, "~") == 0) {
        emit("\tCMA\n");
    }
}

static void compile_expression(ASTNode *node) {
    switch (node->type) {
        case AST_NUMBER:
            emit("\tMVI A, %s\n", node->value);
            break;

        case AST_IDENT: {
            Symbol *sym = find_symbol(compiler->symtab, node->value);
            if (sym) {
                if (sym->is_global) {
                    emit("\tLDA %s\n", node->value);
                } else {
                    // Load from direct memory address
                    emit("\tLDA __VAR_%s\n", node->value);
                }
            }
            break;
        }

        case AST_BINARY_OP:
            compile_binary_op(node);
            break;

        case AST_UNARY_OP:
            compile_unary_op(node);
            break;

        case AST_ASSIGN: {
            // Compile right side
            compile_expression(node->children[1]);

            // Store to variable or through pointer
            ASTNode *lhs = node->children[0];
            if (lhs->type == AST_DEREF) {
                // Assignment through pointer: *ptr = value
                emit("\tMOV B, A\n");  // Save value in B
                // Compile pointer expression
                compile_expression(lhs->children[0]);
                // A now contains the address
                emit("\tMOV L, A\n");
                emit("\tMVI H, 0\n");  // Assuming 8-bit addresses
                emit("\tMOV M, B\n");  // Store value to memory
                emit("\tMOV A, B\n");  // Restore value to A
            } else if (lhs->type == AST_IDENT) {
                Symbol *sym = find_symbol(compiler->symtab, lhs->value);
                if (sym) {
                    if (sym->is_global) {
                        emit("\tSTA %s\n", lhs->value);
                    } else {
                        // Store to direct memory address
                        emit("\tSTA __VAR_%s\n", lhs->value);
                    }
                }
            }
            break;
        }

        case AST_CALL:
            // Push arguments (right to left)
            for (int i = node->child_count - 1; i >= 0; i--) {
                compile_expression(node->children[i]);
                emit("\tPUSH PSW\n");
            }
            emit("\tCALL %s\n", node->value);
            // Pop arguments
            if (node->child_count > 0) {
                emit("\tLXI H, %d\n", node->child_count * 2);
                emit("\tDAD SP\n");
                emit("\tSPHL\n");
            }
            break;

        case AST_DEREF: {
            // Dereference: *ptr
            // Compile the pointer expression to get the address
            compile_expression(node->children[0]);
            // Now A contains the low byte of the address
            // For 16-bit addresses, we need to handle this properly
            // Load from memory at address in HL
            emit("\tMOV L, A\n");
            emit("\tMVI H, 0\n");  // Assuming 8-bit addresses for simplicity
            emit("\tMOV A, M\n");  // Load value from memory
            break;
        }

        case AST_ADDROF: {
            // Address-of: &var
            ASTNode *var = node->children[0];
            if (var->type == AST_IDENT) {
                Symbol *sym = find_symbol(compiler->symtab, var->value);
                if (sym) {
                    if (sym->is_global) {
                        // Load address of global variable
                        emit("\tLXI H, %s\n", var->value);
                        emit("\tMOV A, L\n");
                    } else {
                        // Load address of local variable
                        emit("\tLXI H, __VAR_%s\n", var->value);
                        emit("\tMOV A, L\n");  // Return low byte of address
                    }
                }
            }
            break;
        }

        default:
            break;
    }
}

static void compile_statement(ASTNode *node) {
    switch (node->type) {
        case AST_VARDECL: {
            // Check if this is a pointer declaration (value starts with *)
            bool is_pointer = (node->value[0] == '*');
            const char *var_name = is_pointer ? node->value + 1 : node->value;
            add_symbol(compiler->symtab, var_name, false, is_pointer);
            if (node->child_count > 0) {
                // Initialize variable - compile expression and store
                compile_expression(node->children[0]);
                emit("\tSTA __VAR_%s\n", var_name);
            }
            break;
        }

        case AST_RETURN:
            if (node->child_count > 0) {
                compile_expression(node->children[0]);
            }
            emit("\tRET\n");
            break;

        case AST_IF: {
            int label_else = next_label();
            int label_end = next_label();

            // Compile condition
            compile_expression(node->children[0]);
            emit("\tORA A\n");
            emit("\tJZ L%d\n", label_else);

            // Compile then block
            compile_statement(node->children[1]);
            emit("\tJMP L%d\n", label_end);

            // Else block
            emit("L%d:\n", label_else);
            if (node->child_count > 2) {
                compile_statement(node->children[2]);
            }
            emit("L%d:\n", label_end);
            break;
        }

        case AST_WHILE: {
            int label_start = next_label();
            int label_end = next_label();

            emit("L%d:\n", label_start);
            compile_expression(node->children[0]);
            emit("\tORA A\n");
            emit("\tJZ L%d\n", label_end);

            compile_statement(node->children[1]);
            emit("\tJMP L%d\n", label_start);
            emit("L%d:\n", label_end);
            break;
        }

        case AST_DO_WHILE: {
            int label_start = next_label();
            int label_end = next_label();

            emit("L%d:\n", label_start);
            // Execute body first
            compile_statement(node->children[0]);

            // Then check condition
            compile_expression(node->children[1]);
            emit("\tORA A\n");
            emit("\tJNZ L%d\n", label_start);  // Jump back if condition is true
            emit("L%d:\n", label_end);
            break;
        }

        case AST_FOR: {
            int label_start = next_label();
            int label_end = next_label();
            int label_increment = next_label();

            // Initialization (child 0)
            compile_statement(node->children[0]);

            // Loop start
            emit("L%d:\n", label_start);

            // Condition (child 1)
            compile_expression(node->children[1]);
            emit("\tORA A\n");
            emit("\tJZ L%d\n", label_end);

            // Body (child 3)
            compile_statement(node->children[3]);

            // Increment (child 2)
            emit("L%d:\n", label_increment);
            if (node->children[2]->type != AST_EXPR_STMT || node->children[2]->child_count > 0) {
                compile_expression(node->children[2]);
            }

            emit("\tJMP L%d\n", label_start);
            emit("L%d:\n", label_end);
            break;
        }

        case AST_BLOCK:
            for (int i = 0; i < node->child_count; i++) {
                compile_statement(node->children[i]);
            }
            break;

        case AST_EXPR_STMT:
            if (node->child_count > 0) {
                compile_expression(node->children[0]);
            }
            break;

        case AST_ASM:
            // Emit raw assembly code
            emit("\t; Inline assembly\n");
            emit("%s\n", node->value);
            break;

        default:
            break;
    }
}

static void compile_function(ASTNode *node) {
    compiler->current_function = node->value;
    // Reset symbol table for this function
    compiler->symtab->next_address = 0x8000;  // Start local vars at 0x8000
    compiler->symtab->symbols = NULL;

    emit("\n%s:\n", node->value);

    // Compile function body
    for (int i = 0; i < node->child_count; i++) {
        compile_statement(node->children[i]);
    }

    // Emit variable storage declarations for this function
    emit("\n; Local variables for %s\n", node->value);
    Symbol *sym = compiler->symtab->symbols;
    while (sym) {
        if (!sym->is_global) {
            emit("__VAR_%s:\tDS 2\t; %s\n", sym->name, sym->is_pointer ? "pointer" : "variable");
        }
        sym = sym->next;
    }
}

void compile_to_i8080(ASTNode *ast, FILE *output) {
    compiler = malloc(sizeof(Compiler));
    compiler->output = output;
    compiler->label_counter = 0;
    compiler->symtab = malloc(sizeof(SymbolTable));
    compiler->symtab->symbols = NULL;
    compiler->symtab->next_address = 0x8000;  // Start local variables at 0x8000
    compiler->uses_mul = false;
    compiler->uses_div = false;

    emit("; Generated i8080 assembly code\n");
    emit("; Compiled from C source\n\n");

    emit("\tORG 0100H\n\n");

    // Entry point
    emit("\t; Entry point\n");
    emit("\tLXI SP, STACK_TOP\t; Initialize stack pointer\n");
    emit("\tCALL main\n");
    emit("\tHLT\n\n");

    // Compile all functions
    for (int i = 0; i < ast->child_count; i++) {
        if (ast->children[i]->type == AST_FUNCTION) {
            compile_function(ast->children[i]);
        }
    }

    // Runtime support functions (only emit if used)
    if (compiler->uses_mul || compiler->uses_div) {
        emit("\n; Runtime support functions\n");
    }

    if (compiler->uses_mul) {
        emit("__mul:\n");
        emit("\t; Multiply A * B, result in A (8-bit)\n");
        emit("\tMVI C, 0\n");
        emit("\tMVI D, 8\n");
        emit("__mul_loop:\n");
        emit("\tRAR\n");
        emit("\tJNC __mul_skip\n");
        emit("\tMOV E, A\n");
        emit("\tMOV A, C\n");
        emit("\tADD B\n");
        emit("\tMOV C, A\n");
        emit("\tMOV A, E\n");
        emit("__mul_skip:\n");
        emit("\tDCR D\n");
        emit("\tJNZ __mul_loop\n");
        emit("\tMOV A, C\n");
        emit("\tRET\n\n");
    }

    if (compiler->uses_div) {
        emit("__div:\n");
        emit("\t; Divide A / B, result in A (8-bit)\n");
        emit("\tMVI C, 0\n");
        emit("\tMVI D, 8\n");
        emit("__div_loop:\n");
        emit("\tRAL\n");
        emit("\tMOV E, A\n");
        emit("\tMOV A, C\n");
        emit("\tRAL\n");
        emit("\tSUB B\n");
        emit("\tJM __div_skip\n");
        emit("\tMOV C, A\n");
        emit("\tMOV A, E\n");
        emit("\tORI 1\n");
        emit("\tMOV E, A\n");
        emit("__div_skip:\n");
        emit("\tMOV A, E\n");
        emit("\tDCR D\n");
        emit("\tJNZ __div_loop\n");
        emit("\tRET\n\n");
    }

    emit("; Stack space (still needed for CALL/RET and temporary values)\n");
    emit("\tORG 0FFFFH\n");
    emit("STACK_TOP:\n");

    free(compiler->symtab);
    free(compiler);
}
