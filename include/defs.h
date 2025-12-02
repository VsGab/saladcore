#pragma once
#ifdef MEM_DEBUG
    #include <stdio.h>
#endif


typedef int wd;
typedef long int ptr;
typedef unsigned char byte;

#ifndef bool
    #define bool byte
#endif


// Util
#define ADDR(a) ((ptr)a)
#define true 1
#define false 0
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define SWAP(a, b)   \
do {                 \
    int temp = a;    \
    a = b;           \
    b = temp;        \
} while(0)
#define STRINGIZING(x) #x
#define STR(x) STRINGIZING(x)
#define FILE_LINE "@ " __FILE__ ":" STR(__LINE__)


// Interface
typedef void (*iowritehandler)(ptr addr, int len);
typedef int (*ioreadhandler)(ptr addr, int len);
typedef void (*errorhandler)(ptr addr, int len);
typedef char (*iogetc)();
typedef bool (*ioeof)();


// Context - core + saladforth

#define DS_SZ 384
#define DS_LOW 128
#define RS_SZ 128

typedef struct {
    int d[DS_SZ];
    int r[RS_SZ];
} sf_stacks;


typedef struct {
    // I/O
    iowritehandler io_write;
    ioreadhandler io_read;
    errorhandler on_error;
    iogetc io_getc;
    ioeof io_eof;

    sf_stacks wp; // workspace - globals+stacks
    byte *mem;  // code and data memory
    int mem_sz;

    void *stage_impl;
} sf_ctx;

#define assert(cond) ( cond ? 0 : ctx->on_error(ADDR(FILE_LINE), sizeof(FILE_LINE)-1))
#define error(msg) (ctx->on_error(ADDR(msg), sizeof(msg)-1))
#define print(msg) ctx->io_write(ADDR(msg), sizeof(msg)-1);
#define oputc(c) {char buf[1] = {c}; ctx->io_write(ADDR(buf), 1); }

// Accessors for both C sim and saladforth

#define DS ctx->wp.d
#define RS ctx->wp.r
#define DSP ctx->wp.d[0]
#define RSP ctx->wp.d[1]
#define DSGET(off) (assert(DSP+off<DS_SZ), DS[DSP + off])
#define DSSET(off, val) {assert(DSP+off<DS_SZ); DS[DSP + off]=val;}
#define ADSGET(addr) (assert(addr>1 && addr<DS_SZ), DS[addr])
#define ADSSET(addr, val) {assert(addr>1 && addr<DS_SZ); DS[addr]=val;}
#define DTOS (assert(DSP<DS_SZ), DS[DSP])
#define DPOP() {assert(DSP<DS_SZ); DSP += 1;}
#define DPUSH(val) { assert(DSP>DS_LOW); DS[--DSP] = val; }
#define RPUSH(val) { assert(RSP>0); RS[--RSP] = val; }
#define RPOP()  (assert(RSP<RS_SZ), RS[RSP++])
#define MEM(addr) (assert(addr>=0 && addr<ctx->mem_sz), ctx->mem[addr])
#ifdef MEM_DEBUG
    #define MEM_LOG(addr, val) printf("MEM[%d]=%d\n", addr, val);
#else
    #define MEM_LOG(addr, val) {}
#endif

#define MEMSET(addr, val) {assert(addr>=0 && addr<ctx->mem_sz); \
                           MEM_LOG(addr, val); ctx->mem[addr]=(val);}
