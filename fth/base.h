#pragma once
#include "defs.h"
#include "mem_layout.h"

extern int addr_readnum, addr_isdigit, addr_memcpy;
extern int addr_skip, addr_token;
extern int addr_strcmp, addr_findword, addr_compile;
extern int addr_emitbyte, addr_defword, addr_enddef;
extern int addr_processtib, addr_processtib_wrap;

int add_builtin_functions(byte *mem);
int add_builtin_words(byte* words_base);
int add_sys_skip_word_header(byte* mem, int words_end);
int add_reset_code(byte* mem,
                    int offset,
                    int unused_rom_bytes);

void dump_addresses(const char* fname, int reset_end);


#define T 127
#define F 0
#define NOINL 64
#define FIND_IRQ 127

