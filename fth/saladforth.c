#include "config.h"
#include "defs.h"
#include "enc.h"
#include "saladforth.h"

#define INS(val) {MEMSET(CODEPTR, val); CODEPTR+=1;}
#define MAX_BUILTINS 64

#define Y_FULL_FLAG   2
#define Y_VAR_SHIFT   7
#define Y_VAR_MASK    31
#define Y_VAR_UNSET   ~(31<<7)

extern int saladcore_execute(sf_ctx *ctx, int initial_pc, int limit);
extern int saladcore_resume(sf_ctx *ctx, int limit);

static const char HASH_PERM[] = {2, 0, 11, 3, 5, 9, 7, 1, 12, 8, 13, 4, 10, 14, 6, 15};
static fth_func builtins[MAX_BUILTINS];
static int inliner = 0;

void fth_alloc(sf_ctx *ctx, int sz) {
    DATAPTR -= sz;
    DPUSH(DATAPTR);
}

// ( val -- val )
void printnum(sf_ctx *ctx, int num) {
    char buf[32] = {0};
    bool neg = false;
    if (num < 0) {
        num = -num;
        neg = true;
    }

    int i = sizeof(buf);
    do {
        buf[--i] = '0' + (num % 10);
        num = num / 10;
    } while(num);
    if (neg) {
        buf[--i] = '-';
    }
    buf[--i] = ' ';
    ctx->io_write(ADDR(buf+i), sizeof(buf)-i);
}

void dump_ds(sf_ctx *ctx) {
    print("\n[");
    for (int i = DS_SZ-1 ; i >= DSP; --i) {
        printnum(ctx, DS[i]);
    }
    print(" ]");
}

void dump_tib(sf_ctx *ctx) {
    print(" ");
    int len = MEM(TIB);
    for (int i=0, p=TIB-1; i < len; p--, i++) {
        oputc(MEM(p));
    }
}

// ( -- )
void fth_token(sf_ctx *ctx) {
    unsigned char len = 0;
    char c;

    MEMSET(TIB, 0);
    // skip whitespace
    while(!ctx->io_eof()) {
        c = ctx->io_getc();
        if (c > ' ')  break;
    }
    if (ctx->io_eof() && c <= ' ') return;
    // read token
    do {
        len++;
        MEMSET(TIB-len, c);
        MEMSET(TIB, len);
        c = ctx->io_getc();
        if (c < '!' || c > 126)  break;
    } while(!ctx->io_eof() && len < TIB_SZ-1);

}

void fth_comment(sf_ctx *ctx) {
    while(!ctx->io_eof()) {
        char c = ctx->io_getc();
        if (c == '\n') break;
    }
}

void fth_parsenum(sf_ctx *ctx) {
    int len = MEM(TIB);
    int crnt = TIB - 1;
    int res = 0;
    assert(len > 0);
    do {
        char c = MEM(crnt);
        if (c < '0' || c > '9') { DPUSH(-1); return; }
        res = res * 10 + (c - '0');
        len -= 1;
        crnt -= 1;
    } while (len>0);
    DPUSH(res);
}


int fth_var(sf_ctx* ctx) {
    // TODO interface
    int count = MEM(LOCALS);
    int off = LOCALS+1;
    if (!count) return 0;

    for (int i = 0; i < count ; ++i) {
        int len = MEM(off);
        if (len != MEM(TIB))
            goto next;
        for (int j = 1 ; j <= len; ++j)
            if (MEM(TIB-j) != MEM(off+j)) goto next;
        DPUSH(count-1-i + LOCALS_ADJ);
        return 1;

        next:
        off += len + 1;
    }
    return 0;
}

int read_int24_little(sf_ctx *ctx, int addr) {
    int val = MEM(addr+2);
    val <<= 8;
    val |= MEM(addr+1);
    val <<= 8;
    val |= MEM(addr);
    return val;
}

void write_int24_little(sf_ctx *ctx, int addr, int val) {
    MEMSET(addr, val & 0xff);
    val >>= 8;
    MEMSET(addr+1, val & 0xff);
    val >>= 8;
    MEMSET(addr+2, val & 0xff);
    val >>= 8;
    assert(val == 0);
}

