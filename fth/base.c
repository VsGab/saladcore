#define _GNU_SOURCE
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "enc.h"
#include "base.h"

int addr_readnum, addr_isdigit, addr_memcpy;
int addr_skip, addr_token, addr_printnum;
int addr_strcmp, addr_findword, addr_compile;
int addr_emitbyte, addr_defword, addr_enddef;
int addr_processtib, addr_codelimits;
int addr_processtib_wrap;

extern FILE* sym_fd;

#define SYM(name,start,end) {if(sym_fd) \
    fprintf(sym_fd,"%s\t%d\t%d\n",name,(int)(start),(int)(end));}
#define FUNC(code,dst,mem) dst-mem ; \
    {SYM(#code,dst-mem,dst-mem+sizeof(code));\
    assert(sizeof(code)+(dst-mem) < WORDS_BASE);\
    dst = mempcpy(dst,code,sizeof(code));}
#define FUNC_ADDR(dst)  ()
#define WORD(code,dst) {\
    SYM(#code,dst-mem,dst-mem+sizeof(code));\
    dst = mempcpy(dst,code,sizeof(code));}


int add_builtin_functions(byte* mem) {
    byte* codeptr = mem+ROM_START;

    byte func_readnum[] = {
        lit(2),
        fset(), // (limit start -- number)
        lit(0), // limit crnt res
        // -------
        rpick(1), // loop start  <----------|
        iloadb(), // limit crnt res rawval  |
        lit(0xf),
        and_(),
        rswap(2), // limit crnt val res     |
        // replace *10 for minimal version  |
        lit(10),
        mul(),
        add(),
        //dup_(), lit(3), shl(), swap(),
        //lit(1), shl(), add(), add(),
        // end replacement +5               |
        // limit crnt res*10+val            |
        rpick(1),
        inc(),
        dup_(),
        rmov(1),
        rpick(0), // limit crnt res crnt    |
        cond(CLESS),
        drop(), // limit crnt res           |
        //lit(20), // mul replacement
        lit(15),
        loop(), // -------------------------|
        // limit crnt res
        nip(),
        nip(),
        // res
        ret(),
    };
    addr_readnum = FUNC(func_readnum,codeptr,mem);

    byte func_isdigit[] = {
        // (char -- 127 (T) / 126 (F) )
        lit(48),
        cond(CGEQ),  // c >= 48
        lit(58),
        cond(CLESS),
        drop(), // drop char in both casess
        lit(2),
        jmp(), // if c >= 48 AND c < 58 -> true ----|
         lit(126), // res is and-ed, unset only lsb |                            |
         ret(),
        lit(T), // true ret case   <----------------|
        ret(),
    };
    addr_isdigit = FUNC(func_isdigit,codeptr,mem);

    byte func_skipspace[] = {
        // (limit start -- limit start)
        // loop start  <----------------------------------|
        over(), // limit start limit                      |
        cond(CGEQ),  // if crnt >= limit crnt jump to ret |
        lit(10),
        jmp(), // ----------------------------------|     |
        dup_(), // limit crnt crnt                  |     |
        iloadb(), // limit crnt char                |     |
        lit(33),  // limit crnt char 33             |     |
        cond(CGEQ),
        drop(),  // limit crnt                      |     |
        lit(3),
        jmp(), // if (!space) jump to ret ----------|     |
        inc(), // limit crnt+1                      |     |
        lit(12),
        loop(), // ---------------------------------------|
        ret(), // <---------------------------------|
    };
    addr_skip = FUNC(func_skipspace,codeptr,mem);

    byte func_token[] = {
        // ( -- tok-start flags tok-end)
        lit(hb(TIB_END)),
        lit(lb(TIB_END)),
        iloadw(),
        lit(hb(TIB)),
        lit(lb(TIB)),
        iloadw(),
        lit(2),
        fset(),
        // tib-end tib-crnt

        // skip leading spaces
        lit(hb(addr_skip)),
        lit(lb(addr_skip)),
        call(),
        lit(1), // limit start flags
        rpick(1), // limit start flags crnt

        // loop start
        rpick(0), // limit start flags crnt limit <---|
        cond(CGEQ), // if crnt >= limit jump to end
        lit(16),
        jmp(), // -----------------------------------------------|
        dup_(), // limit start flags crnt crnt                   |
        iloadb(), // limit start flags crnt char      |          |
        lit(33),
        cond(CLESS),
        lit(9),
        jmp(), // if ch is space jump forward to end drop ---|   |
        lit(hb(addr_isdigit)),
        lit(lb(addr_isdigit)),
        call(),   // limit start flags crnt is_dig    |      |   |
        rpick(2), // limit start flags crnt is_dig flags     |   |
        and_(), // unset digit flag if not digit      |      |   |
        rmov(2), // update flags                      |      |   |
        inc(), // limit start flags' crnt+1           |      |   |
        lit(17),
        loop(), // jump to beginning -----------------|
        drop(), // reached space, clean it up  <-------------|   |

        // update tib-crnt in mem
        lit(hb(TIB)), // <---------------------------------------|
        lit(lb(TIB)),
        snw(),

        // tib-end tok-start flags tok-end
        lit(3),
        fkeep(),
        // tok-start flags tok-end
        ret(),
    };
    addr_token = FUNC(func_token,codeptr,mem);


    byte func_memcpy[] = {
        lit(3),
        fset(), // (src-end src-start dst-start -- src-end dst-end )
        swap(), // src-end dst src-start
        // loop start
        rpick(0), // src-end dst src src-end  <---|
        cond(CLESS),
        lit(2),
        jmp(),
         drop(),
         ret(), // src-end dst-end                |
        // src-end dst src
        mcpb(),
        inc(), // src-end dst src+1               |
        swap(), // src-end src+1 dst              |
        inc(), // src-end src+1 dst+1             |
        swap(), // src-end dst+1 src+1            |
        lit(11),
        loop(), // -------------------------------|
    };
    addr_memcpy = FUNC(func_memcpy,codeptr,mem);

    byte func_defword[] = {
        // ( -- )
        // reset here in case anything generated outside defs
        lit(hb(LAST_WORD_END)),
        lit(lb(LAST_WORD_END)),
        iloadw(),
        lit(hb(HERE)),
        lit(lb(HERE)),
        snw(), // initial-here

        // load tib state
        lit(hb(TIB_END)),
        lit(lb(TIB_END)),
        iloadw(),
        lit(hb(TIB)),
        lit(lb(TIB)),
        iloadw(), // initial-here tib-end tib-crnt
        lit(3),
        fset(),

        lit(hb(addr_token)),
        lit(lb(addr_token)),
        call(),

        // here tib-end tib-crnt | name-start flags name-end
        nip(), // drop flags
        // update tib with name-end
        lit(hb(TIB)),
        lit(lb(TIB)),
        snw(),

        rmov(1), // name-end -> 1
        rmov(2), // name-start -> 2
        // here name-end name-start

        rpick(0),

        // here name-end name-start here
        lit(hb(addr_memcpy)),
        lit(lb(addr_memcpy)),
        call(), // here src-end dst-end
        nip(), // drop tib pointer, leave only new here
        // name-start name-end

        // write name len
        dup_(), // name-start name-end name-end
        rswap(0), // name-end name-end name-start
        sub(),   // name-end name-len
        stb(),
        inc(),

        // here at tag byte ptr - default 0
        lit(0),
        stb(),
        inc(), // here at code start ptr

        // update here
        lit(hb(HERE)),
        lit(lb(HERE)),
        snw(),

        // set crnt code base
        lit(hb(CRNT_CODE_BASE)),
        lit(lb(CRNT_CODE_BASE)),
        snw(),

        lit(0),
        fkeep(),
        ret(),
    };
    addr_defword = FUNC(func_defword, codeptr,mem);

    byte func_enddef[] = {
        // ( -- )
        lit(hb(HERE)),
        lit(lb(HERE)),
        iloadw(),
        // crnt-here

        lit(hb(ret())),
        lit(lb(ret())),
        stb(), // func ret op
        inc(),

        lit(hb(CRNT_CODE_BASE)),
        lit(lb(CRNT_CODE_BASE)),
        iloadw(),
        // here code-start
        over(),
        swap(),
        sub(), // len is here - code-start
        // here code-len

        swap(), // code-len here

        // write len LSB
        over(),
        lit(127),
        and_(),
        stb(),
        inc(),

        swap(), // here code-len

        // write len MSB
        lit(7),
        shr(),
        stb(),
        inc(), // final-here

        // write addr to both HERE and LAST_WORD_END
        lit(hb(HERE)),
        lit(lb(HERE)),
        snw(), // update HERE
        lit(hb(LAST_WORD_END)),
        lit(lb(LAST_WORD_END)),
        snw(), // LAST_WORD_END
        drop(),

        ret(),
    };
    addr_enddef = FUNC(func_enddef, codeptr,mem);


    byte func_strcmp[] = {
        // ( name-start tok-end tok-start  -- is_same )
        lit(3),
        fset(),
        // assumes token at least 1 char long
        // loop start
        dup_(), // <------------------------------------|
        iloadb(),
        rpick(0),
        iloadb(),
        // name-crnt tok-end tok-crnt tok-ch name-ch    |
        cond(CEQ), // if not equal jump over ret false  |
        lit(4),
        jmp(),
          lit(0),
          fkeep(),
          lit(F),
          ret(),
        // name-crnt tok-end tok-crnt tok-ch            |
        drop(),
        inc(), // inc tok-crnt first                    |
        over(), //name-start tok-end tok-crnt tok-end   |
        cond(CLESS), // if crnt < end continue          |
        lit(4),
        jmp(),
          // if end reached return true                 |
          lit(0),
          fkeep(),
          lit(T),
          ret(),
        // name-crnt tok-end tok-crnt                   |
        rpick(0),
        inc(),
        rmov(0),
        // continue loop
        lit(24),
        loop(), // -------------------------------------|
    };
    addr_strcmp = FUNC(func_strcmp, codeptr,mem);

    // reads code length footer and returns code range
    byte func_wordend[] = {
        // ( here -- code-end code-start )
        dec(),
        dup_(),
        iloadb(),
        // here-1 len-msb
        lit(7),
        shl(),
        swap(),
        dec(),
        // len-msb here-2
        swap(),
        over(),
        // here-2 len-msb here-2
        iloadb(),
        or_(),
        // code-end len
        over(),
        swap(),
        sub(),
        // code-end code-start
        ret(),
    };
    addr_codelimits = FUNC(func_wordend, codeptr,mem);

    // traverses words from last_word_end to words_base
    // strcmp's token to word name
    byte func_findword[] = {
        // (tok-end tok-start -- code-end code-start)
        lit(2),
        fset(),
        // adjust tok end to addr of last char
        rpick(0),
        rpick(1),
        sub(),
        // tok-end tok-start tok-len
        rswap(0),
        // tok-len tok-start tok-end

        lit(hb(LAST_WORD_END)),
        lit(lb(LAST_WORD_END)),
        iloadw(),
        // tok-len tok-start tok-last here

        // loop begin <----------------------|
        lit(hb(addr_codelimits)),//          |
        lit(lb(addr_codelimits)),//          |
        call(), // uses here or name-start   |
        // tok-len tok-start tok-last code-end code-start
        // check name matches
        dup_(), // keep code-start for return|
        lit(2),
        sub(), // skip tag byte              |
        // tok-len tok-start tok-last code-end code-start name-len-addr
        dup_(),
        iloadb(),//                          |
        // tok-len tok-start tok-last code-end code-start name-len-addr name-len
        rpick(0),
        cond(CNEQ),//                        |
        sub(), // tok-len tok-start tok-last code-end code-start name-start
        lit(13),
        jmp(),// -------------------------------------------|

        // tok-len == name-len               |              |
        // tok-len tok-start tok-last code-end code-start name-start
        dup_(), // keep copy of name-start as strcmp consumes it
        rpick(2),
        rpick(1),
        // tok-len tok-start tok-last code-end code-start name-start
        // ... name-start tok-last tok-start |              |
        lit(hb(addr_strcmp)),//              |              |
        lit(lb(addr_strcmp)),//              |              |
        call(),

        // tok-len tok-start tok-last code-end code-start name-start is-match
        cond(CZERO),
        lit(4),
        jmp(), // if
        // is-match != 0 -> found word       |              |
            drop(), // drop name-start       |              |
            lit(2),
            fkeep(),
            ret(),
        // is-match == 0 -> continue loop    |              |
        // tok-len tok-start tok-last code-end code-start name-start

        // jump target if len mismatch   <---|--------------|
        // drop code range, moving on to next word
        nip(),
        nip(),
        // tok-len tok-start tok-last name-start

        // check words base reached          |
        lit(hb(WORDS_BASE+1)),
        lit(lb(WORDS_BASE+1)),
        cond(CGEQ),
        lit(4),
        jmp(),
            lit(0),
            fkeep(),
            lit(0),
            doint(), // if SOH-addr == WORDS_OFFSET, word not found

         // tok-len tok-start tok-last here  |
        lit(37),//                           |
        loop(), // --------------------------|
    };
    addr_findword = FUNC(func_findword, codeptr,mem);

    // inserts byte at here and increments it
    byte func_ins[] = {
        // (val -- )
        // load code generation pointer
        lit(hb(HERE)),
        lit(lb(HERE)),
        iloadw(), // val addr
        swap(), // addr val

        // low literal
        stb(), // addr
        inc(), // addr+1

        // update CG_PTR
        lit(hb(HERE)),
        lit(lb(HERE)),
        snw(),
        drop(),
        ret(),
    };
    addr_emitbyte = FUNC(func_ins,codeptr,mem);

    // main token eval logic called until TIB is fully processed
    // it depends on the inlining variable
    // 1 if not inlining
    //   - put numbers on stack
    //   - calls words immediately (if no-inline unset)
    // 2 if inlining
    //   - call ins function on number
    //   - memcpy word body without trailing ret to HERE addr
    byte func_tokeval[] = {
        // check end of TIB, else scan token
        lit(hb(addr_token)),
        lit(lb(addr_token)),
        call(),
        // tok-start flags tok-end
        lit(3),
        fset(),

        rpick(0), // pick scan limit for cmp
        cond(CNEQ),
        lit(3),
        jmp(), // tok-end < tib end -> jump over ret
          lit(0),
          fkeep(),
          ret(),
        // tok-start flags tok-end
        // check number flag
        rpick(1),
        lit(1),
        and_(), // check number flag is set
        cond(CZERO),
        // tok-start flags tok-end
        nip(), // drop flags
        swap(), // tok-end tok-start
        lit(14),
        jmp(), // if not number jump to word case -------|
        // tok-end tok-start                             |

        // number case                                   |
        lit(hb(addr_readnum)),
        lit(lb(addr_readnum)),
        call(), // (limit,start -- number)               |
        // if inlining call ins else return              |
        lit(hb(INLINING)),
        lit(lb(INLINING)),
        iloadb(),
        cond(CNZERO),
        lit(1),
        jmp(),
          ret(),
        lit(hb(addr_emitbyte)),
        lit(lb(addr_emitbyte)),
        call(),
        ret(),

        // word case                                     |
        // softirq indirected findword                   |
        lit(FIND_IRQ), // <------------------------------|
        doint(), // code-end code-start

        dup_(),  // code-end code-start code-start
        dec(),   // code-end code-start word-tag-addr
        iloadb(), // code-end code-start word-tag
        lit(64),
        and_(),  // code-end code-start no-inline-flag
        cond(CZERO), // no-inline-flag=0 ...

        lit(hb(INLINING)),
        lit(lb(INLINING)),
        iloadb(),
        cond(CNZERO),// ... AND inline-next != 0
        lit(5),
        // if no-inline-flag=0 AND inline-next!=0 skip inline case
        jmp(),
          nip(), // drop code-end
          lit(FUNSET),
          fset(), // disable frame
          call(), // call word immediately
          ret(),

        // inline case
        // remove last insn from word code - ret
        // code-end code-start
        swap(),
        dec(),
        swap(), // code-end-1 code-start

        lit(hb(HERE)),
        lit(lb(HERE)),
        iloadw(), // code-end-1 code-start here
        lit(hb(addr_memcpy)),
        lit(lb(addr_memcpy)),
        call(),  // code-end-1 new-nere
        nip(),   // new-here
        lit(hb(HERE)),
        lit(lb(HERE)),
        snw(), // update here
        drop(),

        ret(),
    };
    addr_compile = FUNC(func_tokeval, codeptr,mem);

    byte func_eval[] = {
        // main loop
        lit(FUNSET), // <---------|
        fset(),
        lit(hb(addr_compile)),
        lit(lb(addr_compile)),
        call(),
        lit(hb(TIB)),
        lit(lb(TIB)),
        iloadw(),
        lit(hb(TIB_END)),
        lit(lb(TIB_END)),
        iloadw(), // tib tib-end  |
        cond(CLESS), // tib       |
        drop(),
        lit(13),
        loop(), // ---------------|

        // reset tib
        lit(hb(TIB_OFFSET)),
        lit(lb(TIB_OFFSET)),
        nop(),
        lit(hb(TIB)),
        lit(lb(TIB)),
        snw(), // tib var <= tib offset ct
        lit(hb(TIB_END)),
        lit(lb(TIB_END)),
        snw(), // tib end var <= tib offset ct
        drop(),
        // reset frame
        lit(FUNSET),
        fset(),

        ret(),
    };
    addr_processtib = FUNC(func_eval, codeptr,mem);


    byte func_eval_wrap[] = {
        lit(hb(addr_processtib)),
        lit(lb(addr_processtib)),
        call(),
        halt(),
    };
    addr_processtib_wrap = FUNC(func_eval_wrap, codeptr, mem);

    return codeptr-mem;
}

int add_reset_code(byte* mem, int offset, int last_word_end) {
    int boot_word_name = offset+22;
    int boot_word_name_end = offset+24;
    int find_isr = IVT_BASE + (FIND_IRQ<<ISR_SHIFT);
    // at reset use last valid word end to lookup ^B word
    // this must write the dummy word size in the first 2B of RAM
    //   then update last_word_end
    byte reset_code[] = {
        // setup last word
        lit(hb(last_word_end)), // 0
        lit(lb(last_word_end)),
        nop(),
        lit(hb(LAST_WORD_END)),
        lit(lb(LAST_WORD_END)),
        snw(),
        drop(),
        // find and call boot word
        lit(hb(boot_word_name_end)),
        lit(lb(boot_word_name_end)),
        nop(),
        lit(hb(boot_word_name)), // 10
        lit(lb(boot_word_name)),
        nop(),
        lit(hb(addr_findword)),
        lit(lb(addr_findword)),
        nop(),

        // setup findword int 127
        lit(hb(find_isr)),
        lit(lb(find_isr)),
        snw(),
        // call findword
        call(),
        nip(), // drop code-len - off 20
        // finall call boot word
        ljmp(), // 21
        '^', // 22
        '^', // 23
    };
    fprintf(stderr,"\nReset code at [%d,%d]\n",
            offset, offset+(int)sizeof(reset_code));
    assert(offset+sizeof(reset_code) < WORDS_BASE);
    memcpy(mem+offset, reset_code, sizeof(reset_code));
    return offset+sizeof(reset_code);
}

int add_builtin_words(byte* mem) {
    byte* words_codeptr = mem+WORDS_BASE;

    byte word_col[] = {
        'c', ':', 2, NOINL,
        nop(),
        lit(hb(addr_defword)),
        lit(lb(addr_defword)),
        call(),
        ret(),
        lit(5), lit(0),
    };
    WORD(word_col,words_codeptr)

    byte word_semicol[] = {
        ';', 1,  NOINL,
        nop(),
        lit(hb(addr_enddef)),
        lit(lb(addr_enddef)),
        call(),
        ret(),
        lit(5), lit(0),
    };
    WORD(word_semicol,words_codeptr)

    byte word_ins[] = {
        'i','n','s', 3, 0,
        lit(hb(addr_emitbyte)),
        lit(lb(addr_emitbyte)),
        call(),
        ret(),
        lit(4), lit(0), // lit is codesz
    };
    WORD(word_ins,words_codeptr)

    byte word_inl_start[] = {
        '[', 1, NOINL,
        lit(hb(INLINING)),
        lit(lb(INLINING)),
        nop(),
        lit(T),
        stb(),
        drop(),
        ret(),
        lit(7), lit(0), // lit is codesz
    };
    WORD(word_inl_start,words_codeptr)

    byte word_inl_end[] = {
        ']', 1, NOINL,
        lit(hb(INLINING)),
        lit(lb(INLINING)),
        nop(),
        lit(0),
        stb(),
        drop(),
        ret(),
        lit(7), lit(0), // lit is codesz
    };
    WORD(word_inl_end,words_codeptr)

    byte word_word[] = {
        'w', 'o', 'r', 'd', 4, 0,
        lit(hb(addr_token)),
        lit(lb(addr_token)),
        call(),
        ret(),
        lit(4), lit(0), // lit is codesz
    };
    WORD(word_word,words_codeptr)

    byte word_find[] = {
        'f', 'i', 'n', 'd', 4, 0,
        lit(127),
        doint(),
        nop(),
        lit(3), lit(0), // lit is codesz
    };
    WORD(word_find,words_codeptr)

    byte word_ifind[] = {
        'i', 'f', 'i', 'n', 'd', 5, 0,
        lit(hb(addr_findword)),
        lit(lb(addr_findword)),
        call(),
        ret(),
        lit(4), lit(0), // lit is codesz
    };
    WORD(word_ifind,words_codeptr)

    byte word_call[] = {
        'e','x','e', 3, NOINL,
        ljmp(),
        lit(1), lit(0), // lit is codesz
    };
    WORD(word_call,words_codeptr)

    byte word_eval[] = {
        'e','v','a','l', 4, 0,
        lit(hb(addr_processtib)),
        lit(lb(addr_processtib)),
        call(),
        ret(),
        lit(4), lit(0), // lit is codesz
    };
    WORD(word_eval,words_codeptr)

    byte word_strcmp[] = {
        'c','m','p','s', 4, 0,
        lit(hb(addr_strcmp)),
        lit(lb(addr_strcmp)),
        call(),
        ret(),
        lit(4), lit(0), // lit is codesz
    };
    WORD(word_strcmp,words_codeptr)

    byte word_wordend[] = {
        'w','c','o','d','e', 5, 0,
        lit(hb(addr_codelimits)),
        lit(lb(addr_codelimits)),
        call(),
        ret(),
        lit(4), lit(0), // lit is codesz
    };
    WORD(word_wordend,words_codeptr)


    return words_codeptr - mem;
}

int add_sys_skip_word_header(byte* mem, int words_end) {
    byte* words_codeptr = mem+words_end;
    byte sys_skip_word[] = {
        0x18, 1, 0
    };
    WORD(sys_skip_word,words_codeptr)
    return words_end+sizeof(sys_skip_word);
}
