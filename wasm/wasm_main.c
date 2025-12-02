#include "defs.h"
#include "saladforth.h"

#define DISP 128
#define IOBUF_SZ 4
#define MEM_SZ 1<<16

#define _DTOS ctx.wp.d[ctx.wp.d[0]]

extern void js_io_write(ptr addr, int len);
extern int js_io_read(ptr addr, int len);
extern void js_on_error(ptr addr, int len);
extern void js_tui_puts(ptr addr, int len);
extern void js_tui_putc(int val);
extern void js_tui_pos(int row, int col);
extern void js_tui_clr();
extern int js_tui_width();
extern int js_tui_height();

static byte main_mem[MEM_SZ];
static sf_ctx ctx;

// IO
static byte in_buff[IOBUF_SZ];
static int in_sz = 0;
static int in_ptr = 0;


void read_more() {
    if (in_sz == 0) {
        in_sz = js_io_read(ADDR(in_buff), sizeof(in_buff));
        in_ptr = 0;
    }
}

char my_getc() {
    read_more();
    if (!in_sz) return -1;
    in_sz -= 1;
    return in_buff[in_ptr++];
}

bool my_eof() {
    read_more();
    return !in_sz;
}

int main_mem_addr() {
    return (int)&main_mem;
}

int main_mem_bytes() {
    return sizeof(main_mem);
}

void saladcore_js_init() {
    ctx.io_read = js_io_read;
    ctx.io_write = js_io_write;
    ctx.on_error = js_on_error;
    ctx.io_getc = my_getc;
    ctx.io_eof = my_eof;
    ctx.mem = main_mem;
    ctx.mem_sz = sizeof(main_mem);
    saladforth_init(&ctx);
}

void saladcore_js_exec() {
    saladforth_eval_input(&ctx);
}

void saladcore_kbd(int key) {
    js_tui_putc(key);
}

int iobuff_addr() {
    return ADDR(in_buff);
}

int iobuff_size() {
    return sizeof(in_buff);
}

void ext_tui_pos(int row, int col) {
    js_tui_pos(row, col);
}

void ext_tui_puts(const char* addr, int len) {
    js_tui_puts(ADDR(addr), len);
}

void ext_tui_putc(int val) {
    js_tui_putc(val);
}

void ext_tui_clr() {
    js_tui_clr();
}

void ext_tui_w() {
    _DTOS = js_tui_width();
}

void ext_tui_h() {
    _DTOS = js_tui_height();
}