void fth_strhash(sf_ctx *ctx) {
    int len = MEM(DTOS);
    int h1 = 0;
    int hash = 0;

    assert(len>0);
    for (int crnt = DTOS - 1; len>0; crnt--, len--) {
        char ch = MEM(crnt);
        h1 = HASH_PERM[(h1 ^ ch) & 15];
        hash = ((hash << 1) + hash + ch + h1) & 0xff;
    }
    DPUSH(hash & HT_MASK);
}

void word_dict_insert(sf_ctx* ctx, int hash, int name_addr, int hdr_addr) {
    if (HT_SLOT(hash)) {
        write_int24_little(ctx, hdr_addr, HT_SLOT(hash));
        HT_SLOT(hash) = hdr_addr;
    } else {
        HT_SLOT(hash) = hdr_addr;
    }
}

void traverse_dict(sf_ctx *ctx, int str_addr, int hash) {
    int hdr_addr = HT_SLOT(hash);
    int name_len = MEM(str_addr);
    do {
        // N name ,1 namesz, |  3 next, 1 type, 1 codesz, 3 code-addr
        if (MEM(hdr_addr-1) != name_len) {
            goto next;
        }
        for (int i = 1 ;i <= name_len; ++i) {
            if (MEM(hdr_addr-1-i) != MEM(str_addr-i)) goto next;
        }

        DPUSH(hdr_addr);
        return;

        next:
        hdr_addr = read_int24_little(ctx, hdr_addr);
    } while(hdr_addr);
    DPUSH(0);
}


bool fth_find_word_hdr(sf_ctx *ctx) {
    DPUSH(TIB);
    fth_strhash(ctx);
    int hash = DTOS;
    DPOP();
    int str_addr = DTOS;

    if (!HT_SLOT(hash)) goto notfound;

    traverse_dict(ctx, str_addr, hash);
    if (!DTOS) {DPOP(); goto notfound;}
    int hdr = DTOS; DPOP();
    DPOP(); // TIB
    DPUSH(hdr);
    return true;

notfound:
    DPOP(); // TIB
    return false;
}

void fth_find_word_code(sf_ctx *ctx) {
    if (!fth_find_word_hdr(ctx)) {
        print(" @");
        dump_tib(ctx);
        error("word-not-found");
    }
    int hdr = DTOS; DPOP();
    int code = read_int24_little(ctx, hdr+5);
    DPUSH(code)
}

int bare_lit(sf_ctx *ctx, int val) {
    byte lits[16] = {0};
    int len = 0;
    do {
        lits[len++] = val & 127;
        val >>= 7;
    } while(val);
    for (int i = 0; i < len ; ++i) {
        INS(lits[len-i-1]);
    }
    return len; // 1+ for zx
}

int lit(sf_ctx *ctx, int val) {
    INS(ZX);
    return bare_lit(ctx, val) + 1;
}

void write_lit2(sf_ctx *ctx, int addr, int val) {
    MEMSET(addr+1, val & 127);
    val >>= 7;
    MEMSET(addr, val & 127);
    val >>= 7;
    assert(val == 0);
}

int read_int16_little(sf_ctx *ctx, int addr) {
    int val = MEM(addr+1);
    val <<= 8;
    val |= MEM(addr);
    return val;
}

void write_int16_little(sf_ctx *ctx, int addr, int val) {
    MEMSET(addr, val & 0xff);
    val >>= 8;
    MEMSET(addr+1, val & 0xff);
    val >>= 8;
    assert(val == 0);
}


bool fth_handle_word(sf_ctx *ctx) {
    if (!fth_find_word_hdr(ctx)) return false;
    int hdr = DTOS; DPOP();
    int code = read_int24_little(ctx, hdr+5);

    if (MEM(hdr+3) == 1) {
        // inline
        int len = read_int16_little(ctx, code-2) >> 1;
        for (int i = 0; i<len; ++i)
            INS(MEM(code+i));
        return true;
    }

    if (code < MAX_BUILTINS) {
        fth_func impl = builtins[code];
        assert(impl);
        impl(ctx);
    } else {
        int ret = saladcore_execute(ctx, code, CODEPTR);
        while(ret) {
            fth_func impl = builtins[ret];
            assert(impl);
            impl(ctx);
            ret = saladcore_resume(ctx, CODEPTR);
        };

    }
    return true;
    //dump_ds(ctx);
}


void fth_string(sf_ctx *ctx, const char *str, int len) {
    assert(len>0);
    assert(len<256);
    fth_alloc(ctx, 1);
    for (int i = 0, p = DTOS-1; i < len; ++i, --p) {
        MEMSET(p, str[i]);
    }
    DATAPTR -= len;
    MEMSET(DTOS, len);
}

