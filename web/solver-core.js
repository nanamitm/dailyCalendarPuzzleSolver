// Daily Calendar Puzzle solver — JavaScript port of gui_cpp/solver.cpp.
// Same algorithm: place pieces on the first free square, scanning top-left first.
// Board cells are kept in a flat Int8Array and mutated in place with undo,
// instead of copying a Board per attempt as the C++ version does.

export const BXL  = 13;  // total X length (with padding)
export const BYL  = 14;  // total Y length (with padding)
export const BOX  = 3;   // playable-area origin X
export const BOY  = 3;   // playable-area origin Y
export const BDXL = 7;   // playable X span
export const BDYL = 8;   // playable Y span

// Trans enum — same order as solver.h
export const UP = 0, RIGHT = 1, DOWN = 2, LEFT = 3,
             UP_BACK = 4, LEFT_BACK = 5, DOWN_BACK = 6, RIGHT_BACK = 7;

const FACE_T  = [UP, RIGHT, DOWN, LEFT];
const UP_R_T2 = [UP, RIGHT];
const UP_R_T4 = [UP, RIGHT, UP_BACK, RIGHT_BACK];
const ALL_T8  = [UP, RIGHT, DOWN, LEFT, UP_BACK, RIGHT_BACK, DOWN_BACK, LEFT_BACK];

export function applyTrans(x, y, t) {
    switch (t) {
    case UP:         return [ x,  y];
    case RIGHT:      return [ y, -x];
    case DOWN:       return [-x, -y];
    case LEFT:       return [-y,  x];
    case UP_BACK:    return [-x,  y];
    case RIGHT_BACK: return [ y,  x];
    case DOWN_BACK:  return [ x, -y];
    case LEFT_BACK:  return [-y, -x];
    }
    return [x, y];
}

// ── Piece ──────────────────────────────────────────────────────────────────
export class Piece {
    constructor(shape, value, relevantTrans) {
        this.len   = shape.length;
        this.baseX = new Int8Array(this.len);
        this.baseY = new Int8Array(this.len);
        this.currX = new Int8Array(this.len);
        this.currY = new Int8Array(this.len);
        for (let i = 0; i < this.len; ++i) {
            this.baseX[i] = this.currX[i] = shape[i][0];
            this.baseY[i] = this.currY[i] = shape[i][1];
        }
        this.origin = 0;
        this.value  = value;
        this.rel    = relevantTrans;
    }

    transform(t) {
        for (let i = 0; i < this.len; ++i) {
            const v = applyTrans(this.baseX[i], this.baseY[i], t);
            this.currX[i] = v[0];
            this.currY[i] = v[1];
        }
    }

    // Mirrors Piece::operator[]: out of range means the null vector (chain end).
    idxOf(index) {
        let idx = index + this.origin;
        if (index > 0) idx -= 1;
        return (idx >= 0 && idx < this.len) ? idx : -1;
    }

    clone() {
        const shape = [];
        for (let i = 0; i < this.len; ++i) shape.push([this.baseX[i], this.baseY[i]]);
        return new Piece(shape, this.value, this.rel);
    }
}

// ── Board ──────────────────────────────────────────────────────────────────
// cells[y * BXL + x]: -1 = off-board/date cell, 0 = empty, 1..n = piece value
// weekday: 1 = Monday … 7 = Sunday
export function makeBoard(weekday, monthDay, month) {
    const cells = new Int8Array(BYL * BXL).fill(-1);
    for (let y = BOY; y < BOY + BDYL; ++y)
        for (let x = BOX; x < BOX + BDXL; ++x)
            cells[y * BXL + x] = 0;

    // Non-playable corners inside the playable bounding box
    cells[3 * BXL + 9] = -1;
    cells[4 * BXL + 9] = -1;
    for (let x = 3; x <= 6; ++x) cells[10 * BXL + x] = -1;

    // The three date cells must stay uncovered
    cells[((((month - 1) / 6) | 0) + 3) * BXL + (((month - 1) % 6) + 3)] = -1;
    cells[((((monthDay - 1) / 7) | 0) + 5) * BXL + (((monthDay - 1) % 7) + 3)] = -1;
    if ((weekday - 1) === 6)
        cells[9 * BXL + 6] = -1;
    else
        cells[(((((weekday - 1) / 3) | 0) + 9) * BXL) + (((weekday - 1) % 3) + 7)] = -1;

    return cells;
}

