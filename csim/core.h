#include "defs.h"
extern void saladcore_execute();
extern void saladcore_init(byte* user_mem, wd user_mem_size,
                      wd initial_dsp, wd initial_rsp, wd ivt_base,
                      iowritehandler iowrite, ioreadhandler ioread,
                      tracehandler ontrace);
extern void saladcore_interrupt(byte idx);
extern void saladcore_reset(wd addr);
extern void saladcore_start();

extern wd get_reg(reg x);
extern wd get_dsz();
extern byte get_fsz();
extern void set_soft_irq(byte idx, wd addr);