void fth_printstr(sf_ctx *ctx) {
    int len = MEM(DTOS);
    for (int i = 0, p = DTOS-1; i < len; ++i, --p) {
        oputc(MEM(p));
    }
}

void fth_printnum(sf_ctx *ctx) {
    int val = DTOS;
    printnum(ctx, val);
}

extern void ext_tui_pos(int row, int col);
extern void ext_tui_puts(const char* addr, int len);
extern void ext_tui_putc(int val);
extern void ext_tui_clr();
extern void ext_tui_w();
extern void ext_tui_h();

void fth_tui_pos(sf_ctx *ctx) {
    ext_tui_pos(DSGET(1), DTOS);
}

void fth_tui_clr(sf_ctx *ctx) {
    ext_tui_clr();
}

void fth_tui_w(sf_ctx *ctx) {
    ext_tui_w();
}

void fth_tui_h(sf_ctx *ctx) {
    ext_tui_h();
}

void fth_tui_printstr(sf_ctx *ctx) {
    char buf[256];
    int len = MEM(DTOS);
    for (int i = 0, p = DTOS-1; i < len; ++i, --p) {
        buf[i] = MEM(p);
    }
    ext_tui_puts(buf, len);
}

void fth_tui_putc(sf_ctx *ctx) {
    ext_tui_putc(DTOS);
}

void fth_readstr(sf_ctx *ctx) {
    fth_alloc(ctx, 1);
    int len = 0;
    int p = DTOS-1;
    while(!ctx->io_eof()) {
        char c = ctx->io_getc();
        if (c == '"') break;
        MEMSET(p, c);
        p -= 1;
        len += 1;
    }
    assert(len<256);
    DATAPTR -= len;
    MEMSET(DTOS, len);
}

void fth_def_callable(sf_ctx *ctx) {
    // N name ,1 namesz, |  3 next, 1 type, 1 unused, 3 code-addr
    int code_addr = DTOS;
    fth_alloc(ctx, 8); // alloc upper part
    MEMSET(DTOS+0, 0); // next
    MEMSET(DTOS+1, 0);
    MEMSET(DTOS+2, 0);
    MEMSET(DTOS+3, 0); // type - default immediate
    MEMSET(DTOS+4, 0); // stack effect - default 0
    write_int24_little(ctx, DTOS+5, code_addr);
    MEMSET(JUMPS,0);
}

int define_c_word(sf_ctx* ctx, const char *name, int namelen) {
    /*
    - create header with name and dummy addr - includes dict linked list node
    - hash name and add to dict
    */
    INS(0);
    assert(CODEPTR < MAX_BUILTINS);
    DPUSH(CODEPTR);

    fth_def_callable(ctx); // dummy_addr hdr
    fth_string(ctx, name, namelen); // dummy_addr hdr name
    fth_strhash(ctx);      // dummy_addr hdr name hash
    int hash = DTOS;  DPOP(); // dummy_addr hdr name
    int name_addr = DTOS;  DPOP();  // dummy_addr hdr
    MEMSET(DTOS+3, 0);
    word_dict_insert(ctx, hash, name_addr, DTOS);
    DPOP();

    int xt = DTOS; DPOP();
    return xt;
}

void fth_insert(sf_ctx *ctx) {
    INS(DTOS);
    DPOP();
}

void fth_set_inline(sf_ctx* ctx) {
    MEMSET(CRNT_HEAD+3, 1);
}

void fth_tibcpy(sf_ctx *ctx) {
    int len = MEM(TIB);
    assert(len>0);
    fth_alloc(ctx, 1);
    for (int i = 0, p = DTOS-1; i < len; ++i, --p) {
        MEMSET(p, MEM(TIB-i-1));
    }
    DATAPTR -= len;
    MEMSET(DTOS, len);
}

void fth_def_callable_here(sf_ctx* ctx) {
    INS(0);
    INS(0);
    DPUSH(CODEPTR);
    fth_def_callable(ctx); // codeptr hdr
    fth_token(ctx);        // codeptr hdr
    fth_tibcpy(ctx);       // codeptr hdr name
    DPOP();                // codeptr hdr
    CRNT_HEAD = DTOS;
    CRNT_WORD = CODEPTR;
    DPOP();
    DPOP();

    dump_tib(ctx);
}

