const display_size = 128  // must match wasm_main.c
const frame_time_ms = 50
const key_repeat_ms = 50

var term_box = null
var display = null
var wasm_core = null
var loaded_rom = null
const memory = new WebAssembly.Memory({ initial: 2})

addEventListener("DOMContentLoaded", (_) => {
    term_box = document.getElementById("term-box")
    const canvas = document.getElementById("display")
    canvas.width = display_size
    canvas.height = display_size
    display = canvas.getContext("2d");
    display.clearRect(0, 0, canvas.width, canvas.height);
    display.rect(0, 0, canvas.width, canvas.height);
    display.fill()

});

addEventListener("paste", (event) => {
    let data = event.clipboardData || window.clipboardData
    let text = data.getData('Text').replace(/[^\x0A-\x7E]/g, "");
    term_send(text)
})

function exec_wrap() {
    wasm_core.saladcore_js_exec()
    update_stack()
}

function serial_send_to_dev(ascii) {
    serial_buff.push(ascii)
    wasm_core.saladcore_js_int(0)
    exec_wrap()
}

function keypad_press(ascii) {
    last_keypress = ascii
    wasm_core.saladcore_js_int(1)
    exec_wrap()
}

function term_send(text) {
    let last_lf = false;
    for (const c of text) {
        const ascii = c.charCodeAt(0)
        if (ascii == 10) {
            serial_send_to_dev(ascii)
            last_lf = true
        }
        else {
            serial_buff.push(ascii)
            last_lf = false
        }
    }
    // insert newline if not last char
    if (!last_lf) serial_send_to_dev(10)
}


const handled_keys = {
    "Backspace": 8, //bksp
    "ArrowUp": 20, // dc4
    "ArrowDown": 11, // vtab
    "ArrowLeft": 18, // dc2
    "ArrowRight": 9, // tab,
    "Insert": 7 // bell
}

var serial_buff = []
var term_buff = []
var core_stack = []
var last_keypress = 0
var keys_repeat_timers = {}

function js_io_write(addr, val) {
    // console.log(val)
    if (addr == 35) {
        if (val == 8) {
            term_buff.pop()
        } else if (val >= 32 || val == 10) {
            // ignore rest of non-printables echo
            term_buff.push(String.fromCharCode(val))
        }
        term_box.textContent = term_buff.join('') + "|"
        term_box.scrollTop = term_box.scrollHeight;
    }
}

function js_io_read(addr) {
    if (addr == 34) // serial0 recv
        return serial_buff.shift()
    if (addr == 36) // serial0 recv pending
        return serial_buff.length
    if (addr == 40) {
        const key = last_keypress
        last_keypress = 0
        return key
    }
    if (addr == 41) {
        return Math.floor(Math.random() * 255);
    }
}

const error_enum = {
    1: "dstack",
    2: "rstack",
    3: "reg",
    4: "frame",
    5: "memory",
    6: "irq",
    7: "pc",
    8: "soft",
    9: "infloop"
}

function set_error(err) {
    const state = document.getElementById('state')
    if (err) {
        state.textContent = err
        state.classList = "state-error"
    } else {
        state.textContent = "ok"
        state.classList = "state-ok"
    }
}

function js_error(type, pc) {
    if (type==0) {
        console.log(pc)
        return
    }
    const err = error_enum[type] || 'unknown'
    set_error(`${err} err at ${pc}`)
}

function update_canvas() {
    if (!wasm_core) return
    const fb_addr = wasm_core.fb_addr()
    const fb_bytes = wasm_core.fb_bytes()
    wasm_core.convert_fb_to_rgba()
    const fb = new Uint8ClampedArray(
        wasm_core.memory.buffer.slice(fb_addr, fb_addr+fb_bytes))
    let img = new ImageData(fb, display_size, display_size);
    display.putImageData(img, 0, 0);
}

function fetch_rom(cb) {
    fetch('rom.bin')
    .then(response => response.arrayBuffer())
    .then(buffer => {cb(new Uint8Array(buffer))})
    .catch(err => console.error(err));
}

function open_tab(sample) {
    const blob = new Blob([sample], { type: 'text/plain'});
    const url = URL.createObjectURL(blob);
    window.open(url, '_blank');
}

function patch_sample(el) {
    const code = el.textContent.replace(/  +/g, ' ')
    const play = document.createElement('button')
    play.textContent = '\u25B6'
    play.classList = 'tinybtn'
    play.onclick = function() {
        this.blur()
        term_send(code)
    }
    const open = document.createElement('button')
    open.textContent = 'view'
    open.classList = 'tinybtn'
    open.onclick = function() {
        this.blur()
        open_tab(el.textContent)
    }
    el.parentElement.appendChild(open)
    el.parentElement.appendChild(play)
}

function fetch_guide() {
    fetch('guide.html')
    .then(response => response.text())
    .then(html => {
        const box = document.getElementById('guide')
        box.innerHTML = html
        for (const el of document.getElementsByTagName('ins'))
            patch_sample(el)
    }).catch(err => console.error(err));
}

function on_stack_changed(more) {
    const stack_table = document.getElementById("stack")
    const stack_more = document.getElementById("stack_more")
    stack_table.innerHTML = ''
    for (const se of core_stack) {
        const tr = document.createElement("tr");
        const td = document.createElement("td");
        tr.appendChild(td)
        td.textContent = se
        stack_table.appendChild(tr)
    }
    stack_more.hidden = !more
}

function update_stack() {
    if (!wasm_core) return
    let stack = []
    const dsz = wasm_core.get_dsz()
    for (let i = 0 ; i < Math.min(8,dsz) ; ++i)
        stack.push(wasm_core.get_reg(i))
    if (JSON.stringify(core_stack) !== JSON.stringify(stack)) {
        core_stack = stack
        on_stack_changed(dsz > 8)
    }
}

function clear_term(btn) {
    if (btn) btn.blur()
    term_buff = []
    term_box.textContent = "|"
}

function load_rom(rom) {
    let mem = new Uint8Array(wasm_core.memory.buffer)
    mem.fill(0, wasm_core.main_mem_addr(), wasm_core.main_mem_bytes())
    mem.set(rom, wasm_core.main_mem_addr())
    loaded_rom = rom
}

function reset_link_state() {
    serial_buff = []
    term_buff = []
    core_stack = []
    last_keypress = 0
    update_stack()
}

function reset_core(btn) {
    if (btn) btn.blur()
    console.assert(wasm_core && loaded_rom)
    set_error(null)
    clear_term()
    setTimeout(() => {
        reset_link_state()
        load_rom(loaded_rom)
        wasm_core.saladcore_js_init()
    })
}

function key_is_repeated(key) {
    return !!(keys_repeat_timers[key])
}

function key_repeat_on(key) {
    keys_repeat_timers[key] = setInterval(function(){
        keypad_press(key)
    }, key_repeat_ms)
}

function key_repeat_off(key) {
    if (keys_repeat_timers[key]) {
        clearInterval(keys_repeat_timers[key])
        keys_repeat_timers[key] = null
    }
}