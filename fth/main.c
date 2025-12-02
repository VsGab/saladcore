#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "defs.h"
#include "saladforth.h"

#define DISP 128
#define IOBUF_SZ 4
#define MEM_SZ 1<<16

#define _DTOS ctx.wp.d[ctx.wp.d[0]]

static byte main_mem[MEM_SZ];
static sf_ctx ctx;

// IO
static byte in_buff[IOBUF_SZ];
static int in_sz = 0;
static int in_ptr = 0;

char my_getc() {
    return fgetc(stdin);
}

bool my_eof() {
    return feof(stdin);
}

int io_read(intptr_t addr, int len) {
    return 0; //unused
}

void io_write(intptr_t addr, int len) {
    fwrite((const char*)addr, len, 1, stdout);
}

void on_error(intptr_t addr, int len) {
    printf(" ERR: %.*s\n", len, (const char*)addr);
    printf(" STACK: [");
    for (int i = DS_SZ-1; i >= ctx.wp.d[0]; --i) printf(" %d", ctx.wp.d[i]);
    printf(" ]\n");
    abort();
}

void ext_tui_pos(int row, int col) {
    printf(" TUI:POS(%d %d)", row, col);
}
void ext_tui_puts(const char* addr, int len) {
    printf(" TUI:PUTS(%.*s)", len, addr);
}
void ext_tui_putc(int val) {
    printf(" TUI:PUTC(%c)", (char)val);
}
void ext_tui_clr() {
    printf(" TUI:CLR");
}
void ext_tui_w() {
    printf(" TUI:W");
    _DTOS = 128;
}
void ext_tui_h() {
    printf(" TUI:H");
    _DTOS = 128;
}

int main() {
    ctx.io_read = io_read;
    ctx.io_write = io_write;
    ctx.on_error = on_error;
    ctx.io_getc = my_getc;
    ctx.io_eof = my_eof;
    ctx.mem = main_mem;
    ctx.mem_sz = sizeof(main_mem);
    saladforth_init(&ctx);
    saladforth_eval_input(&ctx);
    printf(" OK\n");
    return 0;
}

