// target_i8080.h
#ifndef TARGET_I8080_H
#define TARGET_I8080_H

// ============================================================================
// i8080 Target Definition File
// Abstracts hardware-specific assembly strings away from the compiler AST logic
// ============================================================================

// Stack Operations
#define I8080_PUSH_HL() emit("\tPUSH H\n")
#define I8080_POP_HL()  emit("\tPOP H\n")
#define I8080_PUSH_DE() emit("\tPUSH D\n")
#define I8080_POP_DE()  emit("\tPOP D\n")
#define I8080_PUSH_BC() emit("\tPUSH B\n")
#define I8080_POP_BC()  emit("\tPOP B\n")
#define I8080_PUSH_PSW() emit("\tPUSH PSW\n")
#define I8080_POP_PSW()  emit("\tPOP PSW\n")

// Data Movement
#define I8080_XCHG()    emit("\tXCHG\n")
#define I8080_LOAD_IMM_16(val) emit("\tLXI H, %s\n", val)
#define I8080_LOAD_IMM_16_INT(val) emit("\tLXI H, %d\n", val)
#define I8080_LOAD_IMM_16_LABEL(lbl) emit("\tLXI H, L%d\n", lbl)
#define I8080_LOAD_IMM_DE_INT(val) emit("\tLXI D, %d\n", val)

#define I8080_COPY_HL_TO_BC() emit("\tMOV B, H\n\tMOV C, L\n")
#define I8080_COPY_BC_TO_HL() emit("\tMOV H, B\n\tMOV L, C\n")
#define I8080_COPY_L_TO_C()   emit("\tMOV C, L\n\tMVI B, 0\n")
#define I8080_COPY_C_TO_L()   emit("\tMOV L, C\n\tMVI H, 0\n")
#define I8080_COPY_DE_TO_BC() emit("\tMOV B, D\n\tMOV C, E\n")
#define I8080_COPY_L_TO_A()   emit("\tMOV A, L\n")
#define I8080_COPY_A_TO_L()   emit("\tMOV L, A\n")
#define I8080_ZERO_H()        emit("\tMVI H, 0\n")

// Global Variables
#define I8080_LOAD_GLOBAL_16(name) emit("\tLHLD %s\n", name)
#define I8080_STORE_GLOBAL_16(name) emit("\tSHLD %s\n", name)
#define I8080_LOAD_GLOBAL_8(name)  emit("\tLDA %s\n", name)
#define I8080_STORE_GLOBAL_8(name) emit("\tSTA %s\n", name)

// Local Variables (Shadow Stack)
#define I8080_LOAD_LOCAL_16(func, name) emit("\tLHLD __VAR_%s_%s\n", func, name)
#define I8080_STORE_LOCAL_16(func, name) emit("\tSHLD __VAR_%s_%s\n", func, name)
#define I8080_LOAD_LOCAL_8(func, name)  emit("\tLDA __VAR_%s_%s\n", func, name)
#define I8080_STORE_LOCAL_8(func, name) emit("\tSTA __VAR_%s_%s\n", func, name)
#define I8080_LOAD_LOCAL_ADDR(func, name) emit("\tLXI H, __VAR_%s_%s\n", func, name)
#define I8080_LOAD_LOCAL_ADDR_OFFSET(func, name, offset) emit("\tLXI H, __VAR_%s_%s+%d\n", func, name, offset)

// Frame Pointer
#define I8080_LOAD_FP() emit("\tLHLD __FP\n")
#define I8080_STORE_FP() emit("\tSHLD __FP\n")
#define I8080_ADD_FP_OFFSET(offset) emit("\tLXI D, %d\n\tDAD D\n", offset)
#define I8080_ADD_FP_OFFSET_BC(offset) emit("\tLXI B, %d\n\tDAD B\n", offset)

