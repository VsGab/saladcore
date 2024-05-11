#pragma once
#include <stdio.h>
#include <assert.h>
#include "mem_layout.h"
#include "base.h"

#define UNUSED __attribute__((unused))

#define MEMORY_SIZE 16384

static byte mem[MEMORY_SIZE];
static wd * const tib_end = (void*)mem+TIB_END;
static wd * const tib_crnt = (void*)mem+TIB;
static wd * const here = (void*)mem+HERE;
static wd * const last_word_end = (void*)mem+LAST_WORD_END;
static wd * const prev_code_base = (void*)mem+CRNT_CODE_BASE;
static long cycles = 0;
static int prev_ptr = 0;

// not static, referenced in fth_common
FILE* sym_fd;


UNUSED static void print_stats(int reset_addr) {
    fprintf(stderr,"-----------------------------------\n");
    fprintf(stderr,"All words: %dB \n", (int)(*here-WORDS_BASE));
    fprintf(stderr,"All funcs: %dB \n", reset_addr-ROM_START);
    fprintf(stderr,"Last word code: %d \n", *prev_code_base);
    fprintf(stderr,"Last word end: %d \n", *last_word_end);
    fprintf(stderr,"Cycles: %ld\n", cycles);
    fprintf(stderr,"-----------------------------------\n\n");
}

UNUSED static void print_fth_state() {
    fprintf(stderr,"GC=[ ");
    for (int i=*last_word_end;i < *here; ++i) {
        fprintf(stderr, "%d ", mem[i]);
    }
    fprintf(stderr,"] | <- %d \n", *here);

    fprintf(stderr,"DS=[ ");
    int regs = MIN(get_dsz(), NUM_REGS);
    for (int i = regs-1; i >= 0; --i) {
        fprintf(stderr, "%d ", get_reg(i));
    }
    fprintf(stderr,"]\n");
}

UNUSED static void print_latest(wd ptr, FILE* fd) {
    if (prev_ptr == ptr) return;
    prev_ptr = ptr;

    wd codesz = 0;
    codesz |= mem[--ptr];
    codesz <<= 7;
    codesz |= mem[--ptr];
    byte len = mem[ptr-codesz-2];
    byte* name = &mem[ptr-codesz-len-2];
    fprintf(fd, "\ncodesz=%d range=[%d %d] word: %.*s", codesz, ptr-codesz, ptr, len, name);
    if(sym_fd)
        fprintf(sym_fd,"%.*s\t%d\t%d\n",len, name,ptr-codesz,ptr);
}

UNUSED static void error_handler(int type, wd pc) {
    static bool exiting = false;
    assert(!exiting); // prevent cyclic error
    exiting = true;
    const wd * const tib_crnt = (void*)mem+TIB;
    fprintf(stderr, "\nERROR(%s) at PC=%d TIB=%d\n",
            err_type(type), pc, *tib_crnt);
    print_fth_state();
    fflush(stdout);
    fflush(stderr);
    assert(0);
}

UNUSED static void dump_rom(const byte* mem, int rom_end,  const char* fname,
              int reset_addr) {
    byte ini[8] = {0};
    ini[0] = reset_addr & 0xff;
    ini[1] = (reset_addr >> 8) & 0xff;

    FILE* fd = fopen(fname, "wb");
    assert(fd);
    fwrite(mem, rom_end, 1, fd);
    fseek(fd, 0, SEEK_SET);
    fwrite(ini, sizeof(ini), 1, fd);
    fclose(fd);
}

UNUSED static int load_rom(byte* mem, const char* fname) {
    FILE* fd = fopen(fname, "rb");
    assert(fd);
    fseek(fd, 0, SEEK_END);
    int fsize = ftell(fd);
    fseek(fd, 0, SEEK_SET);

    fread(mem, fsize, 1, fd);
    fclose(fd);
    return fsize;
}