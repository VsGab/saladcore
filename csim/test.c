#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include "core.h"
#include "enc.h"
#include "utils.h"

static byte mem[16384];
#define TEST_ADDR 500

byte mget(wd addr) {
  assert(addr > 0 && addr < sizeof(mem));
  return mem[addr];
}

void trace_handler(int type, wd pc, wd arg1, wd arg2) {
  if (type == INT_TRC) assert(false);
  if (type == ERROR_TRC) assert(false);
  default_trace_handler(type, pc, arg1, arg2);
}

void mset(wd addr, byte value) {
  assert(addr > 0 && addr < sizeof(mem));
  mem[addr] = value;
}

void io_write(byte addr, wd val) {
    // unused
}

wd io_read(byte addr) {
  return 0;
}


static const wd dsp = 200, rsp= 1000;

void setup(int addr) {
  saladcore_init(mem, sizeof(mem),
                dsp, rsp, rsp+4,
                io_write, io_read, trace_handler);
  saladcore_reset(addr);
}


void test_lit() {
  printf("\n--------------- LIT NOP ---------------\n");
  byte lit_nop_test[] = {
      lit(42),
      nop(),
      halt(),
  };
  memcpy(mem+TEST_ADDR,lit_nop_test, sizeof(lit_nop_test));
  setup(TEST_ADDR);
  saladcore_execute();
  assert(get_reg(0) == 42);
}

void test_push() {
  printf("\n--------------- LIT PUSH ---------------\n");
  byte lit_push_test[] = {
      lit((1 << 7) - 1),
      lit((1 << 7) - 1),
      nop(),
      lit(1),
      halt(),
  };
  memcpy(mem+TEST_ADDR,lit_push_test, sizeof(lit_push_test));
  setup(TEST_ADDR);
  saladcore_execute();
  assert(get_reg(0) == 1);
  assert(get_reg(1) == 128*128-1);
}

void test_regs() {

  printf("\n--------------- REGS ---------------\n");
  byte reg_test[] = {
      lit(17), nop(),
      lit(16), nop(),
      lit(15), nop(),
      lit(14), nop(),
      lit(13), nop(),
      lit(12), nop(),
      lit(11), nop(),
      lit(10), nop(),
      lit(9), nop(),
      lit(8), nop(),
      lit(7), nop(),
      lit(6), nop(),
      lit(5), nop(),
      lit(4), nop(),
      lit(3), nop(),
      lit(2), nop(),
      lit(1), nop(),
      lit(0), nop(),
      drop(),
      drop(),
      halt(),
  };
  memcpy(mem+TEST_ADDR, reg_test, sizeof(reg_test));
  setup(TEST_ADDR);
  saladcore_execute();
  assert(get_reg(15) == 17);
  assert(get_reg(14) == 16);
  assert(get_reg(0) == 2);
}

void test_stack() {
  printf("\n--------------- STACK ---------------\n");
  byte stack1[] = {
      lit(1), nop(), // #0
      lit(2), nop(), // #1
      lit(3), nop(), // #2
      lit(4), nop(),// #3
      lit(4),
      fset(),
      lit(40),
      add(), // 1 2 3 44
      lit(6), // 1 2 3 44 (6)
      rpick(0),
      add(),
      rmov(0), // 7 2 3 44
      halt(),
  };
  memcpy(mem+TEST_ADDR, stack1, sizeof(stack1));
  setup(TEST_ADDR);
  saladcore_execute();
  assert(get_reg(0) == 44);
  assert(get_reg(3) == 7);
}

void test_cmp() {
  printf("\n--------------- CMP ---------------\n");
  byte cmp_test[] = {
      lit(1), nop(), // #0
      lit(2), nop(),
      cond(CLESS),
      lit(2),
      jmp(), // 1 < 2 true, jmp over next
      lit(7),
      nop(),
      lit(3), nop(), // 3
      lit(4), // 4
      over(),
      cond(CGEQ), // r1 is dst, r1 < r0 true
      lit(1),
      jmp(), // jump over next
      lit(6),
      halt(),
  };
  memcpy(mem+TEST_ADDR, cmp_test, sizeof(cmp_test));
  setup(TEST_ADDR);
  saladcore_execute();
  assert(get_reg(0) == 4); // 6 not inserted
  assert(get_reg(1) == 3); // 6 not inserted
  assert(get_reg(2) == 1); // 7 not inserted, 2 consumed
}

