#pragma once
#include "core_state.h"

static byte *memory;
static int memory_size;
static tracehandler tracefunc;
static iowritehandler iowrite;
static ioreadhandler ioread;
static long watchdog;

#define WATCHDOG 1000000
#define trace(type, arg1, arg2) tracefunc(type,pc,arg1,arg2)
#define check(cond,err) {if(!(cond)) {halted=true; tracefunc(ERROR_TRC, pc, err, 0);}}
#define unimplemented() {halted=true;tracefunc(ERROR_TRC, pc, ERR_UNIMPLEMENTED, 0);}

static void byte_store(wd addr, byte val)
{
  check(addr>=0, ERR_MEM);
  check(addr<memory_size, ERR_MEM);
  if (addr < IO_MAX) {
    iowrite(addr, val);
  } else {
    trace(STORE_TRC, addr, val);
    memory[addr] = val;
  }
}

static byte byte_load(wd addr)
{
  check(addr>=0, ERR_MEM);
  check(addr<memory_size, ERR_MEM);
  if (addr < IO_MAX) {
    return ioread(addr);
  } else {
    byte val = memory[addr];
    trace(LOAD_TRC, addr, val);
    return val;
  }
}

static void word_store(wd addr, wd val)
{
  check(addr>=0, ERR_MEM);
  check(addr<memory_size-2, ERR_MEM);
  trace(STORE_TRC, addr, val);
  memory[addr] = val & 0xff;
  memory[addr+1] = (val >> 8) & 0xff;
}

static wd word_load(wd addr)
{
  check(addr>=0, ERR_MEM);
  check(addr<memory_size-2, ERR_MEM);
  wd val = memory[addr+1];
  val <<= 8;
  val |= memory[addr];
  trace(LOAD_TRC, addr, val);
  return val;
}

static void byte_reg_store(reg addr_reg, reg val_reg)
{
  byte_store(rget(addr_reg), rget(val_reg));
}

static byte byte_reg_load(reg addr_reg)
{
  return byte_load(rget(addr_reg));
}

static void word_reg_store(reg addr_reg, reg val_reg)
{
  word_store(rget(addr_reg), rget(val_reg));
}

static wd word_reg_load(reg addr_reg)
{
  return word_load(rget(addr_reg));
}


static void ds_push_mem() {
  // dsp points to not-written-yet location
  word_store(dsp, rget(NUM_REGS-1));
  bspop();
  dsp += 2;
}

static void ds_pop_mem() {
  dsp -= 2;
  bspush(word_load(dsp));
}

static void rs_push_dec() {
  rsp -= 4;
}

static void rs_pop_inc() {
  rsp += 4;
}

static void frame_inc() {
  ++fsz;
}

static void frame_dec() {
  --fsz;
}


static byte flags_byte() {
  byte res = 0;
  res |= (take_jmp&1);
  res |= (lit_dirty&1) << 1;
  return res;
}

static void restore_flags(byte flags) {
  take_jmp =  (flags) & 1;
  lit_dirty = (flags >> 1) & 1;
}

static byte frame_info_byte() {
  check(dsz>=fsz, ERR_FRAME);
  check(dsz-fsz<=255, ERR_FRAME);
  return dsz-fsz;
}

static void restore_frame_info(byte dsz_diff) {
  // diff = dsz-fsz
  fsz = dsz-dsz_diff;
}

static void rspush(wd next_pc)
{
  check(rsp>=0, ERR_R_STACK);
  check(rsp<memory_size-4, ERR_R_STACK);
  word_store(rsp, next_pc);
  byte_store(rsp+2, flags_byte());
  byte_store(rsp+3, frame_info_byte());
  rs_push_dec();
}

static wd rspop()
{
  rs_pop_inc();
  restore_flags(byte_load(rsp+2));
  restore_frame_info(byte_load(rsp+3));
  return word_load(rsp);
}

void swap(reg r1, reg r2)
{
  wd r1_val = rget(r1);
  rset(r1, rget(r2));
  rset(r2, r1_val);
}

static void ds_push(wd val)
{
  if (dsz >= NUM_REGS) ds_push_mem();
  tspush(val);
  ++dsz;
  frame_inc();
}

static void ds_pop(void) // pop value from the data stack
{
  check(dsz > 0, ERR_D_STACK);
  tspop();
  if(dsz > NUM_REGS) ds_pop_mem();
  --dsz;
  frame_dec();
}

static void set_frame_size(byte val) {
  ds_pop();
  if (val == FUNSET) {
    fsz = dsz;
    return;
  }
  check(val <= NUM_REGS, ERR_FRAME);
  fsz = val;
}

static void ret_shift(byte _fsz) {
  for(int src = _fsz-1, dst=fsz-1;src>=0;--src, --dst) {
    rset(dst, rget(src));
  }
  int to_pop = fsz - _fsz;
  for (int i = 0 ; i < to_pop; ++i)  ds_pop();
}

static void frame_keep_shift(byte new_fsz) {
  ds_pop();
  ret_shift(new_fsz);
}

static void check_watchdog() {
  --watchdog;
  check(watchdog > 0, ERR_WATCHDOG);
}

static void do_call(wd addr) {
  wd old_next_pc = next_pc;
  next_pc = addr;
  ds_pop();
  trace(CALL_TRC, frame_info_byte(), 0);
  rspush(old_next_pc);
}

static void soft_irq(byte idx) {
  check(idx < NUM_INT, ERR_INT);
  trace(INT_TRC, idx, 0);
  check(idx != 0, ERR_SW);
  wd isr = ivt + (idx << ISR_SHIFT);
  wd addr = word_load(isr);
  check(addr, ERR_INT);
  do_call(addr);
}

///////////////////////////////
// Interfacing utils

wd get_reg(reg x)
{
  return rget(x);
}

byte get_fsz() {
  return fsz;
}

wd get_dsz() {
  return dsz;
}


void saladcore_interrupt(byte idx) {
  wd isr = ivt + (idx << ISR_SHIFT);
  wd addr = word_load(isr);
  if (!halted) {
    rspush(next_pc); // never actually
  }
  tracefunc(HWINT_TRC, pc, idx, addr);
  pc = addr;
  watchdog = WATCHDOG;
  halted = false;
}

void saladcore_reset(wd addr) {
  halted = false;
  pc = addr;
  watchdog = WATCHDOG;
}

void saladcore_start() {
  halted = false;
  pc = word_load(0);
}

void set_soft_irq(byte idx, wd addr) {
    wd isr = ivt + (idx << ISR_SHIFT);
    word_store(isr, addr);
}
