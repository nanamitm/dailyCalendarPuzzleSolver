# Web (HTML5) version

A browser port of the Qt/Android Daily Calendar Puzzle Solver
(`Android/`, `gui_cpp/`). Plain static files — no build step, no dependencies.

**Live: https://nanamitm.github.io/dailyCalendarPuzzleSolver/**

## Files

| File | Role | Ported from |
|---|---|---|
| `solver-core.js` | Recursive solver, board, pieces, piece-set JSON parsing | `gui_cpp/solver.cpp` |
| `solver-worker.js` | Runs the search in a Web Worker | `gui_cpp/solverworker.cpp` |
| `board.js` | Canvas rendering of the board | `Android/BoardCanvas.qml` |
| `app.js` | UI state, settings, animations, date picker | `Android/SolverBackend.cpp`, `Android/Main.qml` |
| `index.html`, `styles.css` | Markup and theming (light / dark) | `Android/Main.qml` |
| `sw.js`, `manifest.webmanifest` | PWA: offline use and install to home screen | — |

The algorithm is the same as the C++ version: the first free square is found
scanning from the top left, then every remaining piece is tried on it
recursively. Phase 1 searches the frosted side only; if a date has no solution
there, phase 2 retries with pieces 5 (Q) and 2 (small S) flipped. The only
change is that the board is mutated in place and undone instead of being copied
per attempt — the try counts match the C++ build exactly (e.g. 3 026 228 tries
for Monday 27 March).

## Features

- Solving runs in a Web Worker, so the UI stays responsive; **キャンセル** stops it
- 全解探索 (find all solutions), スライドショー (random walk over all solutions,
  2 s interval), 深夜自動更新 (jump to the new day at midnight)
- Custom piece sets: import a PuzzleMaker JSON via the ⚙ menu or by dropping the
  file on the page. Imported sets are kept in `localStorage`; ✕ removes one.
- Settings persist in `localStorage`

## Controls

| Action | Mouse / touch | Keyboard |
|---|---|---|
| Previous / next solution | swipe the board, or ◀ ▶ | ← / → |
| Previous / next day | swipe the date, or ‹ › | ↑ / ↓ |
| Jump to today | long-press the date | T |
| Pick a date | tap the date | — |

## Running locally

Modules and workers need to be served over HTTP (`file://` will not work):

```bash
python -m http.server 8765 --directory web
```

Then open <http://localhost:8765>.

## Deployment

`.github/workflows/pages.yml` publishes this folder to GitHub Pages on every
push to `main` that touches `web/`.
