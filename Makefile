# configurable output directory
OUT ?= build
CC=clang
CFLAGS=-Icore -Iinclude -Ifth -Wall -O1 -g
CORE_DEPS = core/core.c
FTH_DEPS = fth/saladforth.c fth/saladforth.h
INCLUDES = $(wildcard include/*.h)
WASM_DEPS = $(wildcard wasm/*.c)
CODEMIRROR = wasm/lib/codemirror.js wasm/lib/codemirror.css
WASM_LDFLAGS=-Wl,--allow-undefined -Wl,--no-entry -Wl,--export-all
WASM_CFLAGS=-nostdlib -flto

all: $(OUT)/web/salad.wasm

$(OUT)/web/salad.wasm: $(WASM_DEPS) $(CORE_DEPS) $(FTH_DEPS) $(INCLUDES)
	mkdir -p $(@D)
	$(CC) --target=wasm32 $(WASM_LDFLAGS) $(WASM_CFLAGS) -o $@ \
		wasm/wasm_main.c fth/saladforth.c core/core.c $(CFLAGS)

serve: $(OUT)/web/salad.wasm $(WASM_DEPS) $(CODEMIRROR)
	cp fth/dev.sf $(OUT)/web/
	cp wasm/index.html $(OUT)/web/
	cp wasm/style.css $(OUT)/web/
	cp wasm/*.js $(OUT)/web/
	cp -r wasm/lib $(OUT)/web/
	cd $(OUT)/web/ ; python3 -m http.server --bind 127.0.0.1

.PHONY: clean, web, serve

clean:
	rm $(OUT)/web/*
	rmdir $(OUT)/web
