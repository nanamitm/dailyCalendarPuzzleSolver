// UI logic — port of Android/SolverBackend.cpp + Android/Main.qml.
import { drawBoard } from './board.js';
import { BXL, BOX, BOY, BDXL, BDYL } from './solver-core.js';

// ── Labels ─────────────────────────────────────────────────────────────────
const MONTH_ABBR = ['Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec'];
const DAY_ABBR   = ['Mon','Tue','Wed','Thu','Fri','Sat','Sun'];

const key = (row, col) => row * BXL + col;
const pad2 = n => String(n).padStart(2, '0');

// weekday: 1 = Monday … 7 = Sunday
function weekdayOf(date) { return ((date.getDay() + 6) % 7) + 1; }

function makeDateMap(date) {
    const m  = new Map();
    const mo = date.getMonth();
    const dy = date.getDate() - 1;
    const wd = weekdayOf(date) - 1;

    m.set(key(((mo / 6) | 0) + BOY,     (mo % 6) + BOX),     MONTH_ABBR[mo]);
    m.set(key(((dy / 7) | 0) + BOY + 2, (dy % 7) + BOX),     pad2(date.getDate()));
    if (wd === 6) m.set(key(9, 6), DAY_ABBR[6]);
    else          m.set(key(((wd / 3) | 0) + 9, (wd % 3) + 7), DAY_ABBR[wd]);
    return m;
}

const ALL_LABELS = (() => {
    const m = new Map();
    for (let i = 0; i < 12; ++i) m.set(key(((i / 6) | 0) + BOY,     (i % 6) + BOX), MONTH_ABBR[i]);
    for (let i = 0; i < 31; ++i) m.set(key(((i / 7) | 0) + BOY + 2, (i % 7) + BOX), pad2(i + 1));
    m.set(key(9, 6), 'Sun');
    for (let i = 0; i < 6; ++i) m.set(key(((i / 3) | 0) + 9, (i % 3) + 7), DAY_ABBR[i]);
    return m;
})();

// Builds the flat board/label arrays the canvas draws (port of updateBoardData)
function computeBoardData(cells, date) {
    const dateMap  = makeDateMap(date);
    const labelMap = cells ? dateMap : ALL_LABELS;
    const data = [], labels = [];

    for (let r = 0; r < BDYL; ++r) {
        const row = r + BOY;
        for (let c = 0; c < BDXL; ++c) {
            const col = c + BOX;
            const k = key(row, col);
            let group;
            if (cells) {
                const v = cells[k];
                group = (v === -1) ? (dateMap.has(k) ? -1 : -2) : v;
            } else {
                const corner = ((row === 3 || row === 4) && col === 9) ||
                               (row === 10 && col >= 3 && col <= 6);
                group = corner ? (dateMap.has(k) ? -1 : -2) : 0;
            }
            data.push(group);
            labels.push(labelMap.get(k) || '');
        }
    }
    return { data, labels };
}

// ── Persistent settings ────────────────────────────────────────────────────
const SETTINGS_KEY  = 'puzzlesolver.settings';
const PIECESETS_KEY = 'puzzlesolver.pieceSets';

function loadJson(k, fallback) {
    try { const s = localStorage.getItem(k); return s ? JSON.parse(s) : fallback; }
    catch { return fallback; }
}
function saveJson(k, v) {
    try { localStorage.setItem(k, JSON.stringify(v)); } catch { /* private mode */ }
}

const settings = Object.assign(
    { findAll: false, slideshow: false, autoMidnight: false, pieceSet: 0 },
    loadJson(SETTINGS_KEY, {}));

// [{ name, text }] — custom piece sets imported from PuzzleMaker JSON
let pieceSets = loadJson(PIECESETS_KEY, []);

const saveSettings = () => saveJson(SETTINGS_KEY, settings);

// ── State ──────────────────────────────────────────────────────────────────
const state = {
    date: new Date(),
    solving: false,
    solutions: [],        // Int8Array per solution (full BYL*BXL board)
    stride: 0,
    idx: 0,
    solLabel: '',
    statusText: '',
    boardData: null,
};

