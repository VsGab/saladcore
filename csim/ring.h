#include "defs.h"

#if NUM_REGS != 16
    #error Unexpected number of registers
#endif

typedef struct {
    wd buff[16];
    byte top;
    byte bottom;
} regs_t;

static regs_t rb;

#define REG(x) rb.buff[(rb.top-x) & 0xf]

static inline wd rget(reg x) {
    return REG(x);
}

static inline void rset(reg x, wd val) {
    REG(x) = val;
}

static inline void tspush(wd val) {
    rb.top = (rb.top+1) & 0xf;
    REG(0) = val;
}

static inline void tspop() {
    rb.top = (rb.top-1) & 0xf;
}

static inline void bspush(wd val) {
    rb.buff[rb.bottom] = val;
    rb.bottom = (rb.bottom-1) & 0xf;
}

static inline void bspop() {
    rb.bottom = (rb.bottom+1) & 0xf;
}

static inline void rb_init() {
    rb.top = 0;
    rb.bottom = 0;
    for (int i = 0; i < 16; ++i)
        rb.buff[i] = 0;
}