function nextAvailablePos(cells, out) {
    for (let y = BOY; y < BOY + BDYL; ++y) {
        for (let x = BOX; x < BOX + BDXL; ++x) {
            if (cells[y * BXL + x] === 0) {
                out[0] = x - BOX;
                out[1] = y - BOY;
                return true;
            }
        }
    }
    return false;
}

function putSquare(cells, value, x, y, placed) {
    const xx = BOX + x, yy = BOY + y;
    if (xx < 0 || xx >= BXL || yy < 0 || yy >= BYL) return false;
    const i = yy * BXL + xx;
    if (cells[i] !== 0) return false;
    cells[i] = value;
    placed.push(i);
    return true;
}

// Places `piece` with its `origin`-th square on (posX, posY), walking the vector
// chain backward then forward. Every written square is recorded in `placed`.
function tryPlace(cells, piece, posX, posY, placed) {
    if (!putSquare(cells, piece.value, posX, posY, placed)) return false;

    let idx = -1, cx = posX, cy = posY, k = piece.idxOf(idx);
    while (k >= 0) {
        cx -= piece.currX[k]; cy -= piece.currY[k];
        if (!putSquare(cells, piece.value, cx, cy, placed)) return false;
        k = piece.idxOf(--idx);
    }
    idx = 1; cx = posX; cy = posY; k = piece.idxOf(idx);
    while (k >= 0) {
        cx += piece.currX[k]; cy += piece.currY[k];
        if (!putSquare(cells, piece.value, cx, cy, placed)) return false;
        k = piece.idxOf(++idx);
    }
    return true;
}

// ── Recursive solver ───────────────────────────────────────────────────────
function solveRec(cells, pieces, out, uniqueSol, st) {
    if (pieces.length === 0) {
        out.solutions.push(Int8Array.from(cells));
        return true;
    }

    const pos = [0, 0];
    if (!nextAvailablePos(cells, pos)) return false;
    const posX = pos[0], posY = pos[1];
    const placed = [];

    for (let pi = 0; pi < pieces.length && st.keep; ++pi) {
        const piece = pieces[pi];
        const rest  = pieces.slice(0, pi).concat(pieces.slice(pi + 1));
        for (let origin = 0; origin <= piece.len && st.keep; ++origin) {
            piece.origin = origin;
            for (let ti = 0; ti < piece.rel.length && st.keep; ++ti) {
                piece.transform(piece.rel[ti]);
                placed.length = 0;
                const ok = tryPlace(cells, piece, posX, posY, placed);
                ++out.tries;
                if (ok) {
                    ++out.pcsPlaced;
                    const isSol = solveRec(cells, rest, out, uniqueSol, st);
                    if (uniqueSol && isSol) st.keep = false;
                }
                for (let i = placed.length - 1; i >= 0; --i) cells[placed[i]] = 0;
            }
        }
    }
    return false;
}