// Memory Access (Pointers)
#define I8080_READ_16_AT_HL() emit("\tMOV E, M\n\tINX H\n\tMOV D, M\n")
#define I8080_WRITE_16_AT_HL() emit("\tMOV M, E\n\tINX H\n\tMOV M, D\n")
#define I8080_READ_8_AT_HL()  emit("\tMOV A, M\n")
#define I8080_WRITE_8_AT_HL() emit("\tMOV M, E\n")
#define I8080_READ_8_TO_L()   emit("\tMOV L, M\n\tMVI H, 0\n")

// Arithmetic
#define I8080_ADD_HL_DE() emit("\tDAD D\n")
#define I8080_ADD_HL_HL() emit("\tDAD H\n")
#define I8080_ADD_HL_SP() emit("\tDAD SP\n")
#define I8080_ADD_SP(offset) emit("\tLXI H, %d\n\tDAD SP\n", offset)
#define I8080_INC_HL()    emit("\tINX H\n")
#define I8080_DEC_HL()    emit("\tDCX H\n")
#define I8080_SUB_DE_HL() emit("\tMOV A, E\n\tSUB L\n\tMOV L, A\n\tMOV A, D\n\tSBB H\n\tMOV H, A\n")
#define I8080_SUB_HL_DE() emit("\tMOV A, L\n\tSUB E\n\tMOV L, A\n\tMOV A, H\n\tSBB D\n\tMOV H, A\n")
#define I8080_MUL_INLINE() emit("\tPUSH B\n\tMOV A, E\n\tMOV B, L\n\tMUL B\n\tPOP B\n")

// Logic
#define I8080_AND_HL_DE() emit("\tMOV A, E\n\tANA L\n\tMOV L, A\n\tMOV A, D\n\tANA H\n\tMOV H, A\n")
#define I8080_OR_HL_DE()  emit("\tMOV A, E\n\tORA L\n\tMOV L, A\n\tMOV A, D\n\tORA H\n\tMOV H, A\n")
#define I8080_XOR_HL_DE() emit("\tMOV A, E\n\tXRA L\n\tMOV L, A\n\tMOV A, D\n\tXRA H\n\tMOV H, A\n")
#define I8080_NOT_HL() emit("\tMOV A, H\n\tCMA\n\tMOV H, A\n\tMOV A, L\n\tCMA\n\tMOV L, A\n")
#define I8080_NEG_HL() I8080_NOT_HL(); I8080_INC_HL()
#define I8080_TEST_HL() emit("\tMOV A, H\n\tORA L\n")

// Shifts
#define I8080_SHL_LOOP(lbl_loop, lbl_end) \
    emit("\tXCHG\n\tMOV A, E\n\tORA A\n\tJZ L%d\nL%d:\n\tDAD H\n\tDCR E\n\tJNZ L%d\nL%d:\n", lbl_end, lbl_loop, lbl_loop, lbl_end)
#define I8080_SHR_LOOP(lbl_loop, lbl_end) \
    emit("\tXCHG\n\tMOV A, E\n\tORA A\n\tJZ L%d\nL%d:\n", lbl_end, lbl_loop); \
    emit("\tMOV A, H\n\tRLC\n\tMOV A, H\n\tRAR\n\tMOV H, A\n\tMOV A, L\n\tRAR\n\tMOV L, A\n"); \
    emit("\tDCR E\n\tJNZ L%d\nL%d:\n", lbl_loop, lbl_end)

// Comparisons
#define I8080_CMP_EQ(lbl_false, lbl_end) \
    emit("\tMOV A, E\n\tCMP L\n\tJNZ L%d\n\tMOV A, D\n\tCMP H\n\tJNZ L%d\n\tLXI H, 1\n\tJMP L%d\nL%d:\n\tLXI H, 0\nL%d:\n", lbl_false, lbl_false, lbl_end, lbl_false, lbl_end)