void test_calls() {
  printf("\n--------------- CALLS FRAME SIZE ---------------\n");
  byte call_func[] = {
      // (a -- a+1)
      lit(1),
      add(),
      ret(),
      // (a b -- 42 b 31)
      lit(2),
      fset(),
      lit(42),
      rmov(0),
      lit(30),
      nop(),
      lit(hb(TEST_ADDR)),
      lit(lb(TEST_ADDR)),
      call(),
      ret(),
      // ----
      lit(0),
      fset(),
      lit(5), nop(),
      lit(4), nop(),
      lit(2), nop(), // #0 arg
      lit(1), nop(), // #1 arg
      lit(hb(TEST_ADDR+3)), // call address msb
      lit(lb(TEST_ADDR+3)),
      call(), // 5 4 |2 1 -> |5 4 42 1 31
      // add 31 to 5
      rpick(0), // 5 4 42 1 31 5
      add(),
      rmov(0), // 36 4 42 1 31
      halt(),
  };
  memcpy(mem+TEST_ADDR, call_func, sizeof(call_func));
  setup(TEST_ADDR+13);
  saladcore_execute();
  assert(get_fsz() == get_dsz());
  assert(get_reg(3) == 36); // #0=r3
  assert(get_reg(2) == 4); // #0=r2
  assert(get_reg(1) == 42); // #2=r1
}

void test_fset() {
  printf("\n--------------- FSET ---------------\n");
  byte call_func[] = {
      // (a b c -- a+1)
      lit(3),
      fset(), // | 4 2 1
      lit(99),
      rpick(0), // | 4 2 1 99 4
      lit(9),
      add(), // | 4 2 1 99 13
      ret(),
      // ----
      lit(18), nop(),
      lit(17), nop(),
      lit(16), nop(),
      lit(15), nop(),
      lit(14), nop(),
      lit(13), nop(),
      lit(12), nop(),
      lit(11), nop(),
      lit(10), nop(),
      lit(9), nop(),
      lit(0),
      fset(),
      lit(8), nop(),
      lit(7), nop(),
      lit(6), nop(),
      lit(5), nop(),
      lit(4), nop(),
      lit(2), nop(), // #0 arg
      lit(1), nop(), // #1 arg
      lit(hb(TEST_ADDR)), // call address msb
      lit(lb(TEST_ADDR)),
      call(), // 5 |4 2 1 -> 5 | 4 2 1 99 13
      halt(),
  };
  memcpy(mem+TEST_ADDR,call_func, sizeof(call_func));
  setup(TEST_ADDR+7);
  saladcore_execute();
  assert(get_reg(0) == 13);
  assert(get_reg(1) == 99);
}

void test_jump() {
  printf("\n--------------- JUMP ---------------\n");
  byte jump[] = {
      lit(2),
      jmp(),
      lit(2), nop(), //skipped
      lit(3), nop(),
      lit(4), nop(),
      lit(hb(TEST_ADDR+13)),
      lit(lb(TEST_ADDR+13)),
      ljmp(),
      lit(5),
      nop(),
      lit(6), // addr=13
      nop(),
      lit(7),
      halt(),
  };
  memcpy(mem+TEST_ADDR, jump, sizeof(jump));
  setup(TEST_ADDR);
  saladcore_execute();
  assert(get_reg(0) == 7);
  assert(get_reg(1) == 6);
  assert(get_reg(2) == 4);
  assert(get_reg(3) == 3);
  assert(get_dsz() == 4);
}

void test_loop() {
  printf("\n--------------- LOOP ---------------\n");
  byte looptest[] = {
      lit(8), nop(), // 8
      lit(0), nop(), // 8 0
      lit(1), // 8 0 1
      add(), // 8 1
      over(),
      cond(CNEQ),
      lit(4),
      loop(), // to psh(1)
      halt(),
  };
  memcpy(mem+TEST_ADDR,looptest, sizeof(looptest));
  setup(TEST_ADDR);
  saladcore_execute();
  assert(get_reg(0) == 8);
  assert(get_reg(1) == 8);
}

void test_memcpy() {
  printf("\n--------------- MEMCPY ---------------\n");

  mset(128, 95);
  mset(129, 96);
  mset(130, 97);
  byte simple_memcpy[] = {
      // lim #0, dst #1, src #2
      lit(0),
      fset(),
      lit(1),
      lit(3),
      nop(), // lim
      lit(1),
      lit(10),
      nop(), // lim dst
      lit(1),
      lit(0), // lim dst src
      // -------
      rpick(0), // lim dst src lim
      cond(CLESS), // lim dst src
      lit(1),
      jmp(),
        halt(),
      mcpb(), // lim dst src
      lit(1),
      add(),  // lim dst src+1
      swap(), // lim src+1 dst
      lit(1),
      add(), // lim src+1 dst+1
      swap(), // lim dst+1 src+1
      lit(12),
      loop(),

      halt(),
  };
  memcpy(mem+TEST_ADDR,simple_memcpy, sizeof(simple_memcpy));
  setup(TEST_ADDR);
  saladcore_execute();
  assert(mget(138) == 95);
  assert(mget(139) == 96);
  assert(mget(140) == 97);

}

int main()
{
  test_lit();
  test_push();
  test_regs();
  test_stack();
  test_cmp();
  test_fset();
  test_calls();
  test_jump();
  test_loop();
  test_memcpy();
  printf("\nTests OK\n");

  return 0;
}