let worker = null;
let reqId = 0;
let debounceTimer = null;
let slideshowTimer = null;
let midnightTimer = null;
let lcgState = 0, lcgM = 1;

const $ = id => document.getElementById(id);
const el = {
    header: $('header'), dateLabel: $('dateLabel'),
    prevDay: $('prevDay'), nextDay: $('nextDay'),
    gear: $('gearButton'), menu: $('settingsMenu'), pieceSetList: $('pieceSetList'),
    boardArea: $('boardArea'), canvas: $('boardCanvas'), outgoing: $('outgoingBoard'),
    overlay: $('solvingOverlay'), cancel: $('cancelButton'),
    footer: $('footer'), dots: $('dots'), solLabel: $('solLabel'), statusText: $('statusText'),
    prevSol: $('prevSol'), nextSol: $('nextSol'),
    splash: $('splash'), toast: $('toast'), fileInput: $('pieceSetInput'),
};

const isDark = () => window.matchMedia('(prefers-color-scheme: dark)').matches;

// ── Board layout / rendering ───────────────────────────────────────────────
let bw = 0, bh = 0, bx = 0, by = 0;

function layoutBoard() {
    const w = el.boardArea.clientWidth, h = el.boardArea.clientHeight;
    bw = Math.min(w, h * BDXL / BDYL);
    bh = bw * BDYL / BDXL;
    bx = (w - bw) / 2;
    by = (h - bh) / 2;
    for (const cv of [el.canvas, el.outgoing]) {
        cv.style.width  = bw + 'px';
        cv.style.height = bh + 'px';
        cv.style.left   = '0px';
        cv.style.top    = by + 'px';
    }
    el.canvas.style.transform = `translateX(${bx}px)`;
    renderBoard();
}

function renderBoard() {
    if (!state.boardData) return;
    drawBoard(el.canvas, state.boardData.data, state.boardData.labels, isDark());
}

function setBoard(cells) {
    state.boardData = computeBoardData(cells, state.date);
    renderBoard();
}

// ── Footer ─────────────────────────────────────────────────────────────────
function renderFooter() {
    const n = state.solutions.length;
    el.footer.hidden = !(n > 0 || state.solLabel !== '');

    const many = n > 1;
    el.prevSol.hidden = el.nextSol.hidden = !many;
    el.solLabel.classList.toggle('compact', many);
    el.solLabel.textContent = many ? `${state.idx + 1} / ${n}` : state.solLabel;
    el.statusText.textContent = state.statusText;

    // Dot indicator, capped at 15 dots
    const count = many ? Math.min(n, 15) : 0;
    const cur   = n <= 15 ? state.idx
                          : Math.round(state.idx * 14 / Math.max(n - 1, 1));
    if (el.dots.childElementCount !== count) {
        el.dots.textContent = '';
        for (let i = 0; i < count; ++i) {
            const d = document.createElement('div');
            d.className = 'dot';
            el.dots.appendChild(d);
        }
    }
    [...el.dots.children].forEach((d, i) => d.classList.toggle('active', i === cur));
}

// ── Solving ────────────────────────────────────────────────────────────────
function ensureWorker() {
    if (worker) return worker;
    worker = new Worker('solver-worker.js', { type: 'module' });
    worker.onmessage = onWorkerMessage;
    return worker;
}

function scheduleSolve() {
    clearTimeout(debounceTimer);
    debounceTimer = setTimeout(solveNow, 300);
}

function solveNow() {
    stopSlideshow();
    if (state.solving) cancelSolve();   // supersede the running search

    state.solutions = [];
    state.idx = 0;
    state.solLabel = 'Solving…';
    state.statusText = '';
    setBoard(null);
    renderFooter();

    state.solving = true;
    el.overlay.hidden = false;

    const set = settings.pieceSet > 0 ? pieceSets[settings.pieceSet - 1] : null;
    ensureWorker().postMessage({
        type: 'solve',
        reqId: ++reqId,
        weekday: weekdayOf(state.date),
        day: state.date.getDate(),
        month: state.date.getMonth() + 1,
        findAll: settings.findAll || settings.slideshow,
        pieceSetText: set ? set.text : null,
        pieceSetName: set ? set.name : null,
    });
}

