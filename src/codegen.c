#include "c_to_i8080.h"

static Compiler *compiler;

Symbol* add_symbol(SymbolTable *symtab, const char *name, bool is_global, bool is_pointer, bool is_16bit, bool target_is_16bit, int array_size, bool is_reg, bool is_static, const char *initializer_value) {
    Symbol *sym = malloc(sizeof(Symbol));
    sym->name = strdup(name);
    sym->is_global = is_global;
    sym->is_pointer = is_pointer;
    sym->is_16bit = is_16bit;
    sym->target_is_16bit = target_is_16bit;
    sym->array_size = array_size;
    sym->is_reg = is_reg;
    sym->is_static = is_static;
    sym->initializer_value = initializer_value ? strdup(initializer_value) : NULL;
    int count = array_size > 0 ? array_size : 1;

    if (!is_global && !is_reg && !is_static) {
        // Allocate offset on stack from frame pointer
        symtab->next_address += (is_16bit ? 2 : 1) * count;
        sym->address = -symtab->next_address;
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
static void emit_shadow_stack_pops(Symbol *sym);

static void compile_binary_op(ASTNode *node) {
    // Compile left operand (result in HL)
    compile_expression(node->children[0]);

    // Save HL to stack
    emit("\tPUSH H\n");

    // Compile right operand (result in HL)
    compile_expression(node->children[1]);

    // Pop left operand back to DE
    emit("\tPOP D\n");

    const char *op = node->value;
    if (strcmp(op, "+") == 0) {
        emit("\tDAD D\n");
    } else if (strcmp(op, "-") == 0) {
        emit("\tMOV A, E\n");
        emit("\tSUB L\n");
        emit("\tMOV L, A\n");
        emit("\tMOV A, D\n");
        emit("\tSBB H\n");
        emit("\tMOV H, A\n");
    } else if (strcmp(op, "*") == 0) {
        // Inline hardware multiply! (Replaces slow software __mul)
        emit("\tPUSH B\t; Preserve reg variables\n");
        emit("\tMOV A, E\n");
        emit("\tMOV B, L\n");
        emit("\tMUL B\n");
        emit("\tPOP B\t; Restore reg variables\n");
    } else if (strcmp(op, "/") == 0) {
        compiler->uses_div = true;
        emit("\tCALL __div\n");
    } else if (strcmp(op, "%") == 0) {
        compiler->uses_mod = true;
        emit("\tCALL __mod\n");
    } else if (strcmp(op, "==") == 0) {
        emit("\tMOV A, E\n");
        emit("\tCMP L\n");
        emit("\tJNZ L%d\n", next_label());
        emit("\tMOV A, D\n");
        emit("\tCMP H\n");
        emit("\tJNZ L%d\n", compiler->label_counter - 1);
        emit("\tLXI H, 1\n");
        emit("\tJMP L%d\n", next_label());
        emit("L%d:\n", compiler->label_counter - 2);
        emit("\tLXI H, 0\n");
        emit("L%d:\n", compiler->label_counter - 1);
    } else if (strcmp(op, "!=") == 0) {
        emit("\tMOV A, E\n");
        emit("\tCMP L\n");
        emit("\tJNZ L%d\n", next_label());
        emit("\tMOV A, D\n");
        emit("\tCMP H\n");
        emit("\tJZ L%d\n", next_label());
        emit("L%d:\n", compiler->label_counter - 2);
        emit("\tLXI H, 1\n");
        emit("\tJMP L%d\n", next_label());
        emit("L%d:\n", compiler->label_counter - 2);
        emit("\tLXI H, 0\n");
        emit("L%d:\n", compiler->label_counter - 1);
    } else if (strcmp(op, "<") == 0) {
        emit("\tMOV A, E\n");
        emit("\tSUB L\n");
        emit("\tMOV A, D\n");
        emit("\tSBB H\n");
        emit("\tJM L%d\n", next_label());
        emit("\tLXI H, 0\n");
        emit("\tJMP L%d\n", next_label());
        emit("L%d:\n", compiler->label_counter - 2);
        emit("\tLXI H, 1\n");
        emit("L%d:\n", compiler->label_counter - 1);
    } else if (strcmp(op, "<=") == 0) {
        emit("\tMOV A, L\n");
        emit("\tSUB E\n");
        emit("\tMOV A, H\n");
        emit("\tSBB D\n");
        emit("\tJM L%d\n", next_label());
        emit("\tLXI H, 1\n");
        emit("\tJMP L%d\n", next_label());
        emit("L%d:\n", compiler->label_counter - 2);
        emit("\tLXI H, 0\n");
        emit("L%d:\n", compiler->label_counter - 1);
    } else if (strcmp(op, ">") == 0) {
        emit("\tMOV A, L\n");
        emit("\tSUB E\n");
        emit("\tMOV A, H\n");
        emit("\tSBB D\n");
        emit("\tJM L%d\n", next_label());
        emit("\tLXI H, 0\n");
        emit("\tJMP L%d\n", next_label());
        emit("L%d:\n", compiler->label_counter - 2);
        emit("\tLXI H, 1\n");
        emit("L%d:\n", compiler->label_counter - 1);
    } else if (strcmp(op, ">=") == 0) {
        emit("\tMOV A, E\n");
        emit("\tSUB L\n");
        emit("\tMOV A, D\n");
        emit("\tSBB H\n");
        emit("\tJM L%d\n", next_label());
        emit("\tLXI H, 1\n");
        emit("\tJMP L%d\n", next_label());
        emit("L%d:\n", compiler->label_counter - 2);
        emit("\tLXI H, 0\n");
        emit("L%d:\n", compiler->label_counter - 1);
    } else if (strcmp(op, "&") == 0) {
        emit("\tMOV A, E\n");
        emit("\tANA L\n");
        emit("\tMOV L, A\n");
        emit("\tMOV A, D\n");
        emit("\tANA H\n");
        emit("\tMOV H, A\n");
    } else if (strcmp(op, "|") == 0) {
        emit("\tMOV A, E\n");
        emit("\tORA L\n");
        emit("\tMOV L, A\n");
        emit("\tMOV A, D\n");
        emit("\tORA H\n");
        emit("\tMOV H, A\n");
    } else if (strcmp(op, "^") == 0) {
        emit("\tMOV A, E\n");
        emit("\tXRA L\n");
        emit("\tMOV L, A\n");
        emit("\tMOV A, D\n");
        emit("\tXRA H\n");
        emit("\tMOV H, A\n");
    } else if (strcmp(op, "&&") == 0) {
        int lbl_false = next_label();
        int lbl_end = next_label();
        emit("\tMOV A, D\n");
        emit("\tORA E\n");
        emit("\tJZ L%d\n", lbl_false);
        emit("\tMOV A, H\n");
        emit("\tORA L\n");
        emit("\tJZ L%d\n", lbl_false);
        emit("\tLXI H, 1\n");
        emit("\tJMP L%d\n", lbl_end);
        emit("L%d:\n", lbl_false);
        emit("\tLXI H, 0\n");
        emit("L%d:\n", lbl_end);
    } else if (strcmp(op, "||") == 0) {
        int lbl_true = next_label();
        int lbl_end = next_label();
        emit("\tMOV A, D\n");
        emit("\tORA E\n");
        emit("\tJNZ L%d\n", lbl_true);
        emit("\tMOV A, H\n");
        emit("\tORA L\n");
        emit("\tJNZ L%d\n", lbl_true);
        emit("\tLXI H, 0\n");
        emit("\tJMP L%d\n", lbl_end);
        emit("L%d:\n", lbl_true);
        emit("\tLXI H, 1\n");
        emit("L%d:\n", lbl_end);
    } else if (strcmp(op, "<<") == 0) {
        int lbl_loop = next_label();
        int lbl_end = next_label();
        emit("\tXCHG\n"); // DE = shift count, HL = value
        emit("\tMOV A, E\n");
        emit("\tORA A\n");
        emit("\tJZ L%d\n", lbl_end);
        emit("L%d:\n", lbl_loop);
        emit("\tDAD H\n");
        emit("\tDCR E\n");
        emit("\tJNZ L%d\n", lbl_loop);
        emit("L%d:\n", lbl_end);
    } else if (strcmp(op, ">>") == 0) {
        int lbl_loop = next_label();
        int lbl_end = next_label();
        emit("\tXCHG\n"); // DE = shift count, HL = value
        emit("\tMOV A, E\n");
        emit("\tORA A\n");
        emit("\tJZ L%d\n", lbl_end);
        emit("L%d:\n", lbl_loop);
        emit("\tMOV A, H\n");
        emit("\tRLC\n"); // Carry = MSB
        emit("\tMOV A, H\n");
        emit("\tRAR\n"); // Shift right, keeping MSB
        emit("\tMOV H, A\n");
        emit("\tMOV A, L\n");
        emit("\tRAR\n");
        emit("\tMOV L, A\n");
        emit("\tDCR E\n");
        emit("\tJNZ L%d\n", lbl_loop);
        emit("L%d:\n", lbl_end);
    }
}

static void compile_unary_op(ASTNode *node) {
    compile_expression(node->children[0]);

    const char *op = node->value;
    if (strcmp(op, "-") == 0) {
        emit("\tMOV A, H\n\tCMA\n\tMOV H, A\n");
        emit("\tMOV A, L\n\tCMA\n\tMOV L, A\n");
        emit("\tINX H\n");  // Two's complement
    } else if (strcmp(op, "!") == 0) {
        emit("\tMOV A, H\n");
        emit("\tORA L\n");
        emit("\tJNZ L%d\n", next_label());
        emit("\tLXI H, 1\n");
        emit("\tJMP L%d\n", next_label());
        emit("L%d:\n", compiler->label_counter - 2);
        emit("\tLXI H, 0\n");
        emit("L%d:\n", compiler->label_counter - 1);
    } else if (strcmp(op, "~") == 0) {
        emit("\tMOV A, H\n\tCMA\n\tMOV H, A\n");
        emit("\tMOV A, L\n\tCMA\n\tMOV L, A\n");
    }
}

static bool get_target_is_16bit(ASTNode *expr) {
    if (!expr) return true;
    if (expr->type == AST_IDENT) {
        Symbol *sym = find_symbol(compiler->symtab, expr->value);
        return sym ? sym->target_is_16bit : true;
    }
    if (expr->type == AST_BINARY_OP) {
        return get_target_is_16bit(expr->children[0]);
    }
    return true;
}

static void emit_store_hl_to_lvalue(ASTNode *lhs) {
    if (lhs->type == AST_DEREF) {
        // Assignment through pointer: *ptr = value
        // The value to store is in HL.
        bool target_is_16bit = get_target_is_16bit(lhs->children[0]);
        emit("\tPUSH H\n");  // Save value to be stored
        compile_expression(lhs->children[0]); // Pointer address now in HL
        emit("\tPOP D\n");  // Value to be stored now in DE
        if (target_is_16bit) {
            emit("\tMOV M, E\n");
            emit("\tINX H\n");
            emit("\tMOV M, D\n");
        } else {
            emit("\tMOV M, E\n");
        }
        emit("\tXCHG\n"); // Restore value back to HL
    } else if (lhs->type == AST_ARRAY_ACCESS) {
        Symbol *sym = find_symbol(compiler->symtab, lhs->value);
        if (sym) {
            emit("\tPUSH H\n"); // Save value to be stored
            compile_expression(lhs->children[0]); // Index to HL
            if (sym->target_is_16bit) emit("\tDAD H\n");
            emit("\tPUSH H\n"); // Save offset
            
            // Get base address
            if (sym->array_size > 0) {
                if (sym->is_global) {
                    emit("\tLXI H, %s\n", lhs->value);
                } else {
                    if (compiler->use_frame_pointer && !sym->is_static) {
                        emit("\tLHLD __FP\n");
                        emit("\tLXI D, %d\n", sym->address);
                        emit("\tDAD D\n");
                    } else {
                        emit("\tLXI H, __VAR_%s_%s\n", compiler->current_function, lhs->value);
                    }
                }
            } else { // Pointer fallback
                if (sym->is_global) {
                    emit("\tLHLD %s\n", lhs->value);
                } else {
                    if (compiler->use_frame_pointer && !sym->is_static) {
                        emit("\tLHLD __FP\n");
                        emit("\tLXI D, %d\n", sym->address);
                        emit("\tDAD D\n");
                        emit("\tMOV E, M\n\tINX H\n\tMOV D, M\n\tXCHG\n");
                    } else {
                        emit("\tLHLD __VAR_%s_%s\n", compiler->current_function, lhs->value);
                    }
                }
            }
            
            emit("\tPOP D\n"); emit("\tDAD D\n"); // Base + Offset
            emit("\tPOP D\n"); // Value to store in DE
            
            if (sym->target_is_16bit) {
                emit("\tMOV M, E\n\tINX H\n\tMOV M, D\n\tXCHG\n");
            } else {
                emit("\tMOV M, E\n\tXCHG\n");
            }
        }
    } else if (lhs->type == AST_IDENT) {
        Symbol *sym = find_symbol(compiler->symtab, lhs->value);
        if (sym) {
            if (sym->is_16bit) {
                if (sym->is_global) {
                    emit("\tSHLD %s\n", lhs->value);
                } else {
                    if (sym->is_reg) {
                        emit("\tMOV B, H\n\tMOV C, L\n");
                    } else {
                        if (compiler->use_frame_pointer && !sym->is_static) {
                            emit("\tPUSH H\t; Save assigned value\n");
                            emit("\tLHLD __FP\n");
                            emit("\tLXI D, %d\n", sym->address);
                            emit("\tDAD D\n");
                            emit("\tPOP D\n");
                            emit("\tMOV M, E\n");
                            emit("\tINX H\n");
                            emit("\tMOV M, D\n");
                            emit("\tXCHG\t; Restore to %s\n", lhs->value);
                        } else {
                            emit("\tSHLD __VAR_%s_%s\n", compiler->current_function, lhs->value);
                        }
                    }
                }
            } else { // 8-bit assignment
                if (sym->is_global) {
                    emit("\tMOV A, L\n");
                    emit("\tSTA %s\n", lhs->value);
                } else {
                    if (sym->is_reg) {
                        emit("\tMOV C, L\n\tMVI B, 0\n");
                    } else {
                        if (compiler->use_frame_pointer && !sym->is_static) {
                            emit("\tPUSH H\n");
                            emit("\tLHLD __FP\n");
                            emit("\tLXI D, %d\n", sym->address);
                            emit("\tDAD D\n");
                            emit("\tPOP D\n");
                            emit("\tMOV M, E\n");
                            emit("\tXCHG\n");
                        } else {
                            emit("\tMOV A, L\n");
                            emit("\tSTA __VAR_%s_%s\n", compiler->current_function, lhs->value);
                        }
                    }
                }
            }
        }
    }
}

static void compile_expression(ASTNode *node) {
    switch (node->type) {
        case AST_NUMBER:
            emit("\tLXI H, %s\n", node->value);
            break;

        case AST_IDENT: {
            Symbol *sym = find_symbol(compiler->symtab, node->value);
            if (sym) {
                if (sym->array_size > 0) {
                    // Array decays to pointer (load base address into HL)
                    if (sym->is_global) {
                        emit("\tLXI H, %s\n", node->value);
                    } else {
                        if (compiler->use_frame_pointer && !sym->is_static) {
                            emit("\tLHLD __FP\n");
                            emit("\tLXI D, %d\n", sym->address);
                            emit("\tDAD D\n");
                        } else {
                            emit("\tLXI H, __VAR_%s_%s\n", compiler->current_function, node->value);
                        }
                    }
                } else if (sym->is_16bit) {
                    if (sym->is_global) {
                        emit("\tLHLD %s\n", node->value);
                    } else {
                        if (sym->is_reg) {
                            if (sym->is_16bit) {
                                emit("\tMOV H, B\n\tMOV L, C\n");
                            } else {
                                emit("\tMOV L, C\n\tMVI H, 0\n");
                            }
                        } else {
                        if (compiler->use_frame_pointer && !sym->is_static) {
                            emit("\tLHLD __FP\n");
                            emit("\tLXI D, %d\n", sym->address);
                            emit("\tDAD D\n");
                            emit("\tMOV E, M\n");
                            emit("\tINX H\n");
                            emit("\tMOV D, M\n");
                            emit("\tXCHG\t; Load %s\n", node->value);
                        } else {
                            emit("\tLHLD __VAR_%s_%s\n", compiler->current_function, node->value);
                        }
                        }
                    }
                } else { // 8-bit read
                    if (sym->is_global) {
                        emit("\tLDA %s\n", node->value);
                    } else {
                        if (sym->is_reg) {
                            emit("\tMOV L, C\n\tMVI H, 0\n");
                        } else {
                        if (compiler->use_frame_pointer && !sym->is_static) {
                            emit("\tLHLD __FP\n");
                            emit("\tLXI D, %d\n", sym->address);
                            emit("\tDAD D\n");
                            emit("\tMOV A, M\t; Load %s\n", node->value);
                        } else {
                            emit("\tLDA __VAR_%s_%s\n", compiler->current_function, node->value);
                        }
                        }
                    }
                    emit("\tMOV L, A\n");
                    emit("\tMVI H, 0\n"); // Zero extend
                }
            } else {
                // Treat undefined symbols as function labels/addresses
                emit("\tLXI H, %s\n", node->value);
            }
            break;
        }

        case AST_ARRAY_ACCESS: {
            Symbol *sym = find_symbol(compiler->symtab, node->value);
            if (sym) {
                compile_expression(node->children[0]); // Index to HL
                
                if (sym->target_is_16bit) {
                    emit("\tDAD H\n"); // Index * 2
                }
                emit("\tPUSH H\n"); // Save offset
                
                // Get base address
                if (sym->array_size > 0) {
                    if (sym->is_global) {
                        emit("\tLXI H, %s\n", node->value);
                    } else {
                        if (compiler->use_frame_pointer && !sym->is_static) {
                            emit("\tLHLD __FP\n");
                            emit("\tLXI D, %d\n", sym->address);
                            emit("\tDAD D\n");
                        } else {
                            emit("\tLXI H, __VAR_%s_%s\n", compiler->current_function, node->value);
                        }
                    }
                } else { // Pointer fallback
                    if (sym->is_global) {
                        emit("\tLHLD %s\n", node->value);
                    } else {
                        if (compiler->use_frame_pointer && !sym->is_static) {
                            emit("\tLHLD __FP\n");
                            emit("\tLXI D, %d\n", sym->address);
                            emit("\tDAD D\n");
                            emit("\tMOV E, M\n\tINX H\n\tMOV D, M\n\tXCHG\n");
                        } else {
                            emit("\tLHLD __VAR_%s_%s\n", compiler->current_function, node->value);
                        }
                    }
                }
                
                emit("\tPOP D\n"); // Offset in DE
                emit("\tDAD D\n"); // Base + Offset in HL
                
                if (sym->target_is_16bit) {
                    emit("\tMOV E, M\n\tINX H\n\tMOV D, M\n\tXCHG\n");
                } else {
                    emit("\tMOV L, M\n\tMVI H, 0\n");
                }
            }
            break;
        }

        case AST_STRING: {
            int lbl_str = next_label();
            int lbl_skip = next_label();
            emit("\tJMP L%d\n", lbl_skip);
            emit("L%d:\n", lbl_str);
            emit("\tDB ");
            for (int i = 0; node->value[i]; i++) {
                emit("%d,", (unsigned char)node->value[i]);
            }
            emit("0\n"); // Null terminator
            emit("L%d:\n", lbl_skip);
            emit("\tLXI H, L%d\n", lbl_str); // Load string pointer to HL
            break;
        }

        case AST_BINARY_OP:
            compile_binary_op(node);
            break;

        case AST_UNARY_OP:
            compile_unary_op(node);
            break;

        case AST_ASSIGN: {
            // Compile right side, result in HL
            compile_expression(node->children[1]);
            // Store HL to the l-value on the left side
            emit_store_hl_to_lvalue(node->children[0]);
            break;
        }

        case AST_COMPOUND_ASSIGN: {
            ASTNode *lhs = node->children[0];
            ASTNode *rhs = node->children[1];

            compile_expression(rhs);
            emit("\tPUSH H\n");
            compile_expression(lhs);
            emit("\tPOP D\n");

            const char *op = node->value;
            if (strcmp(op, "+") == 0) {
                emit("\tDAD D\n");
            } else if (strcmp(op, "-") == 0) {
                emit("\tMOV A, L\n\tSUB E\n\tMOV L, A\n");
                emit("\tMOV A, H\n\tSBB D\n\tMOV H, A\n");
            } else if (strcmp(op, "*") == 0) {
                emit("\tPUSH B\n");
                emit("\tMOV A, L\n");
                emit("\tMOV B, E\n");
                emit("\tMUL B\n");
                emit("\tPOP B\n");
            } else if (strcmp(op, "/") == 0) {
                emit("\tXCHG\n");
                compiler->uses_div = true;
                emit("\tCALL __div\n");
            }

            emit_store_hl_to_lvalue(lhs);
            break;
        }

        case AST_PRE_INC:
        case AST_PRE_DEC: {
            ASTNode *operand = node->children[0];
            compile_expression(operand);
            if (node->type == AST_PRE_INC) {
                emit("\tINX H\n");
            } else {
                emit("\tDCX H\n");
            }
            emit_store_hl_to_lvalue(operand);
            break;
        }

        case AST_POST_INC:
        case AST_POST_DEC: {
            ASTNode *operand = node->children[0];
            compile_expression(operand);
            emit("\tPUSH H\t; Save original value for postfix op\n");
            if (node->type == AST_POST_INC) {
                emit("\tINX H\n");
            } else {
                emit("\tDCX H\n");
            }
            emit_store_hl_to_lvalue(operand);
            emit("\tPOP H\t; Restore original value as expression result\n");
            break;
        }

        case AST_CALL:
            // Push arguments (right to left)
            for (int i = node->child_count - 1; i >= 0; i--) {
                compile_expression(node->children[i]);
                emit("\tPUSH H\n");
            }
            
            Symbol *sym = find_symbol(compiler->symtab, node->value);
            if (sym) {
                // Indirect call through variable
                if (sym->is_global) {
                    emit("\tLHLD %s\n", node->value);
                } else {
                    if (sym->is_reg) {
                        if (sym->is_16bit) {
                            emit("\tMOV H, B\n\tMOV L, C\n");
                        } else {
                            emit("\tMOV L, C\n\tMVI H, 0\n");
                        }
                    } else {
                        if (compiler->use_frame_pointer && !sym->is_static) {
                            emit("\tLHLD __FP\n");
                            emit("\tLXI D, %d\n", sym->address);
                            emit("\tDAD D\n");
                            emit("\tMOV E, M\n\tINX H\n\tMOV D, M\n\tXCHG\n");
                        } else {
                            emit("\tLHLD __VAR_%s_%s\n", compiler->current_function, node->value);
                        }
                    }
                }
                compiler->uses_icall = true;
                emit("\tCALL __icall\n");
            } else {
                emit("\tCALL %s\n", node->value);
            }
            
            // Pop arguments
            if (node->child_count > 0) {
                if (node->child_count <= 4) {
                    emit("\tXCHG\t; Save Return Value\n");
                    for (int i = 0; i < node->child_count; i++) {
                        emit("\tPOP H\t; Discard argument\n");
                    }
                    emit("\tXCHG\t; Restore Return Value\n");
                } else {
                    emit("\tXCHG\t; Save Return Value\n");
                    emit("\tLXI H, %d\n", node->child_count * 2);
                    emit("\tDAD SP\n");
                    emit("\tSPHL\n");
                    emit("\tXCHG\t; Restore Return Value\n");
                }
            }
            break;

        case AST_INDIRECT_CALL: {
            // Push arguments (child 1 to n)
            for (int i = node->child_count - 1; i >= 1; i--) {
                compile_expression(node->children[i]);
                emit("\tPUSH H\n");
            }
            
            // Evaluate function pointer expression
            ASTNode *target = node->children[0];
            if (target->type == AST_DEREF) {
                // In C, dereferencing a function pointer decays back to the pointer itself.
                // We bypass the memory read of AST_DEREF and just evaluate the pointer address.
                compile_expression(target->children[0]);
            } else {
                compile_expression(target);
            }
            
            compiler->uses_icall = true;
            emit("\tCALL __icall\n");
            
            // Pop arguments
            int arg_count = node->child_count - 1;
            if (arg_count > 0) {
                if (arg_count <= 4) {
                    emit("\tXCHG\t; Save Return Value\n");
                    for (int i = 0; i < arg_count; i++) {
                        emit("\tPOP H\t; Discard argument\n");
                    }
                    emit("\tXCHG\t; Restore Return Value\n");
                } else {
                    emit("\tXCHG\t; Save Return Value\n");
                    emit("\tLXI H, %d\n", arg_count * 2);
                    emit("\tDAD SP\n");
                    emit("\tSPHL\n");
                    emit("\tXCHG\t; Restore Return Value\n");
                }
            }
            break;
        }

        case AST_DEREF: {
            compile_expression(node->children[0]);
            bool target_is_16bit = get_target_is_16bit(node->children[0]);
            if (target_is_16bit) {
                emit("\tMOV E, M\n");
                emit("\tINX H\n");
                emit("\tMOV D, M\n");
                emit("\tXCHG\n");
            } else {
                emit("\tMOV L, M\n");
                emit("\tMVI H, 0\n");
            }
            break;
        }

        case AST_ADDROF: {
            // Address-of: &var
            ASTNode *var = node->children[0];
            if (var->type == AST_IDENT) {
                Symbol *sym = find_symbol(compiler->symtab, var->value);
                if (sym) {
                    if (sym->is_reg) {
                        fprintf(stderr, "Error: Cannot take address of register variable '%s'\n", var->value);
                        exit(1);
                    }
                    if (sym->is_global) {
                        emit("\tLXI H, %s\n", var->value);
                    } else {
                        if (compiler->use_frame_pointer && !sym->is_static) {
                            emit("\tLHLD __FP\n");
                            emit("\tLXI D, %d\n", sym->address);
                            emit("\tDAD D\n");
                        } else {
                            emit("\tLXI H, __VAR_%s_%s\n", compiler->current_function, var->value);
                        }
                    }
                } else {
                    // Treat undefined symbols as function labels/addresses
                    emit("\tLXI H, %s\n", var->value);
                }
            } else {
                fprintf(stderr, "Error: Cannot take address of a non-variable\n");
                exit(1);
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
            Symbol *sym = find_symbol(compiler->symtab, var_name);
            // Do not generate runtime initialization code for static variables.
            if (node->child_count > 0 && sym->array_size == 0 && !sym->is_static) {
                compile_expression(node->children[0]);
                if (sym->is_16bit) {
                    if (compiler->use_frame_pointer && !sym->is_static) {
                        emit("\tPUSH H\t; Save init value\n");
                        emit("\tLHLD __FP\n");
                        emit("\tLXI D, %d\n", sym->address);
                        emit("\tDAD D\n");
                        emit("\tPOP D\n");
                        emit("\tMOV M, E\n");
                        emit("\tINX H\n");
                        emit("\tMOV M, D\n");
                    } else if (sym->is_reg) {
                        if (sym->is_16bit) {
                            emit("\tMOV B, H\n\tMOV C, L\n");
                        } else {
                            emit("\tMOV C, L\n\tMVI B, 0\n");
                        }
                    } else {
                        emit("\tSHLD __VAR_%s_%s\n", compiler->current_function, var_name);
                    }
                } else {
                    if (compiler->use_frame_pointer && !sym->is_static) {
                        emit("\tPUSH H\t; Save init value\n");
                        emit("\tLHLD __FP\n");
                        emit("\tLXI D, %d\n", sym->address);
                        emit("\tDAD D\n");
                        emit("\tPOP D\n");
                        emit("\tMOV M, E\n"); // Store low byte
                    } else if (sym->is_reg) {
                        emit("\tMOV C, L\n\tMVI B, 0\n");
                    } else {
                        emit("\tMOV A, L\n");
                        emit("\tSTA __VAR_%s_%s\n", compiler->current_function, var_name);
                    }
                }
            }
            break;
        }

        case AST_RETURN:
            if (node->child_count > 0) {
                compile_expression(node->children[0]);
            }
            if (compiler->uses_bc) {
                emit("\tPOP B\t; Restore callee-saved register for 'reg'\n");
            }
            // Epilogue
            emit("\tXCHG\t; Save return value\n");
            if (compiler->use_frame_pointer) {
                emit("\tLHLD __FP\n");
                emit("\tSPHL\t\t; Restore SP\n");
                emit("\tPOP H\t\t; Get old FP\n");
                emit("\tSHLD __FP\t; Restore FP\n");
            } else {
                emit_shadow_stack_pops(compiler->symtab->symbols);
            }
            emit("\tXCHG\t; Restore return value\n");
            emit("\tRET\n");
            break;

        case AST_IF: {
            int label_else = next_label();
            int label_end = next_label();

            // Compile condition
            compile_expression(node->children[0]);
            emit("\tMOV A, H\n");
            emit("\tORA L\n");
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
            emit("\tMOV A, H\n");
            emit("\tORA L\n");
            emit("\tJZ L%d\n", label_end);

            int old_break = compiler->current_break_label;
            int old_continue = compiler->current_continue_label;
            compiler->current_break_label = label_end;
            compiler->current_continue_label = label_start;
            compile_statement(node->children[1]);
            compiler->current_break_label = old_break;
            compiler->current_continue_label = old_continue;

            emit("\tJMP L%d\n", label_start);
            emit("L%d:\n", label_end);
            break;
        }

        case AST_DO_WHILE: {
            int label_start = next_label();
            int label_end = next_label();
            int label_continue = next_label();

            emit("L%d:\n", label_start);
            // Execute body first
            int old_break = compiler->current_break_label;
            int old_continue = compiler->current_continue_label;
            compiler->current_break_label = label_end;
            compiler->current_continue_label = label_continue;
            compile_statement(node->children[0]);
            compiler->current_break_label = old_break;
            compiler->current_continue_label = old_continue;

            emit("L%d:\n", label_continue);
            // Then check condition
            compile_expression(node->children[1]);
            emit("\tMOV A, H\n");
            emit("\tORA L\n");
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
            emit("\tMOV A, H\n");
            emit("\tORA L\n");
            emit("\tJZ L%d\n", label_end);

            // Body (child 3)
            int old_break = compiler->current_break_label;
            int old_continue = compiler->current_continue_label;
            compiler->current_break_label = label_end;
            compiler->current_continue_label = label_increment;
            compile_statement(node->children[3]);
            compiler->current_break_label = old_break;
            compiler->current_continue_label = old_continue;

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

        case AST_BREAK:
            if (compiler->current_break_label >= 0) {
                emit("\tJMP L%d\n", compiler->current_break_label);
            } else {
                fprintf(stderr, "Error: break statement not within loop\n");
                exit(1);
            }
            break;

        case AST_CONTINUE:
            if (compiler->current_continue_label >= 0) {
                emit("\tJMP L%d\n", compiler->current_continue_label);
            } else {
                fprintf(stderr, "Error: continue statement not within loop\n");
                exit(1);
            }
            break;

        case AST_EXPR_STMT:
            if (node->child_count > 0) {
                compile_expression(node->children[0]);
            }
            break;

        case AST_ASM: {
            // Emit raw assembly code
            emit("\t; Inline assembly\n");
            char *p = node->value;
            while (*p) {
                if (strncmp(p, "__VAR_", 6) == 0) {
                    char var_name[256];
                    int i = 0;
                    const char *q = p + 6;
                    while (isalnum((unsigned char)*q) || *q == '_') {
                        if (i < 255) var_name[i++] = *q;
                        q++;
                    }
                    var_name[i] = '\0';
                    
                    Symbol *sym = find_symbol(compiler->symtab, var_name);
                    if (sym) {
                        emit("__VAR_%s_%s", compiler->current_function, var_name);
                    } else {
                        emit("__VAR_%s", var_name);
                    }
                    p = (char*)q;
                } else {
                    fputc(*p, compiler->output);
                    p++;
                }
            }
            emit("\n");
            break;
        }

        default:
            break;
    }
}

static void collect_local_vars(ASTNode *node) {
    if (!node) return;
    if (node->type == AST_VARDECL) {
        bool is_pointer = (node->value[0] == '*');
        const char *var_name = is_pointer ? node->value + 1 : node->value;
        Symbol *existing = find_symbol(compiler->symtab, var_name);
        if (!existing || existing->is_global) {
            bool allocate_reg = false;
            if (node->is_reg && !compiler->uses_bc && !is_pointer && node->array_size == 0) {
                allocate_reg = true;
                compiler->uses_bc = true;
            }
            bool is_16bit = (node->datatype == 2) || is_pointer;
            bool target_is_16bit = (node->datatype == 2);
            add_symbol(compiler->symtab, var_name, false, is_pointer, is_16bit, target_is_16bit, node->array_size, allocate_reg, node->is_static, node->initializer_value);
        }
    }
    for (int i = 0; i < node->child_count; i++) {
        collect_local_vars(node->children[i]);
    }
}

static void emit_shadow_stack_pops(Symbol *sym) {
    if (!sym) return;
    emit_shadow_stack_pops(sym->next);
    if (sym->is_global || sym->array_size > 0 || sym->is_reg || sym->is_static) return; // Skip statically allocated arrays, registers, statics, and globals
    emit("\tPOP H\t\t; Shadow stack pop\n");
    if (sym->is_16bit) {
        emit("\tSHLD __VAR_%s_%s\n", compiler->current_function, sym->name);
    } else {
        emit("\tMOV A, L\n");
        emit("\tSTA __VAR_%s_%s\n", compiler->current_function, sym->name);
    }
}

static void compile_function(ASTNode *node) {
    compiler->current_function = node->value;
    
    // Save current global symbols head
    Symbol *global_syms = compiler->symtab->symbols;

    // Reset symbol table for this function
    compiler->symtab->next_address = 0;  // Start offsets at 0
    compiler->uses_bc = false;

    collect_local_vars(node);
    int local_space = compiler->symtab->next_address;
    int pushed_symbol_count = 0;
    for (Symbol *s = compiler->symtab->symbols; s; s = s->next) {
        if (!s->is_global && s->array_size == 0 && !s->is_reg && !s->is_static) {
            pushed_symbol_count++;
        }
    }

    emit("\n%s:\n", node->value);

    // Prologue
    if (compiler->use_frame_pointer) {
        emit("\tLHLD __FP\t; Save old frame pointer\n");
        emit("\tPUSH H\n");
        emit("\tLXI H, 0\n");
        emit("\tDAD SP\t\t; HL = current SP\n");
        emit("\tSHLD __FP\t; Set new frame pointer\n");
        if (local_space > 0) {
            emit("\tLXI D, -%d\n", local_space);
            emit("\tDAD D\n");
            emit("\tSPHL\t\t; Allocate space for %d local bytes\n", local_space);
        }
    } else {
        Symbol *sym = compiler->symtab->symbols;
        while (sym) {
            if (!sym->is_global && sym->array_size == 0 && !sym->is_reg && !sym->is_static) {
                emit("\tLHLD __VAR_%s_%s\t; Shadow stack push\n", compiler->current_function, sym->name);
                emit("\tPUSH H\n");
            }
            sym = sym->next;
        }
    }

    // Copy parameters to their local variables
    for (int i = 0; i < node->child_count - 1; i++) {
        ASTNode *param = node->children[i];
        if (param->type == AST_VARDECL) {
            bool is_pointer = (param->value[0] == '*');
            const char *var_name = is_pointer ? param->value + 1 : param->value;
            Symbol *sym = find_symbol(compiler->symtab, var_name);
            
            if (compiler->use_frame_pointer) {
                emit("\t; Load parameter %s from stack\n", var_name);
                emit("\tLHLD __FP\n");
                emit("\tLXI D, %d\n", 4 + i * 2);
                emit("\tDAD D\n");
                emit("\tMOV E, M\n");
                emit("\tINX H\n");
                emit("\tMOV D, M\n");
                
                emit("\tLHLD __FP\n");
                emit("\tLXI B, %d\n", sym->address);
                emit("\tDAD B\n");
                if (sym->is_reg) {
                    emit("\tMOV B, D\n\tMOV C, E\n");
                } else if (sym->is_16bit) {
                    emit("\tMOV M, E\n\tINX H\n\tMOV M, D\n");
                } else {
                    emit("\tMOV M, E\n");
                }
            } else {
                emit("\t; Load parameter %s from hardware stack\n", var_name);
                emit("\tLXI H, %d\n", pushed_symbol_count * 2 + 2 + i * 2);
                emit("\tDAD SP\n");
                emit("\tMOV E, M\n");
                emit("\tINX H\n");
                emit("\tMOV D, M\n");
                emit("\tXCHG\n"); // HL = value
                
                if (sym->is_reg) {
                    emit("\tMOV B, H\n\tMOV C, L\n");
                } else if (sym->is_16bit) {
                    emit("\tSHLD __VAR_%s_%s\n", compiler->current_function, var_name);
                } else {
                    emit("\tMOV A, L\n");
                    emit("\tSTA __VAR_%s_%s\n", compiler->current_function, var_name);
                }
            }
        }
    }

    if (compiler->uses_bc) {
        emit("\tPUSH B\t; Save callee-saved register for 'reg' keyword\n");
    }

    // Compile function body
    compile_statement(node->children[node->child_count - 1]);

    // Fallback epilogue in case function doesn't end with return
    emit("\t; Fallback epilogue\n");
    emit("\tLXI H, 0\n"); // default return 0
    if (compiler->uses_bc) {
        emit("\tPOP B\t; Restore callee-saved register for 'reg'\n");
    }
    emit("\tXCHG\n");
    if (compiler->use_frame_pointer) {
        emit("\tLHLD __FP\n");
        emit("\tSPHL\n");
        emit("\tPOP H\n");
        emit("\tSHLD __FP\n");
    } else {
        emit_shadow_stack_pops(compiler->symtab->symbols);
    }
    emit("\tXCHG\n");
    emit("\tRET\n");

    emit("\n; Local variables for %s\n", node->value);
    Symbol *sym = compiler->symtab->symbols;
    while (sym) {
        if (!sym->is_global && !sym->is_reg) {
            if (!compiler->use_frame_pointer || sym->is_static) {
                if (sym->is_static && sym->initializer_value) {
                    if (sym->is_16bit) {
                        emit("__VAR_%s_%s:\tDW %s\t; static initialized\n", compiler->current_function, sym->name, sym->initializer_value);
                    } else {
                        emit("__VAR_%s_%s:\tDB %s\t; static initialized\n", compiler->current_function, sym->name, sym->initializer_value);
                    }
                } else {
                    int size = (sym->is_16bit || sym->is_pointer ? 2 : 1) * (sym->array_size > 0 ? sym->array_size : 1);
                    emit("__VAR_%s_%s:\tDS %d\t; %s\n", compiler->current_function, sym->name, size, sym->array_size > 0 ? "array" : (sym->is_pointer ? "pointer" : (sym->is_static ? "static variable" : "variable")));
                }
            }
        }
        sym = sym->next;
    }
    
    // Restore symbol table to only contain globals
    compiler->symtab->symbols = global_syms;
}

static ASTNode* find_function(ASTNode *ast, const char *name) {
    for (int i = 0; i < ast->child_count; i++) {
        if (ast->children[i]->type == AST_FUNCTION && strcmp(ast->children[i]->value, name) == 0) {
            return ast->children[i];
        }
    }
    return NULL;
}

static void mark_calls_in_node(ASTNode *ast, ASTNode *node);

static void mark_function_used(ASTNode *ast, ASTNode *func) {
    if (!func || func->is_used) return;
    func->is_used = true;
    mark_calls_in_node(ast, func);
}

static void mark_calls_in_node(ASTNode *ast, ASTNode *node) {
    if (!node) return;
    if (node->type == AST_STRING) {
        return; // Safely ignore standard string literals to prevent substring collisions
    }
    if (node->type == AST_CALL || node->type == AST_IDENT) {
        mark_function_used(ast, find_function(ast, node->value));
    } else if (node->type == AST_ASM) {
        // Protect functions called from inline assembly
        for (int i = 0; i < ast->child_count; i++) {
            if (ast->children[i]->type == AST_FUNCTION) {
                if (strstr(node->value, ast->children[i]->value) != NULL) {
                    mark_function_used(ast, ast->children[i]);
                }
            }
        }
    }
    for (int i = 0; i < node->child_count; i++) {
        mark_calls_in_node(ast, node->children[i]);
    }
}

void compile_to_i8080(ASTNode *ast, FILE *output, bool use_frame_pointer, int org_address, int stack_address) {
    compiler = malloc(sizeof(Compiler));
    compiler->output = output;
    compiler->use_frame_pointer = use_frame_pointer;
    compiler->org_address = org_address;
    compiler->stack_address = stack_address;
    compiler->label_counter = 0;
    compiler->symtab = malloc(sizeof(SymbolTable));
    compiler->symtab->symbols = NULL;
    compiler->symtab->next_address = 0;
    compiler->uses_mul = false;
    compiler->uses_div = false;
    compiler->uses_mod = false;
    compiler->uses_icall = false;
    compiler->current_break_label = -1;
    compiler->current_continue_label = -1;

    bool has_custom_crt = false;
    for (int i = 0; i < ast->child_count; i++) {
        if (ast->children[i]->type == AST_ASM) {
            has_custom_crt = true;
            break;
        }
    }

    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);

    emit("; Generated i8080 assembly code\n");
    emit("; Compiled from C source on %s\n\n", time_str);

    if (!has_custom_crt) {
        emit("\tORG %04XH\n\n", compiler->org_address);
        emit("\t; Entry point\n");
        emit("\tLXI SP, STACK_TOP\t; Initialize stack pointer\n");
        if (compiler->use_frame_pointer) {
            emit("\tLXI H, 0\n");
            emit("\tSHLD __FP\t; Initialize frame pointer\n");
        }
        emit("\tCALL main\n");
        emit("\tHLT\n\n");
    } else {
        for (int i = 0; i < ast->child_count; i++) {
            if (ast->children[i]->type == AST_ASM) {
                emit("; Custom CRT / Global Assembly\n");
                emit("%s\n\n", ast->children[i]->value);
            }
        }
    }

    // Dead Code Elimination (Reachability Analysis)
    ASTNode *main_func = find_function(ast, "main");
    if (main_func) {
        mark_function_used(ast, main_func);
    } else if (!has_custom_crt) {
        // Library mode fallback: if no main, assume all functions are used
        for (int i = 0; i < ast->child_count; i++) {
            if (ast->children[i]->type == AST_FUNCTION) {
                ast->children[i]->is_used = true;
            }
        }
    }

    // Ensure functions called directly from the custom CRT are protected from DCE
    if (has_custom_crt) {
        for (int i = 0; i < ast->child_count; i++) {
            if (ast->children[i]->type == AST_ASM) {
                for (int j = 0; j < ast->child_count; j++) {
                    if (ast->children[j]->type == AST_FUNCTION) {
                        if (strstr(ast->children[i]->value, ast->children[j]->value) != NULL) {
                            mark_function_used(ast, ast->children[j]);
                        }
                    }
                }
            }
        }
    }

    // Collect global variables
    for (int i = 0; i < ast->child_count; i++) {
        if (ast->children[i]->type == AST_VARDECL) {
            bool is_pointer = (ast->children[i]->value[0] == '*');
            const char *var_name = is_pointer ? ast->children[i]->value + 1 : ast->children[i]->value;
            bool is_16bit = (ast->children[i]->datatype == 2) || is_pointer;
            bool target_is_16bit = (ast->children[i]->datatype == 2);
            add_symbol(compiler->symtab, var_name, true, is_pointer, is_16bit, target_is_16bit, ast->children[i]->array_size, false, false, ast->children[i]->initializer_value);
        }
    }

    // Compile all functions
    for (int i = 0; i < ast->child_count; i++) {
        if (ast->children[i]->type == AST_FUNCTION && ast->children[i]->is_used) {
            compile_function(ast->children[i]);
        }
    }

    // Runtime support functions (only emit if used)
    if (compiler->uses_mul || compiler->uses_div || compiler->uses_mod) {
        emit("\n; Runtime support functions\n");
    }

    if (compiler->uses_mul) {
        emit("__mul:\n");
        emit("\t; Multiply DE * HL, result in HL (16-bit)\n");
        emit("\tPUSH B\t; Preserve callee-saved reg variable\n");
        emit("\tMOV B, H\n");
        emit("\tMOV C, L\n");
        emit("\tLXI H, 0\n");
        emit("\tMVI A, 16\n");
        emit("__mul_loop:\n");
        emit("\tDAD H\n");
        emit("\tPUSH PSW\n");
        emit("\tMOV A, C\n");
        emit("\tRAL\n");
        emit("\tMOV C, A\n");
        emit("\tMOV A, B\n");
        emit("\tRAL\n");
        emit("\tMOV B, A\n");
        emit("\tJNC __mul_skip\n");
        emit("\tDAD D\n");
        emit("__mul_skip:\n");
        emit("\tPOP PSW\n");
        emit("\tDCR A\n");
        emit("\tJNZ __mul_loop\n");
        emit("\tPOP B\n");
        emit("\tRET\n\n");
    }

    if (compiler->uses_div) {
        emit("__div:\n");
        emit("\t; Divide DE / HL, result in HL (16-bit)\n");
        emit("\tPUSH B\t; Preserve callee-saved reg variable\n");
        emit("\tMOV B, H\n");
        emit("\tMOV C, L\n"); // BC = divisor
        emit("\tLXI H, 0\n"); // HL = quotient
        emit("__div_loop:\n");
        emit("\tMOV A, E\n");
        emit("\tSUB C\n");
        emit("\tMOV E, A\n");
        emit("\tMOV A, D\n");
        emit("\tSBB B\n");
        emit("\tMOV D, A\n");
        emit("\tJC __div_end\n"); // If borrow (carry), done
        emit("\tINX H\n");
        emit("\tJMP __div_loop\n");
        emit("__div_end:\n");
        emit("\tPOP B\n");
        emit("\tRET\n\n");
    }

    if (compiler->uses_mod) {
        emit("__mod:\n");
        emit("\t; Modulo DE %% HL, result in HL (16-bit)\n");
        emit("\tPUSH B\t; Preserve callee-saved reg variable\n");
        emit("\tMOV B, H\n");
        emit("\tMOV C, L\n"); // BC = divisor
        emit("__mod_loop:\n");
        emit("\tMOV A, E\n");
        emit("\tSUB C\n");
        emit("\tMOV E, A\n");
        emit("\tMOV A, D\n");
        emit("\tSBB B\n");
        emit("\tMOV D, A\n");
        emit("\tJC __mod_end\n"); // If borrow (carry), done
        emit("\tJMP __mod_loop\n");
        emit("__mod_end:\n");
        emit("\tMOV A, E\n\tADD C\n\tMOV L, A\n"); // Add divisor back to remainder
        emit("\tMOV A, D\n\tADC B\n\tMOV H, A\n");
        emit("\tPOP B\n");
        emit("\tRET\n\n");
    }

    if (compiler->uses_icall) {
        emit("__icall:\n");
        emit("\t; Jump to function pointer in HL, but keep return address on stack\n");
        emit("\tPCHL\n\n");
    }

    emit("; Runtime variables\n");
    if (compiler->use_frame_pointer) {
        emit("__FP:\tDS 2\t; Frame pointer for stack variables\n\n");
    }
    
    // Emit global variables
    Symbol *gsym = compiler->symtab->symbols;
    while (gsym) {
        if (gsym->is_global) {
            if (gsym->initializer_value) {
                if (gsym->is_16bit) {
                    emit("%s:\tDW %s\t; global initialized\n", gsym->name, gsym->initializer_value);
                } else {
                    emit("%s:\tDB %s\t; global initialized\n", gsym->name, gsym->initializer_value);
                }
            } else {
                int size = (gsym->is_16bit || gsym->is_pointer ? 2 : 1) * (gsym->array_size > 0 ? gsym->array_size : 1);
                emit("%s:\tDS %d\t; global variable\n", gsym->name, size);
            }
        }
        gsym = gsym->next;
    }
    
    if (!has_custom_crt) {
        emit("; Stack space (still needed for CALL/RET and temporary values)\n");
        emit("\tORG %04XH\n", compiler->stack_address);
        emit("STACK_TOP:\n");
    }

    free(compiler->symtab);
    free(compiler);
}