// ── Default piece set ──────────────────────────────────────────────────────
// flipSmallS (piece 2) / flipQ (piece 5) widen those pieces to their back side.
function defaultPieces(flipSmallS, flipQ) {
    const P = (shape, value, trans) => new Piece(shape, value, trans);

    const FourFlat   = P([[0,1],[0,1],[0,1]],          1, UP_R_T2);
    const SmallS     = P([[0,1],[1,0],[0,1]],          2, flipSmallS ? UP_R_T4 : UP_R_T2);
    const SmallL     = P([[0,1],[1,0],[1,0]],          3, FACE_T);
    const T          = P([[1,0],[1,0],[-1,1],[0,1]],   4, FACE_T);
    const Q          = P([[0,1],[1,0],[0,1],[-1,0]],   5, flipQ ? ALL_T8 : FACE_T);
    const BigS       = P([[1,0],[0,1],[0,1],[1,0]],    6, UP_R_T2);
    const SmallsTail = P([[1,0],[0,1],[1,0],[1,0]],    7, FACE_T);
    const BigL       = P([[0,1],[1,0],[1,0],[1,0]],    8, FACE_T);
    const U          = P([[0,1],[1,0],[1,0],[0,-1]],   9, FACE_T);
    const Lequal     = P([[0,1],[0,1],[1,0],[1,0]],   10, FACE_T);

    return [FourFlat, U, Q, SmallsTail, SmallL, BigL, SmallS, Lequal, BigS, T];
}

const now = () => (typeof performance !== 'undefined' ? performance.now() : Date.now());

function runPhase(weekday, day, month, findAll, pieces) {
    const cells = makeBoard(weekday, day, month);
    const out = { solutions: [], tries: 0, pcsPlaced: 0, elapsedMs: 0 };
    solveRec(cells, pieces, out, !findAll, { keep: true });
    return out;
}

// ── Public entry points ────────────────────────────────────────────────────
// weekday: 1 = Monday … 7 = Sunday
export function solveDate(weekday, day, month, findAll) {
    const t0 = now();

    // Phase 1: frosted side only — fast, covers most dates
    let out = runPhase(weekday, day, month, findAll, defaultPieces(false, false));

    // Phase 2: retry flipping Q (5) and SmallS (2) when phase 1 found nothing
    if (out.solutions.length === 0)
        out = runPhase(weekday, day, month, findAll, defaultPieces(true, true));

    out.elapsedMs = now() - t0;
    return out;
}

export function solveDateCustom(weekday, day, month, findAll, pieceSet) {
    const t0 = now();
    // Clone so transform() does not pollute the stored set
    const out = runPhase(weekday, day, month, findAll, pieceSet.pieces.map(p => p.clone()));
    out.elapsedMs = now() - t0;
    return out;
}

// ── Custom piece sets (PuzzleMaker JSON) ───────────────────────────────────
function chainToCells(vecs, t) {
    let cx = 0, cy = 0, minX = 0, minY = 0;
    const pts = [[0, 0]];
    for (const v of vecs) {
        const tv = applyTrans(v[0], v[1], t);
        cx += tv[0]; cy += tv[1];
        pts.push([cx, cy]);
        if (cx < minX) minX = cx;
        if (cy < minY) minY = cy;
    }
    const cells = new Set();
    for (const [x, y] of pts) cells.add((x - minX) + ',' + (y - minY));
    return [...cells].sort().join(';');
}

function computeRelevantTrans(vecs, bothSides) {
    const all = [UP, RIGHT, DOWN, LEFT, UP_BACK, LEFT_BACK, DOWN_BACK, RIGHT_BACK];
    const count = bothSides ? 8 : 4;
    const seen = new Set();
    const relevant = [];
    for (let i = 0; i < count; ++i) {
        const key = chainToCells(vecs, all[i]);
        if (!seen.has(key)) { seen.add(key); relevant.push(all[i]); }
    }
    return relevant;
}

// Parse a JSON piece set produced by PuzzleMaker. Throws on invalid input.
export function parsePieceSet(text, fallbackName) {
    const root = JSON.parse(text);
    const arr = root.pieces;
    if (!Array.isArray(arr) || arr.length === 0)
        throw new Error('pieces 配列が空です');

    const bothSides = root.bothSides !== undefined ? !!root.bothSides : true;
    const pieces = arr.map((pobj, pi) => {
        const vecs = (pobj.vectors || []).map(v => [v[0] | 0, v[1] | 0]);
        return new Piece(vecs, pi + 1, computeRelevantTrans(vecs, bothSides));
    });

    return { description: root.description || fallbackName || '', bothSides, pieces };
}