void clear_expression(sf_ctx* ctx) {
    MODE &= ~Y_FULL_FLAG;
    MODE &= Y_VAR_UNSET;
}

void fth_end_def(sf_ctx* ctx) {
    assert(CRNT_WORD);
    assert(CRNT_HEAD);

    bool is_inline = (MEM(CRNT_HEAD+3) == 1);
    if (!is_inline) INS(RET);

    int codesz = CODEPTR - CRNT_WORD;
    assert(codesz >0 && codesz < (1<<13));
    write_int16_little(ctx, CRNT_WORD-2, is_inline ? codesz << 1 | 1 : codesz << 1);

    if (!is_inline)
        INS(MEM(CRNT_HEAD+4));

    DPUSH(CRNT_HEAD-1); // name
    fth_strhash(ctx);   // name hash

    int hash = DTOS;  DPOP(); // codeptr hdr name
    int name_addr = DTOS;  DPOP();  // codeptr hdr

    word_dict_insert(ctx, hash, name_addr, CRNT_HEAD);
    clear_expression(ctx);

    #ifdef DEBUG_WORD_BYTES
    printf("\n end word @ %d [", CRNT_WORD);
    for (int i = CRNT_WORD-2; i < CODEPTR ;++i) {
        printf("%d ", MEM(i));
    }
    printf("]\n");
    #endif

}

void fth_lit(sf_ctx *ctx) {
    lit(ctx, DTOS);
    DPOP();
}

void fth_start_body(sf_ctx* ctx) {
    INS(ZX);
    DPUSH(CODEPTR);
    INS(0);
    INS(0);
    INS(JMP); // jmp
}

void fth_start_body_cond(sf_ctx* ctx) {
    INS(ZX);
    DPUSH(CODEPTR);
    INS(0);
    INS(0);
    INS(CJMP); // cjmp
}


void fth_end_body(sf_ctx* ctx) {
    int start_off = DTOS; DPOP();
    int len = CODEPTR - start_off - 3;
    write_lit2(ctx, start_off, len);
    DPUSH(len);
}


void fth_label(sf_ctx* ctx) {
    int count = MEM(JUMPS);
    int label = DTOS; DPOP();
    for (int i = 1 ; i <= count; ++i) {
        int addr = ((count) << 2) + JUMPS;
        if (MEM(addr) == label) {
            int from = read_int24_little(ctx, addr+1);
            int off = CODEPTR-from-3;
            assert(off >= 0);
            write_lit2(ctx, from, off);
        }
    }
}

void fth_goto(sf_ctx* ctx) {
    int count = MEM(JUMPS);
    int label = DTOS; DPOP();
    int addr = ((count+1) << 2) + JUMPS;
    INS(ZX);
    write_int24_little(ctx, addr+1, CODEPTR);
    MEMSET(addr, label);
    MEMSET(JUMPS, MEM(JUMPS)+1);
    INS(0);
    INS(0);
    INS(JMP);
}

void fth_while(sf_ctx* ctx) {
    // compile: len var
    int var = DTOS; DPOP();
    int len = DTOS; DPOP();
    assert(var < 32);
    INS(YRG(var));
    int lit_len = (len+4)< 128 ? 2 : 3; // zx + 1/2 lits
    int jmp_len = len+1+lit_len;
    assert(lit(ctx, jmp_len) == lit_len); // ins above + lit itself
    INS(LOOP);
}


void fth_get(sf_ctx* ctx) {
    int var = DTOS;
    DPOP();
    if (!(MODE & Y_FULL_FLAG)) {
        INS(YRG(var));
        MODE |= Y_FULL_FLAG;
        MODE &= Y_VAR_UNSET;
        MODE |= var<<Y_VAR_SHIFT;
    } else {
        INS(XRG(var));
    }
}

void fth_push_var(sf_ctx* ctx) {
    int var = DTOS;
    DPOP();
    INS(YRG(var));
    INS(YPU);
    LOCALS_ADJ += 1;
    clear_expression(ctx);
}

void fth_clear_exp(sf_ctx* ctx) {
    clear_expression(ctx);
}

void fth_drop_var(sf_ctx* ctx) {
    LOCALS_ADJ -= 1;
    INS(DROP);
}

void fth_ypush(sf_ctx* ctx) {
    INS(YPU);
    LOCALS_ADJ += 1;
    clear_expression(ctx);
}

