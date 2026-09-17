// Board rendering — port of Android/BoardCanvas.qml.
// boardData:   BDYL*BDXL ints (-2 = off-board, -1 = date cell, 0 = empty, 1..n = piece)
// boardLabels: parallel array of display strings ('' = no label)

const ROWS = 8;   // BDYL
const COLS = 7;   // BDXL

const PIECE_COLORS_DARK = [
    '',
    '#5a8fb8',   // 1  I   steel blue
    '#e09040',   // 2  s   orange
    '#4aaa70',   // 3  Ls  sea green
    '#c84848',   // 4  T   red
    '#9870b8',   // 5  Q   purple
    '#38b0b0',   // 6  S   cyan
    '#c8a828',   // 7  sl  yellow
    '#c05898',   // 8  L   magenta
    '#58b040',   // 9  U   lime
    '#2898a0',   // 10 LL  teal
];
const PIECE_COLORS_LIGHT = [
    '',
    '#4682B4', '#FFA032', '#3CB371', '#DC5050', '#9467BD',
    '#40C8C8', '#F0C832', '#DC64B4', '#64C850', '#32B4AA',
];

// Extra colours for custom piece sets with more than 10 pieces
function pieceColor(palette, group) {
    if (group < palette.length) return palette[group];
    const hue = ((group - palette.length) * 47 + 20) % 360;
    return `hsl(${hue}, 55%, 55%)`;
}