function cancelSolve() {
    if (worker) { worker.terminate(); worker = null; }
    state.solving = false;
    el.overlay.hidden = true;
}

function onWorkerMessage(e) {
    const msg = e.data;
    if (msg.reqId !== reqId) return;   // stale result

    state.solving = false;
    el.overlay.hidden = true;

    if (msg.type === 'error') {
        state.solLabel = 'エラー: ' + msg.error;
        state.statusText = '';
        setBoard(null);
        renderFooter();
        dismissSplash();
        return;
    }

    state.stride = msg.stride;
    state.solutions = [];
    for (let i = 0; i < msg.count; ++i)
        state.solutions.push(msg.boards.subarray(i * msg.stride, (i + 1) * msg.stride));
    state.idx = 0;

    if (state.solutions.length > 0) {
        setBoard(state.solutions[0]);
        state.solLabel = `Solution 1 / ${state.solutions.length}`;
        if (settings.slideshow && state.solutions.length > 1) startSlideshow();
    } else {
        setBoard(null);
        state.solLabel = 'No solution found';
    }
    state.statusText = `${state.solutions.length} 解  ·  ${msg.elapsedMs.toFixed(1)} ms`;
    renderFooter();
    dismissSplash();
}

// ── Solution navigation ────────────────────────────────────────────────────
function showSolution(i) {
    state.idx = i;
    setBoard(state.solutions[i]);
    state.solLabel = `Solution ${i + 1} / ${state.solutions.length}`;
    renderFooter();
}

function step(goingNext) {
    const n = state.solutions.length;
    if (n < 2) return;
    return (state.idx + (goingNext ? 1 : n - 1)) % n;
}

// Slides the current board out and the next one in (port of swipeSolution)
let animating = false;
function swipeSolution(goingNext) {
    const next = step(goingNext);
    if (next === undefined || animating) return;
    animating = true;

    const prev = state.boardData;
    drawBoard(el.outgoing, prev.data, prev.labels, isDark());
    el.outgoing.style.visibility = 'visible';
    el.outgoing.classList.remove('sliding');
    el.outgoing.style.transform = `translateX(${bx}px)`;

    showSolution(next);
    el.canvas.classList.remove('sliding');
    el.canvas.style.transform = `translateX(${goingNext ? el.boardArea.clientWidth : -bw}px)`;

    requestAnimationFrame(() => {
        el.outgoing.classList.add('sliding');
        el.canvas.classList.add('sliding');
        el.outgoing.style.transform =
            `translateX(${goingNext ? -bw : el.boardArea.clientWidth}px)`;
        el.canvas.style.transform = `translateX(${bx}px)`;
    });

    setTimeout(() => {
        el.outgoing.style.visibility = 'hidden';
        el.canvas.classList.remove('sliding');
        animating = false;
    }, 300);

    if (slideshowTimer) restartSlideshow();   // reset the interval on manual nav
}

// ── Slideshow (full-period Hull–Dobell LCG over the solution list) ─────────
function initLcg() {
    let m = 1;
    while (m < state.solutions.length) m <<= 1;
    lcgM = m;
    lcgState = state.idx;
}
function nextLcgIdx() {
    const n = state.solutions.length;
    do { lcgState = (5 * lcgState + 1) % lcgM; } while (lcgState >= n);
    return lcgState;
}
function startSlideshow() {
    initLcg();
    restartSlideshow();
}
function restartSlideshow() {
    clearInterval(slideshowTimer);
    slideshowTimer = setInterval(() => {
        if (state.solutions.length < 2) return;
        showSolution(nextLcgIdx());
    }, 2000);
}
function stopSlideshow() {
    clearInterval(slideshowTimer);
    slideshowTimer = null;
}

// ── Date handling ──────────────────────────────────────────────────────────
const dateFmt = new Intl.DateTimeFormat('ja-JP', {
    year: 'numeric', month: '2-digit', day: '2-digit', weekday: 'short',
});