void fth_xpush(sf_ctx* ctx) {
    INS(XPU);
    LOCALS_ADJ += 1;
    clear_expression(ctx);
}

void fth_upd(sf_ctx* ctx) {
    int var = (MODE >> Y_VAR_SHIFT) & Y_VAR_MASK;
    INS(YRS(var));
    clear_expression(ctx);
}

void fth_defvar(sf_ctx* ctx) {
    fth_token(ctx);
    int len = MEM(TIB);

    int count = MEM(LOCALS);
    int off = 1;
    for (int i = 0 ;i < count ; ++i) {
        off += MEM(LOCALS+off) + 1;
    }
    assert(off+len+1 < 128);
    MEMSET(LOCALS+off, len);
    for (int i = 1 ; i <= len; ++i) MEMSET(LOCALS+off+i, MEM(TIB-i));
    MEMSET(LOCALS, MEM(LOCALS)+1);
    INS(YPU);
    clear_expression(ctx);
}

void fth_setvar(sf_ctx* ctx) {
    fth_token(ctx);
    assert(fth_var(ctx));
    int var = DTOS;
    DPOP();
    INS(YRS(var));
}

void fth_bind(sf_ctx* ctx) {
    MEMSET(LOCALS, 0);
    LOCALS_ADJ = 0;
    clear_expression(ctx);
    int count = 0, len = 0, off = 1;
    do {
        fth_token(ctx);
        len = MEM(TIB);
        if (len == 1 && MEM(TIB-1) == '|') break;
        assert(off+len+1 < 128);

        if (fth_find_word_hdr(ctx)) {
            print(" @");
            DPOP();
            dump_tib(ctx);
            error("var-shadows-word");
        }

        MEMSET(LOCALS+off, len);
        for (int i = 1 ; i <= len; ++i) MEMSET(LOCALS+off+i, MEM(TIB-i));
        off += len + 1;
        count += 1;
    } while(len);
    MEMSET(LOCALS, count);
}

void fth_tick(sf_ctx* ctx) {
    fth_token(ctx);
    if (!MEM(TIB)) return;
    fth_find_word_code(ctx);
}

void fth_quote(sf_ctx* ctx) {
    int len = 0;
    while(!ctx->io_eof()){
        fth_token(ctx);
        if (!MEM(TIB)) break;
        if (MEM(TIB) == 1 && MEM(TIB-1) == ']') break;
        fth_parsenum(ctx);
        if (DTOS < 0) {
            // insert word
            DPOP(); // discard -1
            fth_find_word_code(ctx); // push code addr
            int xt_lit = DTOS << 1 | 1;
            DPOP();
            DPUSH(xt_lit);
        } else {
            int lit_lit = DTOS << 1;
            DPOP();
            DPUSH(lit_lit);
        }
        MEMSET(TIB, 0);
        len++;
    }
   DPUSH(len);
}

void fth_inline_word(sf_ctx* ctx) {
    int xt = DTOS; DPOP();
    int len = read_int16_little(ctx, xt-2);
    len >>= 1;
    for (int i = 0; i<len; ++i)
        INS(MEM(xt+i));
}

void fth_insert_quote(sf_ctx* ctx) {
    int len = DTOS; DPOP();
    for (int i = len-1 ; i >=0 ; --i) {
        int val = DSGET(i); // could be done with lds instruction
        if (val & 1) {
            // word
            int code = val >> 1;
            int len = read_int16_little(ctx, code-2);
            bool is_inline = len & 1;
            if (is_inline) {
                // call inliner word
                lit(ctx, code);
                INS(XPU);
                lit(ctx, inliner);
                INS(SIG);
            } else {
                // call in compiled word
                lit(ctx, code);
                INS(code < MAX_BUILTINS ? SIG : CALL);
                int delta = MEM(code + (len>>1));
                delta = delta & 1 ? -(delta>>1) : (delta>>1);
                LOCALS_ADJ += delta;
            }

        } else {
            lit(ctx, val >> 1);
            INS(XPU);
            LOCALS_ADJ += 1;
        }
    }
    for (int i = 0 ; i < len; ++i) DPOP();
}