export function drawBoard(canvas, boardData, boardLabels, darkMode) {
    const ctx  = canvas.getContext('2d');
    const dpr  = window.devicePixelRatio || 1;
    const cssW = canvas.clientWidth;
    const cssH = canvas.clientHeight;
    if (cssW <= 0 || cssH <= 0) return;

    if (canvas.width !== Math.round(cssW * dpr) || canvas.height !== Math.round(cssH * dpr)) {
        canvas.width  = Math.round(cssW * dpr);
        canvas.height = Math.round(cssH * dpr);
    }
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    ctx.clearRect(0, 0, cssW, cssH);

    const cell = Math.floor(cssW / COLS);
    const R    = Math.max(4, Math.round(cell * 0.15));
    const lw   = Math.max(1.5, cell * 0.03);
    const off  = lw / 2;   // inset so top/left borders are not half-clipped

    const g = (r, c) => {
        if (r < 0 || r >= ROWS || c < 0 || c >= COLS) return -2;
        const v = boardData[r * COLS + c];
        return (v !== undefined) ? v : -2;
    };

    function roundedCellR(px, py, sz, rad, tlR, trR, brR, blR) {
        ctx.beginPath();
        ctx.moveTo(px + (tlR ? rad : 0), py);
        if (trR) { ctx.lineTo(px+sz-rad, py);      ctx.arcTo(px+sz, py,    px+sz,     py+rad,   rad); }
        else       ctx.lineTo(px+sz,     py);
        if (brR) { ctx.lineTo(px+sz,  py+sz-rad);  ctx.arcTo(px+sz, py+sz, px+sz-rad, py+sz,    rad); }
        else       ctx.lineTo(px+sz,  py+sz);
        if (blR) { ctx.lineTo(px+rad, py+sz);      ctx.arcTo(px,    py+sz, px,        py+sz-rad, rad); }
        else       ctx.lineTo(px,     py+sz);
        if (tlR) { ctx.lineTo(px,     py+rad);     ctx.arcTo(px,    py,    px+rad,    py,       rad); }
        else       ctx.lineTo(px,     py);
        ctx.closePath();
    }
    const roundedCell = (px, py, sz, tlR, trR, brR, blR) =>
        roundedCellR(px, py, sz, R, tlR, trR, brR, blR);

    const dk = darkMode;
    const BG_COLOR    = dk ? '#1a1a1a' : '#e8e8e8';
    const EMPTY_COLOR = dk ? '#383838' : '#c0c0c0';
    const DATE_COLOR  = dk ? '#4a4035' : '#F0EBD2';
    const BORDER_COL  = dk ? '#888888' : '#505050';
    const TEXT_COL    = dk ? '#d8d8d8' : '#1E1E1E';
    const palette     = dk ? PIECE_COLORS_DARK : PIECE_COLORS_LIGHT;

    const fam      = getComputedStyle(canvas).fontFamily || 'sans-serif';
    const baseFont = 'bold ' + Math.round(cell * 0.22) + 'px ' + fam;
    const dateFont = 'bold ' + Math.round(cell * 0.25) + 'px ' + fam;

    ctx.font         = baseFont;
    ctx.textAlign    = 'center';
    ctx.textBaseline = 'middle';

    let r, c, px, py, sz, lbl, grp;

    // Pass 1: off-board background
    ctx.fillStyle = BG_COLOR;
    for (r = 0; r < ROWS; ++r)
        for (c = 0; c < COLS; ++c)
            if (g(r, c) === -2) ctx.fillRect(c*cell, r*cell, cell, cell);

    // Pass 2: empty cells
    for (r = 0; r < ROWS; ++r) {
        for (c = 0; c < COLS; ++c) {
            if (g(r, c) !== 0) continue;
            px = c*cell+2; py = r*cell+2; sz = cell-4;
            ctx.fillStyle = EMPTY_COLOR;
            roundedCell(px, py, sz, true, true, true, true);
            ctx.fill();
            lbl = boardLabels[r * COLS + c];
            if (lbl) {
                ctx.fillStyle = TEXT_COL;
                ctx.fillText(lbl, c*cell + cell/2, r*cell + cell/2);
            }
        }
    }

    // Pass 3: piece cells
    for (r = 0; r < ROWS; ++r) {
        for (c = 0; c < COLS; ++c) {
            grp = g(r, c);
            if (grp < 1) continue;
            const tO = g(r-1, c) !== grp, rO = g(r, c+1) !== grp;
            const bO = g(r+1, c) !== grp, lO = g(r, c-1) !== grp;
            ctx.fillStyle = pieceColor(palette, grp);
            roundedCell(c*cell, r*cell, cell, tO&&lO, tO&&rO, bO&&rO, bO&&lO);
            ctx.fill();
        }
    }

    // Pass 4: piece borders with rounded exterior corners
    ctx.save();
    ctx.translate(off, off);
    ctx.strokeStyle = BORDER_COL;
    ctx.lineWidth   = lw;
    for (r = 0; r < ROWS; ++r) {
        for (c = 0; c < COLS; ++c) {
            grp = g(r, c);
            if (grp < 1) continue;
            const x0 = c*cell, y0 = r*cell;
            const tO = g(r-1, c) !== grp, rO = g(r, c+1) !== grp;
            const bO = g(r+1, c) !== grp, lO = g(r, c-1) !== grp;
            if (!tO && !rO && !bO && !lO) continue;

            ctx.beginPath();
            if (tO) {
                ctx.moveTo(x0 + (lO ? R : 0),        y0);
                ctx.lineTo(x0 + cell - (rO ? R : 0), y0);
            }
            if (tO && rO) {
                ctx.moveTo(x0 + cell - R, y0);
                ctx.arcTo(x0 + cell, y0, x0 + cell, y0 + R, R);
            }
            if (rO) {
                ctx.moveTo(x0 + cell, y0 + (tO ? R : 0));
                ctx.lineTo(x0 + cell, y0 + cell - (bO ? R : 0));
            }
            if (bO && rO) {
                ctx.moveTo(x0 + cell, y0 + cell - R);
                ctx.arcTo(x0 + cell, y0 + cell, x0 + cell - R, y0 + cell, R);
            }
            if (bO) {
                ctx.moveTo(x0 + cell - (rO ? R : 0), y0 + cell);
                ctx.lineTo(x0 + (lO ? R : 0),        y0 + cell);
            }
            if (bO && lO) {
                ctx.moveTo(x0 + R, y0 + cell);
                ctx.arcTo(x0, y0 + cell, x0, y0 + cell - R, R);
            }
            if (lO) {
                ctx.moveTo(x0, y0 + cell - (bO ? R : 0));
                ctx.lineTo(x0, y0 + (tO ? R : 0));
            }
            if (tO && lO) {
                ctx.moveTo(x0, y0 + R);
                ctx.arcTo(x0, y0, x0 + R, y0, R);
            }
            ctx.stroke();
        }
    }
    ctx.restore();

    // Pass 5: date label cells
    const dR = Math.round(R * 0.6);
    for (r = 0; r < ROWS; ++r) {
        for (c = 0; c < COLS; ++c) {
            if (g(r, c) !== -1) continue;
            px = c*cell+2; py = r*cell+2; sz = cell-4;
            ctx.fillStyle   = DATE_COLOR;
            ctx.strokeStyle = BORDER_COL;
            ctx.lineWidth   = 1.5;
            roundedCellR(px, py, sz, dR, true, true, true, true);
            ctx.fill();
            ctx.stroke();
            lbl = boardLabels[r * COLS + c];
            if (lbl) {
                ctx.font      = dateFont;
                ctx.fillStyle = dk ? 'white' : '#111111';
                ctx.fillText(lbl, c*cell + cell/2, r*cell + cell/2);
                ctx.font      = baseFont;
            }
        }
    }
}