function renderDateLabel() {
    const d = state.date;
    const wd = dateFmt.formatToParts(d).find(p => p.type === 'weekday').value;
    el.dateLabel.textContent =
        `${d.getFullYear()}/${pad2(d.getMonth() + 1)}/${pad2(d.getDate())} (${wd})`;

    el.dateLabel.classList.add('fading');
    setTimeout(() => el.dateLabel.classList.remove('fading'), 80);
}

function setDate(d) {
    if (d < new Date(1900, 0, 1) || d > new Date(2099, 11, 31)) return;
    state.date = d;
    renderDateLabel();
    scheduleSolve();
}

function addDays(n) {
    const d = new Date(state.date);
    d.setDate(d.getDate() + n);
    setDate(d);
}

const isToday = () => {
    const n = new Date();
    return state.date.toDateString() === n.toDateString();
};

function goToday() { if (!isToday()) setDate(new Date()); }

function scheduleMidnight() {
    clearTimeout(midnightTimer);
    const now = new Date();
    const midnight = new Date(now.getFullYear(), now.getMonth(), now.getDate() + 1, 0, 0, 1);
    midnightTimer = setTimeout(() => {
        if (settings.autoMidnight) goToday();
        scheduleMidnight();
    }, midnight - now);
}

// ── Settings menu ──────────────────────────────────────────────────────────
function renderMenu() {
    const mark = (action, on, enabled = true) => {
        const item = el.menu.querySelector(`[data-action="${action}"]`);
        item.classList.toggle('checked', on);
        item.disabled = !enabled;
        item.setAttribute('aria-checked', String(on));
    };
    // Slideshow implies 全解探索, so that item is locked while it runs
    mark('findAll',   settings.findAll || settings.slideshow, !settings.slideshow);
    mark('slideshow', settings.slideshow);
    mark('midnight',  settings.autoMidnight);

    el.pieceSetList.textContent = '';
    const names = ['デフォルト (元の10ピース)', ...pieceSets.map(p => p.name)];
    names.forEach((name, i) => {
        const item = document.createElement('button');
        item.className = 'menu-item' + (i === settings.pieceSet ? ' checked' : '');
        item.type = 'button';
        item.innerHTML = '<span class="check"></span>';

        const label = document.createElement('span');
        label.className = 'label';
        label.textContent = name;
        item.appendChild(label);

        item.addEventListener('click', () => {
            settings.pieceSet = i;
            saveSettings();
            renderMenu();
            closeMenu();
            scheduleSolve();
        });

        if (i > 0) {
            const del = document.createElement('span');
            del.className = 'del';
            del.textContent = '✕';
            del.title = 'このピースセットを削除';
            del.addEventListener('click', ev => {
                ev.stopPropagation();
                pieceSets.splice(i - 1, 1);
                saveJson(PIECESETS_KEY, pieceSets);
                if (settings.pieceSet === i) settings.pieceSet = 0;
                else if (settings.pieceSet > i) settings.pieceSet--;
                saveSettings();
                renderMenu();
                scheduleSolve();
            });
            item.appendChild(del);
        }
        el.pieceSetList.appendChild(item);
    });
}

const openMenu  = () => { el.menu.hidden = false; };
const closeMenu = () => { el.menu.hidden = true; };

el.gear.addEventListener('click', ev => {
    ev.stopPropagation();
    el.menu.hidden ? openMenu() : closeMenu();
});
document.addEventListener('click', ev => {
    if (!el.menu.hidden && !el.menu.contains(ev.target)) closeMenu();
});

el.menu.addEventListener('click', ev => {
    const item = ev.target.closest('.menu-item[data-action]');
    if (!item || item.disabled) return;
    switch (item.dataset.action) {
    case 'findAll':
        settings.findAll = !settings.findAll;
        saveSettings(); renderMenu(); scheduleSolve();
        break;
    case 'slideshow':
        settings.slideshow = !settings.slideshow;
        if (!settings.slideshow) stopSlideshow();
        saveSettings(); renderMenu(); scheduleSolve();
        break;
    case 'midnight':
        settings.autoMidnight = !settings.autoMidnight;
        saveSettings(); renderMenu();
        break;
    case 'import':
        el.fileInput.click();
        closeMenu();
        break;
    }
});

