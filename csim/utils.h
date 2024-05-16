#pragma once
#include <stdio.h>
#include "core.h"

#define UNUSED __attribute__((unused))

#define BYTE_TO_BINARY(byte)  \
  ((byte) & 0x80 ? '1' : '0'), \
  ((byte) & 0x40 ? '1' : '0'), \
  ((byte) & 0x20 ? '1' : '0'), \
  ((byte) & 0x10 ? '1' : '0'), \
  ((byte) & 0x08 ? '1' : '0'), \
  ((byte) & 0x04 ? '1' : '0'), \
  ((byte) & 0x02 ? '1' : '0'), \
  ((byte) & 0x01 ? '1' : '0')

static void print_inst(wd pc, byte val) {
  printf("\n%05d: %c%c%c%c %c%c%c%c", pc, BYTE_TO_BINARY(val));
}

static const char* nullary_name(byte op) {
  switch(op) {
    case HALT: return "HALT";
    case NOP: return "NOP";
    case DUP: return "DUP";
    case RET: return "RET";
    case NEG: return "NEG";
    case INC: return "INC";
    case DEC: return "DEC";
    case FSZ: return "FSZ";
    case FSET: return "FSET";
    case FKEEP: return "FKEEP";
    case INT: return "INT";
    case CALL: return "CALL";
    case LOOP: return "LOOP";
    case JMP: return "JMP";
    case LJMP: return "LJMP";
  }
  return "?";
}

static const char* regop_name(byte op) {
   switch (op) {
      case RPICK: return "RPICK";
      case RSWAP: return "RSWAP";
      case RMOV: return "RMOV";
    }
  return "?";
}

static const char* cond_name(byte op) {
   switch (op) {
      case CZERO: return "CZERO";
      case CNZERO: return "CNZERO";
      case CEQ: return "CEQ";
      case CNEQ: return "CNEQ";
      case CGEQ: return "CGEQ";
      case CLESS: return "CLESS";
    }
  return "?";
}

static const char* memop_name(byte op) {
   switch (op) {
      case MEMCPYB: return "MEMCPYB";
      case MEMCPYW: return "MEMCPYW";
      case ILOADB: return "ILOADB";
      case ILOADW: return "ILOADW";
      case STOREB: return "STOREB";
      case STOREW: return "STOREW";
      case ASTOREB: return "ASTOREB";
      case ASTOREW: return "ASTOREW";
    }
  return "?";
}


static const char* aluop_name(byte op) {
  switch(op) {
    case DROP: return "DROP";
    case ADD: return "ADD";
    case SUB: return "SUB";

    case AND: return "AND";
    case OR: return "OR";
    case XOR: return "XOR";
    case MUL: return "MUL";
    case DIV: return "DIV";
    case SHR: return "SHR";
    case SHL: return "SHL";


  }
  return "?";
}


UNUSED static const char* err_type(int err) {
  switch(err) {
    case ERR_D_STACK: return "data stack";
    case ERR_R_STACK: return "return stack";
    case ERR_REG: return "register";
    case ERR_FRAME: return "frame";
    case ERR_MEM: return "memory";
    case ERR_INT: return "interrupt";
    case ERR_PC: return "program counter";
    case ERR_WATCHDOG: return "watchdog";
    case ERR_SW: return "software";
  }
  return "?";
}

UNUSED static void dump_regs()
{
  int fsz = get_fsz(), dsz = get_dsz();
  assert(dsz >= 0);
  printf("{ FSZ=%d DSZ=%d } \t", fsz, dsz);
  dsz = MIN(dsz, NUM_REGS);
  for (int i = dsz-1; i >= 0; --i) {
    if (i == fsz-1) printf("| ");
    printf("r%d=%d  ", i, get_reg(i));
  }
  if (fsz == 0) printf(" | ");
  printf("\n");
}


UNUSED static void default_trace_handler(int type, wd pc, wd arg1, wd arg2) {
    switch (type)
    {
    case INSN_TRC:
        print_inst(pc, arg1);
        break;
    case DECODE_TRC:
        switch (arg1)
        {
            case ALU_INSN:
                printf("  %s", aluop_name(arg2));
                break;
            case REG_INSN:
                printf("  %s", regop_name(arg2));
                break;
            case TOS_INSN:
                printf("  %s", nullary_name(arg2));
                break;
            case MEM_INSN:
                printf("  %s", memop_name(arg2));
                break;
            case LIT_INSN:
                printf("  LIT %d", arg2);
                break;
        }
        break;
    case COND_TRC:
        printf("  COND %s ? %s",  cond_name(arg1), arg2 ? "T" : "F");
        break;
    case AFTER_TRC:
        printf("\n%05d:\t", pc);
        dump_regs();
        break;
    case LOAD_TRC:
        printf("  MEM[%d] => %d", arg1, arg2);
        break;
    case STORE_TRC:
        printf("  MEM[%d] <= %d", arg1, arg2);
        break;
    case REGOP_TRC:
        printf("(%d = #%d)", arg1, arg2);
        break;
    case CALL_TRC:
    case RET_TRC:
        printf("  DFSZ=%d FSET=%d", arg1, arg2);
        break;
    default:
        break;
    }
    fflush(stdout);
}
