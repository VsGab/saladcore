#pragma once
#include "defs.h"

typedef void (*fth_func)(sf_ctx*);

// globals

#define CODEPTR ctx->wp.d[2]
#define DATAPTR ctx->wp.d[3]
#define CRNT_WORD ctx->wp.d[4]
#define TIB ctx->wp.d[5]
#define MODE ctx->wp.d[6]
#define CRNT_HEAD ctx->wp.d[7]
#define LOCALS ctx->wp.d[8]
#define LOCALS_ADJ ctx->wp.d[9]
#define JUMPS ctx->wp.d[10]
#define HT_SLOT(i) ctx->wp.d[64+i]
#define HT_MASK 63


// data layout
#define TIB_SZ 64
#define LOCALS_SZ 64
#define JUMPS_SZ 64

void saladforth_init(sf_ctx*);
void saladforth_eval_input(sf_ctx*);