// ── Piece set import ───────────────────────────────────────────────────────
function showToast(message, isError) {
    el.toast.textContent = message;
    el.toast.classList.toggle('error', !!isError);
    el.toast.hidden = false;
    clearTimeout(showToast.timer);
    showToast.timer = setTimeout(() => { el.toast.hidden = true; }, 3000);
}

async function importPieceSet(file) {
    let text;
    try { text = await file.text(); }
    catch (err) { showToast('ファイルを開けません: ' + err.message, true); return; }

    let parsed;
    try {
        const { parsePieceSet } = await import('./solver-core.js');
        parsed = parsePieceSet(text, file.name.replace(/\.json$/i, ''));
    } catch (err) {
        showToast('不正な形式: ' + err.message, true);
        return;
    }

    const name = parsed.description || file.name.replace(/\.json$/i, '');
    const existing = pieceSets.findIndex(p => p.name === name);
    if (existing >= 0) pieceSets[existing] = { name, text };
    else pieceSets.push({ name, text });
    saveJson(PIECESETS_KEY, pieceSets);

    settings.pieceSet = (existing >= 0 ? existing : pieceSets.length - 1) + 1;
    saveSettings();
    renderMenu();
    showToast(`「${name}」を追加しました`);
    scheduleSolve();
}

el.fileInput.addEventListener('change', () => {
    const f = el.fileInput.files[0];
    if (f) importPieceSet(f);
    el.fileInput.value = '';
});

// Drag & drop a piece set anywhere on the page
document.addEventListener('dragover', ev => ev.preventDefault());
document.addEventListener('drop', ev => {
    ev.preventDefault();
    const f = ev.dataTransfer?.files?.[0];
    if (f) importPieceSet(f);
});

// ── Pointer interaction ────────────────────────────────────────────────────
function addSwipe(target, onSwipe, onTap, onLongPress) {
    let startX = 0, startY = 0, longTimer = null, didLong = false;
    let activeId = null;   // ignore a pointerup without its own pointerdown

    target.addEventListener('pointerdown', ev => {
        activeId = ev.pointerId;
        startX = ev.clientX; startY = ev.clientY; didLong = false;
        if (onLongPress) {
            longTimer = setTimeout(() => { didLong = true; onLongPress(); }, 600);
        }
    });
    target.addEventListener('pointerup', ev => {
        clearTimeout(longTimer);
        if (ev.pointerId !== activeId) return;
        activeId = null;
        if (didLong) return;
        const dx = ev.clientX - startX, dy = ev.clientY - startY;
        if (Math.abs(dx) > Math.abs(dy) * 1.5 && Math.abs(dx) > 40) onSwipe(dx < 0);
        else if (Math.hypot(dx, dy) < 10 && onTap) onTap();
    });
    target.addEventListener('pointercancel', () => { clearTimeout(longTimer); activeId = null; });
}

addSwipe(el.dateLabel,
    goingNext => addDays(goingNext ? 1 : -1),
    () => openDatePicker(),
    () => {
        if (isToday()) return;
        goToday();
        el.dateLabel.classList.add('pulse');
        setTimeout(() => el.dateLabel.classList.remove('pulse'), 420);
    });

addSwipe(el.boardArea, goingNext => {
    if (state.solutions.length > 1 && !state.solving) swipeSolution(goingNext);
});

el.prevDay.addEventListener('click', () => addDays(-1));
el.nextDay.addEventListener('click', () => addDays(1));
el.prevSol.addEventListener('click', () => swipeSolution(false));
el.nextSol.addEventListener('click', () => swipeSolution(true));
el.cancel.addEventListener('click', () => {
    cancelSolve();
    state.solLabel = 'キャンセルしました';
    renderFooter();
    dismissSplash();
});

document.addEventListener('keydown', ev => {
    if (!$('datePickerBackdrop').hidden) return;
    switch (ev.key) {
    case 'ArrowLeft':  swipeSolution(false); break;
    case 'ArrowRight': swipeSolution(true);  break;
    case 'ArrowUp':    addDays(-1); break;
    case 'ArrowDown':  addDays(1);  break;
    case 't': case 'T': goToday();  break;
    default: return;
    }
    ev.preventDefault();
});