#define I8080_CMP_NE(lbl_true, lbl_false, lbl_end) \
    emit("\tMOV A, E\n\tCMP L\n\tJNZ L%d\n\tMOV A, D\n\tCMP H\n\tJZ L%d\nL%d:\n\tLXI H, 1\n\tJMP L%d\nL%d:\n\tLXI H, 0\nL%d:\n", lbl_true, lbl_false, lbl_true, lbl_end, lbl_false, lbl_end)
#define I8080_CMP_LT(lbl_false, lbl_end) \
    emit("\tMOV A, E\n\tSUB L\n\tMOV A, D\n\tSBB H\n\tJM L%d\n\tLXI H, 0\n\tJMP L%d\nL%d:\n\tLXI H, 1\nL%d:\n", lbl_false, lbl_end, lbl_false, lbl_end)
#define I8080_CMP_LE(lbl_false, lbl_end) \
    emit("\tMOV A, L\n\tSUB E\n\tMOV A, H\n\tSBB D\n\tJM L%d\n\tLXI H, 1\n\tJMP L%d\nL%d:\n\tLXI H, 0\nL%d:\n", lbl_false, lbl_end, lbl_false, lbl_end)
#define I8080_CMP_GT(lbl_false, lbl_end) \
    emit("\tMOV A, L\n\tSUB E\n\tMOV A, H\n\tSBB D\n\tJM L%d\n\tLXI H, 0\n\tJMP L%d\nL%d:\n\tLXI H, 1\nL%d:\n", lbl_false, lbl_end, lbl_false, lbl_end)
#define I8080_CMP_GE(lbl_false, lbl_end) \
    emit("\tMOV A, E\n\tSUB L\n\tMOV A, D\n\tSBB H\n\tJM L%d\n\tLXI H, 1\n\tJMP L%d\nL%d:\n\tLXI H, 0\nL%d:\n", lbl_false, lbl_end, lbl_false, lbl_end)
#define I8080_LOGICAL_AND(lbl_false, lbl_end) \
    emit("\tMOV A, D\n\tORA E\n\tJZ L%d\n\tMOV A, H\n\tORA L\n\tJZ L%d\n\tLXI H, 1\n\tJMP L%d\nL%d:\n\tLXI H, 0\nL%d:\n", lbl_false, lbl_false, lbl_end, lbl_false, lbl_end)
#define I8080_LOGICAL_OR(lbl_true, lbl_end) \
    emit("\tMOV A, D\n\tORA E\n\tJNZ L%d\n\tMOV A, H\n\tORA L\n\tJNZ L%d\n\tLXI H, 0\n\tJMP L%d\nL%d:\n\tLXI H, 1\nL%d:\n", lbl_true, lbl_true, lbl_end, lbl_true, lbl_end)
#define I8080_LOGICAL_NOT(lbl_false, lbl_end) \
    emit("\tMOV A, H\n\tORA L\n\tJNZ L%d\n\tLXI H, 1\n\tJMP L%d\nL%d:\n\tLXI H, 0\nL%d:\n", lbl_false, lbl_end, lbl_false, lbl_end)
    
// Control Flow
#define I8080_JMP(lbl) emit("\tJMP L%d\n", lbl)
#define I8080_JZ(lbl)  emit("\tJZ L%d\n", lbl)
#define I8080_JNZ(lbl) emit("\tJNZ L%d\n", lbl)
#define I8080_LABEL(lbl) emit("L%d:\n", lbl)
#define I8080_CALL(func) emit("\tCALL %s\n", func)
#define I8080_JMP_STR(func, lbl) emit("\tJMP __LABEL_%s_%s\n", func, lbl)
#define I8080_LABEL_STR(func, lbl) emit("__LABEL_%s_%s:\n", func, lbl)
#define I8080_CALL_INDIRECT() emit("\tCALL __icall\n")
#define I8080_RET() emit("\tRET\n")

// Pointers & Stack Base
#define I8080_SPHL() emit("\tSPHL\n")
#define I8080_INIT_SP() emit("\tLXI SP, STACK_TOP\n")
#define I8080_HLT() emit("\tHLT\n")

