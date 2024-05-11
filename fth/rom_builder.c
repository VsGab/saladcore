/*
Memory layout [begin end) ranges
0-8               RESERVED for initial PC, RSP, DSP in ROM
8-128             IO
128-ROM_SIZE      Builtin functions, then words ; dummy word at very end
ROM_SIZE-5k       IVT, reserved
5k  - 6k          PAD - reserved for user data
6k  - 8k          128x128 framebuffer
8k  - 9k          Hash table word index
9k  - 9.5k        Interpretter state, including TIB
9.5k- 11k         Data and return stacks
11k - MEM_END     User defined words

This tool takes a .sf source file and outputs a ROM image of ROM_SIZE.
There are three unused ranges in the ROM:
 - 8-128 is reserved for future uses (e.g. emulated table)
 - from end of reset code to WORDS_BASE (small)
 - from last word in ROM (+3 for skip word header) to ROM_SIZE

Currently:
ROM_SIZE = 4k

*/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "enc.h"
#include "core.h"
#include "utils.h"
#include "run_common.h"


bool is_pc_ignored(int pc) {
    // most time is spent in strcmp and findword
    return pc > addr_strcmp && pc < addr_compile;
}

void trace_handler(int type, wd pc, wd arg1, wd arg2) {
    if (type == INSN_TRC) {
        cycles += 1;
    }
    if (type == ERROR_TRC) error_handler(arg1, pc);
    if (is_pc_ignored(pc))
        printf(".");
    else
        default_trace_handler(type, pc, arg1, arg2);
}

void io_write(byte addr, wd val) {
    printf("    IO [%d] <= %d (%c)\n", addr, val, val);
    fprintf(stderr, "IO [%d] <= %d (%c)\n", addr, val, val);
}

wd io_read(byte addr) {
    // unused
    return 0;
}

void usage() {
    fprintf(stderr,"Usage: rom_builder SF_IN_PATH ROM_OUT_PATH SYM_OUT_PATH\n");
}

int main(int argc, char **argv) {
    if (argc != 4) {
        usage();
        return 1;
    }
    const char* src_path = argv[1];
    const char* rom_path = argv[2];
    const char* sym_path = argv[3];

    // open symbols file so base writes to it
    sym_fd = fopen(sym_path,"w");

    // write functions
    int func_end = add_builtin_functions(mem);

    // write builtin words
    int words_end = add_builtin_words(mem);

    // init here to end of last word (after trailing 2B)
    *here = words_end; // after word
    *last_word_end = words_end;

    // initialize core
    saladcore_init(mem, sizeof(mem), DS_OFFSET, RS_OFFSET, IVT_BASE,
                   io_write, io_read, trace_handler);

    // set find isr - ROM build only
    set_soft_irq(FIND_IRQ, addr_findword);

    FILE *src = fopen(src_path, "r");
    assert(src);

    char buff[1024];
    char c = 0;
    int line = 0;

    while (c != EOF) {
        ++line;
        bool empty = true;
        int read = 0;
        do {
            c = fgetc(src);
            buff[read++] = c;
            if (c > ' ' && c <='~') empty = false;
        } while(c != '\n' && c != EOF);
        if (empty) continue;
        if (c == EOF) read--;
        printf("\n@LINE %d: %d-%d <%.*s>\n",line,
              TIB_OFFSET,TIB_OFFSET+read,read,buff);
        fflush(stdout);

        assert(read < TIB_SIZE);
        *tib_end = TIB_OFFSET+read;
        *tib_crnt = TIB_OFFSET;
        memcpy(mem+TIB_OFFSET, buff, read);
        saladcore_reset(addr_processtib_wrap);
        saladcore_execute();

        print_latest(*last_word_end, stdout);
        printf("\n@END %d: %d-%d <%.*s>\n",line,
               TIB_OFFSET,TIB_OFFSET+read,read,buff);
    }

    int reset_addr = func_end;
    int reset_end = add_reset_code(mem, reset_addr, *last_word_end);

    print_stats(reset_addr);
    print_fth_state();

    fprintf(stderr,"-----------------------------------\n");
    int code_end = add_sys_skip_word_header(mem, *last_word_end);
    fprintf(stderr, "ROM: %d B\nUnused: [%d %d] and [%d %d] \n",
            ROM_SIZE, reset_end, WORDS_BASE, code_end, ROM_SIZE);
    dump_rom(mem, ROM_SIZE, rom_path, reset_addr);


    return 0;
}
