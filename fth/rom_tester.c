#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "enc.h"
#include "utils.h"
#include "run_common.h"

// writes go at wptr % 2
// reads are from rptr % 2
static byte sr0_buf;
static bool sr0_pending;

#define SR0_RECV 34
#define SR0_RECV_PEND 36
#define SR0_SEND 35

static byte outbuf[256];
static byte outlen;


void trace_handler(int type, wd pc, wd arg1, wd arg2) {
    if (type == INSN_TRC) ++cycles;
    if (type == ERROR_TRC) error_handler(arg1, pc);
    if (type == INT_TRC) {
        printf("\n  SW INT #%d : PC %d\n", arg1, pc);
    }
    if (type == HWINT_TRC) {
        printf("\n  HW INT #%d : PC %d => %d\n", arg1, pc, arg2);
    }
    default_trace_handler(type, pc, arg1, arg2);
}


void io_write(byte addr, wd val) {
    printf("  IO [%d] <= %d\n", addr, val);
    if (addr != SR0_SEND) return;
    assert(outlen < sizeof(outbuf)-1);
    outbuf[outlen++] = val;
    if (val == '\n') {
        fprintf(stderr, "FTH: %.*s\n", outlen, outbuf);
        outlen = 0;
    }
}

wd io_read(byte addr) {
    if (addr == SR0_RECV ) {
        sr0_pending = false;
        return sr0_buf;
    }
    if (addr == SR0_RECV_PEND)
        return sr0_pending;
    return 0;
}

void usage() {
    fprintf(stderr,"Usage: rom_tester ROM_BIN_FILE SF_FILE\n");
}

int main(int argc, char **argv) {
    if (argc != 3) {
        usage();
        return 1;
    }
    const char *rom = argv[1];
    const char *sf = argv[2];

    // in memory layout order
    load_rom(mem, rom);
    saladcore_init(mem, sizeof(mem), DS_OFFSET, RS_OFFSET, IVT_BASE,
                   io_write, io_read, trace_handler);
    saladcore_start();
    saladcore_execute(); // setup

    FILE *fd = fopen(sf, "r");
    assert(fd);

    while (true) {
        char c = fgetc(fd);
        sr0_buf = (c == EOF) ? '\n' : c;
        sr0_pending = true;
        printf("\n------------------------------\n");
        saladcore_interrupt(0); // 0 - serial recv
        saladcore_execute();
        if (c == EOF) break;
    }

    wd* reset_addr = (wd*)mem;
    print_stats(*reset_addr);
    print_fth_state();

    return 0;
}