// ── Date picker (tumbler wheels) ───────────────────────────────────────────
const ITEM_H = 36;
const picker = {
    backdrop: $('datePickerBackdrop'),
    year:  $('yearTumbler'),
    month: $('monthTumbler'),
    day:   $('dayTumbler'),
};

const daysInMonth = (year, month) => new Date(year, month, 0).getDate();

function fillTumbler(node, values) {
    node.textContent = '';
    values.forEach(v => {
        const d = document.createElement('div');
        d.textContent = v;
        node.appendChild(d);
    });
}

const tumblerIndex = node => Math.round(node.scrollTop / ITEM_H);

function setTumblerIndex(node, i) {
    node.scrollTop = i * ITEM_H;
    markTumbler(node);
}

function markTumbler(node) {
    const sel = tumblerIndex(node);
    const y = 1900 + tumblerIndex(picker.year);
    const m = tumblerIndex(picker.month) + 1;
    const maxDay = daysInMonth(y, m);
    [...node.children].forEach((child, i) => {
        child.classList.toggle('sel', i === sel);
        if (node === picker.day) child.classList.toggle('invalid', i + 1 > maxDay);
    });
}

for (const node of [picker.year, picker.month, picker.day]) {
    let t = null;
    node.addEventListener('scroll', () => {
        clearTimeout(t);
        t = setTimeout(() => {
            markTumbler(node);
            if (node !== picker.day) markTumbler(picker.day);
        }, 60);
    });
}

function openDatePicker() {
    fillTumbler(picker.year,  Array.from({ length: 200 }, (_, i) => 1900 + i));
    fillTumbler(picker.month, Array.from({ length: 12 },  (_, i) => pad2(i + 1)));
    fillTumbler(picker.day,   Array.from({ length: 31 },  (_, i) => pad2(i + 1)));
    picker.backdrop.hidden = false;
    setTumblerIndex(picker.year,  state.date.getFullYear() - 1900);
    setTumblerIndex(picker.month, state.date.getMonth());
    setTumblerIndex(picker.day,   state.date.getDate() - 1);
}

$('pickerCancel').addEventListener('click', () => { picker.backdrop.hidden = true; });
picker.backdrop.addEventListener('click', ev => {
    if (ev.target === picker.backdrop) picker.backdrop.hidden = true;
});
$('pickerOk').addEventListener('click', () => {
    const y = 1900 + tumblerIndex(picker.year);
    const m = tumblerIndex(picker.month) + 1;
    const d = Math.min(tumblerIndex(picker.day) + 1, daysInMonth(y, m));
    picker.backdrop.hidden = true;
    setDate(new Date(y, m - 1, d));
});

// ── Splash ─────────────────────────────────────────────────────────────────
let splashMinPassed = false, splashSolveDone = false;

function tryDismissSplash() {
    if (!splashMinPassed || !splashSolveDone || !el.splash) return;
    el.splash.classList.add('hide');
    setTimeout(() => el.splash.remove(), 520);
}
function dismissSplash() { splashSolveDone = true; tryDismissSplash(); }

setTimeout(() => { splashMinPassed = true; tryDismissSplash(); }, 800);
setTimeout(() => { splashMinPassed = splashSolveDone = true; tryDismissSplash(); }, 5000);

// ── Startup ────────────────────────────────────────────────────────────────
window.addEventListener('resize', layoutBoard);
// The board area also changes size when the footer appears or its text wraps
new ResizeObserver(layoutBoard).observe(el.boardArea);
window.matchMedia('(prefers-color-scheme: dark)').addEventListener('change', renderBoard);

if (settings.pieceSet > pieceSets.length) settings.pieceSet = 0;

renderDateLabel();
renderMenu();
layoutBoard();
setBoard(null);
renderFooter();
scheduleMidnight();
scheduleSolve();

if ('serviceWorker' in navigator)
    window.addEventListener('load', () => navigator.serviceWorker.register('sw.js').catch(() => {}));