void fth_postpone_quote(sf_ctx* ctx) {
    INS(ZX);
    int len = DTOS; DPOP();
    for (int i = len-1 ; i >=0 ; --i) {
        int val = DSGET(i); // could be done with lds instruction
        bare_lit(ctx, val);
        INS(XPU);
    }
    for (int i = 0 ; i < len; ++i) DPOP();
    bare_lit(ctx, len);
    INS(XPU);
}

void fth_stack_effect(sf_ctx* ctx) {
    assert(DTOS > -63 && DTOS < 63);
    int delta = DTOS < 0 ? ((-DTOS)<<1 | 1) : (DTOS<<1);
    MEMSET(CRNT_HEAD+4, delta);
    DPOP();
}

#define BUILTIN_WORD(name, func) (\
    builtins[define_c_word(ctx, name, sizeof(name)-1)]=func,\
    CODEPTR)

void init_builint_words(sf_ctx* ctx) {
    // base
    BUILTIN_WORD("b.", fth_insert);
    BUILTIN_WORD(":", fth_def_callable_here);
    BUILTIN_WORD(";", fth_end_def);
    BUILTIN_WORD("inl", fth_set_inline);
    BUILTIN_WORD(".", fth_lit);
    BUILTIN_WORD("\\", fth_comment);
    BUILTIN_WORD("[", fth_quote);
    BUILTIN_WORD("_", fth_insert_quote);
    BUILTIN_WORD("'", fth_tick);
    BUILTIN_WORD("q.", fth_postpone_quote);
    BUILTIN_WORD("d.", fth_stack_effect);
    inliner = BUILTIN_WORD("w.", fth_inline_word);

    // control flow
    BUILTIN_WORD("-[", fth_start_body);
    BUILTIN_WORD("]-", fth_end_body);
    BUILTIN_WORD("?[", fth_start_body_cond);
    BUILTIN_WORD("while", fth_while);
    BUILTIN_WORD("#", fth_label);
    BUILTIN_WORD("#goto", fth_goto);

    // expressions
    BUILTIN_WORD(",", fth_get);
    BUILTIN_WORD("=>", fth_setvar);
    BUILTIN_WORD("y>:", fth_defvar);
    BUILTIN_WORD("|", fth_bind);
    BUILTIN_WORD("!", fth_upd);
    BUILTIN_WORD("y>", fth_ypush);
    BUILTIN_WORD("x>", fth_xpush);
    BUILTIN_WORD(">", fth_push_var);
    BUILTIN_WORD(";;", fth_clear_exp);
    BUILTIN_WORD("v", fth_drop_var);

    // utils
    BUILTIN_WORD("\"", fth_readstr);
    BUILTIN_WORD("puts", fth_printstr);
    BUILTIN_WORD("prnt", fth_printnum);

    // TUI
    BUILTIN_WORD("tui/puts", fth_tui_printstr);
    BUILTIN_WORD("tui/putc", fth_tui_putc);
    BUILTIN_WORD("tui/pos", fth_tui_pos);
    BUILTIN_WORD("tui/clr", fth_tui_clr);
    BUILTIN_WORD("tui/w", fth_tui_w);
    BUILTIN_WORD("tui/h", fth_tui_h);
    CODEPTR = MAX_BUILTINS;
}

void saladforth_init(sf_ctx* ctx) {
    DSP = DS_SZ;
    RSP = RS_SZ;
    CODEPTR = 1; // dummy slots for builtins
    TIB = ctx->mem_sz-1;
    LOCALS = ctx->mem_sz-TIB_SZ-LOCALS_SZ;
    JUMPS = ctx->mem_sz-TIB_SZ-LOCALS_SZ-JUMPS_SZ;
    DATAPTR = ctx->mem_sz-TIB_SZ-LOCALS_SZ-JUMPS_SZ;
    CRNT_WORD = 0;
    CRNT_HEAD = 0;
    MODE = 0;
    MEMSET(TIB, 0);
    init_builint_words(ctx);
}



void saladforth_eval_input(sf_ctx* ctx) {
    print(" EVAL ");
    while(!ctx->io_eof()){
        fth_token(ctx);
        if (!MEM(TIB)) break;
        fth_parsenum(ctx);
        if (DTOS < 0) {
            DPOP();
            if (!fth_handle_word(ctx)) {
                if (!fth_var(ctx)) {
                    print(" @");
                    dump_tib(ctx);
                    error("name-not-found");
                }
            }
        }
        MEMSET(TIB, 0);
    }
    print(" OK");
    dump_ds(ctx);
}