#include "core.h"
#include "enc.h"
#include "mem_layout.h"
#define DISP 128

extern void js_io_write(byte addr, wd val);
extern wd js_io_read(byte addr);
extern void js_error(int type, wd pc);

static byte main_mem[16384];
static byte rgba_fb[DISP*DISP*4];
static byte color1[3] = {22, 57, 69};
static byte color2[3] = {191, 208, 208};
static byte* colors[2] = {color1, color2};

#define MEMCPY(dst,bytes) {for (int i = 0; i < sizeof(bytes); ++i)\
                            mem[dst+i] = bytes[i];}

int main_mem_addr() {
    return (int)&main_mem;
}

int main_mem_bytes() {
    return sizeof(main_mem);
}

void trace_handler(int type, wd pc, wd arg1, wd arg2) {
    if (type == ERROR_TRC) js_error(arg1, pc);
    // only errors sent to js
}

void saladcore_js_init() {
    saladcore_init(main_mem, sizeof(main_mem), DS_OFFSET, RS_OFFSET, IVT_BASE,
                   js_io_write, js_io_read, trace_handler);

    saladcore_start();
    saladcore_execute(); // setup
}

void saladcore_js_exec() {
    saladcore_execute();
}

void saladcore_js_int(byte i) {
    saladcore_interrupt(i);
}

int fb_addr() {
    return (int)&rgba_fb;
}

int fb_bytes() {
    return sizeof(rgba_fb);
}

int fb_size() {
    return DISP;
}

#define FB_OFFSET 6144
#define FB_BYTES DISP*DISP/8
#define FB_W DISP/4
#define FB_H DISP/2

void convert_fb_to_rgba() {
    for (int x = 0; x < FB_W; ++x)
    for (int y = 0; y < FB_H; ++y) {
        byte blk = main_mem[FB_OFFSET + x*FB_H + y];
        for (int i = 0 ; i < 8; ++i) {
            int fy = x*4 + i % 4;
            int fx = y*2 + i / 4;
            int fb_off = (fx*DISP+fy)*4;
            byte b = !!(blk & (1<<i));
            rgba_fb[fb_off] = colors[b][0];
            rgba_fb[fb_off+1] = colors[b][1];
            rgba_fb[fb_off+2] = colors[b][2];
            rgba_fb[fb_off+3] = 255;
        }
    }
}