# configurable output directory
OUT ?= build
# redirected stdout of rom_builder
DEBUG ?= /dev/null
CC=clang
CFLAGS=-Icsim -Wall
CSIM_DEPS = $(wildcard csim/*.h)
FTH_DEPS = $(wildcard fth/*.h)
CORE_SRC = csim/core.c
CORE_OBJ = $(OUT)/csim_core.o
FTH_OBJ = $(OUT)/fth_base.o
WASM_DEPS = $(wildcard wasm/*)
WASM_LDFLAGS=-Wl,--allow-undefined -Wl,--lto-O3 -Wl,--no-entry -Wl,--export-all
WASM_FLAGS=-O3 -nostdlib -flto

all: $(OUT)/csim_test $(OUT)/rom_builder $(OUT)/rom_tester \
	$(OUT)/rom.bin $(OUT)/web/salad.wasm

$(OUT)/csim_%.o: csim/%.c $(CSIM_DEPS)
	mkdir -p $(@D)
	$(CC) -c -o $@ $< $(CFLAGS)

$(OUT)/fth_%.o: fth/%.c $(FTH_DEPS) $(CSIM_DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(OUT)/csim_test: csim/test.c $(CORE_OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

$(OUT)/rom_builder: fth/rom_builder.c $(CORE_OBJ) $(FTH_OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

$(OUT)/rom_tester: fth/rom_tester.c $(CORE_OBJ) $(FTH_OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

$(OUT)/rom.bin: $(OUT)/rom_builder fth/base.sf
	$(OUT)/rom_builder fth/base.sf $@ $(OUT)/syms.txt > $(DEBUG)

#depends on csim sources directly
$(OUT)/web/salad.wasm: $(OUT)/rom.bin $(WASM_DEPS) $(CSIM_DEPS) $(CSIM_SRC)
	mkdir -p $(@D)
	$(CC) --target=wasm32 $(WASM_LDFLAGS) $(WASM_FLAGS) -o $@ \
		wasm/wasm_main.c $(CORE_SRC) $(CFLAGS)
	cp wasm/*.html $(OUT)/web/
	cp wasm/*.js $(OUT)/web/
	cp wasm/*.css $(OUT)/web/
	cp $(OUT)/rom.bin $(OUT)/web/rom.bin

.PHONY: clean, web, dev, serve, test

test: $(OUT)/csim_test
	$(OUT)/csim_test

dev: $(OUT)/rom.bin $(OUT)/rom_tester fth/dev.sf
	$(OUT)/rom_tester $(OUT)/rom.bin fth/dev.sf > $(DEBUG)

serve: $(OUT)/web/salad.wasm
	cd $(OUT)/web/ ; python3 -m http.server --bind 127.0.0.1

web: $(OUT)/web/salad.wasm

clean:
	rm $(OUT)/web/*
	rmdir $(OUT)/web
	rm $(OUT)/*