// Memory and Data Directives
#define I8080_ORG(addr) emit("\tORG %04XH\n", addr)
#define I8080_DS(size)  emit("\tDS %d\n", size)
#define I8080_DW(val)   emit("\tDW %s\n", val)
#define I8080_DB(val)   emit("\tDB %s\n", val)
#define I8080_DB_BYTE(val) emit("%d,", (unsigned char)val)
#define I8080_COMMENT(msg) emit("\t; %s\n", msg)
#define I8080_RAW(msg) emit(msg)
#define I8080_FUNC_LABEL(name) emit("\n%s:\n", name)

// Variable Formatting
#define I8080_FMT_LOCAL_VAR(func, name) emit("__VAR_%s_%s", func, name)
#define I8080_FMT_GLOBAL_VAR(name) emit("__VAR_%s", name)
#define I8080_STATIC_DW(func, name, val) emit("__VAR_%s_%s:\tDW %s\t; static initialized\n", func, name, val)
#define I8080_STATIC_DB(func, name, val) emit("__VAR_%s_%s:\tDB %s\t; static initialized\n", func, name, val)
#define I8080_LOCAL_DS(func, name, size, comment) emit("__VAR_%s_%s:\tDS %d\t; %s\n", func, name, size, comment)
#define I8080_GLOBAL_DW(name, val) emit("%s:\tDW %s\t; global initialized\n", name, val)
#define I8080_GLOBAL_DB(name, val) emit("%s:\tDB %s\t; global initialized\n", name, val)
#define I8080_GLOBAL_DS(name, size) emit("%s:\tDS %d\t; global variable\n", name, size)

// Runtimes
#define I8080_EMIT_RUNTIME_MUL() emit("__mul:\n\t; Multiply DE * HL, result in HL (16-bit)\n\tPUSH B\t; Preserve callee-saved reg variable\n\tMOV B, H\n\tMOV C, L\n\tLXI H, 0\n\tMVI A, 16\n__mul_loop:\n\tDAD H\n\tPUSH PSW\n\tMOV A, C\n\tRAL\n\tMOV C, A\n\tMOV A, B\n\tRAL\n\tMOV B, A\n\tJNC __mul_skip\n\tDAD D\n__mul_skip:\n\tPOP PSW\n\tDCR A\n\tJNZ __mul_loop\n\tPOP B\n\tRET\n\n")
#define I8080_EMIT_RUNTIME_DIV() emit("__div:\n\t; Divide DE / HL, result in HL (16-bit)\n\tPUSH B\t; Preserve callee-saved reg variable\n\tMOV B, H\n\tMOV C, L\n\tLXI H, 0\n__div_loop:\n\tMOV A, E\n\tSUB C\n\tMOV E, A\n\tMOV A, D\n\tSBB B\n\tMOV D, A\n\tJC __div_end\n\tINX H\n\tJMP __div_loop\n__div_end:\n\tPOP B\n\tRET\n\n")
#define I8080_EMIT_RUNTIME_MOD() emit("__mod:\n\t; Modulo DE %% HL, result in HL (16-bit)\n\tPUSH B\t; Preserve callee-saved reg variable\n\tMOV B, H\n\tMOV C, L\n__mod_loop:\n\tMOV A, E\n\tSUB C\n\tMOV E, A\n\tMOV A, D\n\tSBB B\n\tMOV D, A\n\tJC __mod_end\n\tJMP __mod_loop\n__mod_end:\n\tMOV A, E\n\tADD C\n\tMOV L, A\n\tMOV A, D\n\tADC B\n\tMOV H, A\n\tPOP B\n\tRET\n\n")
#define I8080_EMIT_RUNTIME_ICALL() emit("__icall:\n\t; Jump to function ptr in HL, retain return address on stack\n\tPCHL\n\n")

#endif // TARGET_I8080_H