(function() {
    const row_span_idx = []
    let n_cols = null, n_rows = null;
    let cursor = [0,0]
    let tui_box = null

    function rowClamp(val) {
        return Math.min(Math.max(val,0), n_rows-1)
    }

    function colClamp(val) {
        return Math.min(Math.max(val,0), n_cols-1)
    }

    function myspan(cls = "d") {
        const span = document.createElement("span");
        span.className = "tuitext " + cls
        return span
    }

    function tuiReset() {
        tuiInit(tui_box)
        console.log(`TUI resized to ${n_rows}x${n_cols}`)
    }

    window.tuiInit = function (elem) {
        tui_box = elem;

        function measureMonospace(spanCreator) {
            const span = spanCreator()
            span.style.visibility = "hidden";
            span.textContent = "M"; // a representative monospace char
            document.body.appendChild(span);
            const charBB = span.getBoundingClientRect();
            document.body.removeChild(span);
            return {w: charBB.width, h: charBB.height };
        }
        const char = measureMonospace(myspan)
        n_cols = Math.floor(elem.clientWidth / char.w);
        n_rows = Math.floor(elem.clientHeight / char.h);
        tuiClr()
        addEventListener("resize", tuiReset)
    }


    function crnt_row() {
        return document.getElementById('r'+cursor[0])
    }

    function crnt_span() {
        const row = crnt_row()
        const row_spans = row_span_idx[cursor[0]]
        const icol = cursor[1]
        let ispan = 0
        while(ispan < row_spans.length && icol >= row_spans[ispan]) {
            ispan++
        }
        return [ispan-1, row.childNodes[ispan-1]]
    }

    window.tuiClr = function() {
        tui_box.innerHTML = ''
        for (let i = 0 ;i < n_rows; ++i) {
            const row = document.createElement("div");
            row.id = 'r'+i
            const span = myspan()
            span.textContent = ' '.repeat(n_cols)
            row.appendChild(span)
            row_span_idx[i] = [0]
            tui_box.appendChild(row);
        }
    }

    window.tuiPuts = function(str, cls="d") {
        if (cursor[0] >= n_rows) return
        if (cursor[1] >= n_cols ) return
        if (cursor[1] + str.length >= n_cols) {
            str = str.substring(0, n_cols - cursor[1])
        }
        const [i,span] = crnt_span()
        const next = span.nextSibling
        const row_span = row_span_idx[cursor[0]]
        if (span.className == cls) {
            const off = cursor[1] - row_span[i]
            span.firstChild.replaceData(off, str.length, str)
        } else {
            // insert two more span - first with different style and second with old
            const left_len = cursor[1] - row_span[i]
            const orig_left_len = span.textContent.length
            const middle = myspan(cls)
            const right = myspan(span.className)
            middle.textContent = str
            right.textContent = span.textContent.substring(left_len+str.length)
            span.parentNode.insertBefore(middle, next);
            span.parentNode.insertBefore(right, next);
            span.textContent = span.textContent.substring(0, left_len)
            row_span.splice(i+1, 0, cursor[1])
            row_span.splice(i+2, 0, cursor[1]+str.length)
        }
    }

    window.tuiPos = function(row, col) {
        cursor[0] = rowClamp(row, n_rows-1)
        cursor[1] = colClamp(col, n_cols-1)
    }

    window.tuiMove = function(drow, dcol) {
        if (dcol>0 && !drow && cursor[1] >= n_cols-1) {
            cursor[0] = rowClamp(cursor[0]+1)
            cursor[1] = 0
            return
        }
        cursor[0] = rowClamp(cursor[0]+drow)
        cursor[1] = colClamp(cursor[1]+dcol)
    }

    window.tuiSz = function() {
        return [n_rows, n_cols]
    }

})()
