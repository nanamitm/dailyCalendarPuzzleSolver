import QtQuick

// Draws the puzzle board from flat cell arrays provided by SolverBackend.
// boardData:   list of BDYL*BDXL ints  (-2=off-board, -1=date-cell, 0=empty, 1-10=piece)
// boardLabels: parallel list of display strings (empty string = no label)
Canvas {
    id: root

    property var    boardData:   []
    property var    boardLabels: []
    property bool   darkMode:    false
    property string fontFamily:  "sans-serif"  // set from Main.qml via FontLoader.name

    readonly property int rows: 8   // BDYL
    readonly property int cols: 7   // BDXL

    onBoardDataChanged:   requestPaint()
    onBoardLabelsChanged: requestPaint()
    onWidthChanged:       requestPaint()
    onHeightChanged:      requestPaint()
    onDarkModeChanged:    requestPaint()
    onFontFamilyChanged:  requestPaint()

    onPaint: {
        var ctx = getContext("2d")
        var cell = Math.floor(width / cols)
        var R    = Math.max(4, Math.round(cell * 0.15))

        ctx.clearRect(0, 0, width, height)

        // Shift all drawing by half the border line-width so the top and left
        // piece borders are fully visible rather than half-clipped by the canvas edge.
        var lw  = Math.max(1.5, cell * 0.03)
        var off = lw / 2
        ctx.save()
        ctx.translate(off, off)

        // ── helpers ────────────────────────────────────────────────────────
        function g(r, c) {
            if (r < 0 || r >= rows || c < 0 || c >= cols) return -2
            var v = boardData[r * cols + c]
            return (v !== undefined) ? v : -2
        }

        function roundedCellR(px, py, sz, rad, tlR, trR, brR, blR) {
            ctx.beginPath()
            ctx.moveTo(px + (tlR ? rad : 0), py)
            if (trR) { ctx.lineTo(px+sz-rad, py);     ctx.arcTo(px+sz, py,    px+sz,    py+rad,  rad) }
            else       ctx.lineTo(px+sz,     py)
            if (brR) { ctx.lineTo(px+sz,  py+sz-rad); ctx.arcTo(px+sz, py+sz, px+sz-rad, py+sz, rad) }
            else       ctx.lineTo(px+sz,  py+sz)
            if (blR) { ctx.lineTo(px+rad, py+sz);     ctx.arcTo(px,    py+sz, px,       py+sz-rad, rad) }
            else       ctx.lineTo(px,     py+sz)
            if (tlR) { ctx.lineTo(px,     py+rad);    ctx.arcTo(px,    py,    px+rad,   py,   rad) }
            else       ctx.lineTo(px,     py)
            ctx.closePath()
        }

        function roundedCell(px, py, sz, tlR, trR, brR, blR) {
            roundedCellR(px, py, sz, R, tlR, trR, brR, blR)
        }

        // ── Colors (light / dark) ──────────────────────────────────────────
        var dk = darkMode
        var BG_COLOR    = dk ? "#1a1a1a" : "#e8e8e8"
        var EMPTY_COLOR = dk ? "#383838" : "#c0c0c0"
        var DATE_COLOR  = dk ? "#4a4035" : "#F0EBD2"
        var BORDER_COL  = dk ? "#888888" : "#505050"
        var TEXT_COL    = dk ? "#d8d8d8" : "#1E1E1E"

        // Piece colors — slightly desaturated in dark mode for better contrast
        var PIECE_COLORS = dk ? [
            "",
            "#5a8fb8",   // 1  I   steel blue
            "#e09040",   // 2  s   orange
            "#4aaa70",   // 3  Ls  sea green
            "#c84848",   // 4  T   red
            "#9870b8",   // 5  Q   purple
            "#38b0b0",   // 6  S   cyan
            "#c8a828",   // 7  sl  yellow
            "#c05898",   // 8  L   magenta
            "#58b040",   // 9  U   lime
            "#2898a0",   // 10 LL  teal
        ] : [
            "",
            "#4682B4",   // 1
            "#FFA032",   // 2
            "#3CB371",   // 3
            "#DC5050",   // 4
            "#9467BD",   // 5
            "#40C8C8",   // 6
            "#F0C832",   // 7
            "#DC64B4",   // 8
            "#64C850",   // 9
            "#32B4AA",   // 10
        ]

        var fam      = "'" + fontFamily + "', sans-serif"
        var baseFont = "bold " + Math.round(cell * 0.22) + "px " + fam
        var dateFont = "bold " + Math.round(cell * 0.25) + "px " + fam  // slightly larger for date cells

        ctx.font         = baseFont
        ctx.textAlign    = "center"
        ctx.textBaseline = "middle"

        var r, c, px, py, sz, lbl, grp

        // ── Pass 1: off-board background ───────────────────────────────────
        ctx.fillStyle = BG_COLOR
        for (r = 0; r < rows; ++r)
            for (c = 0; c < cols; ++c)
                if (g(r, c) === -2)
                    ctx.fillRect(c*cell, r*cell, cell, cell)

        // ── Pass 2: empty cells ────────────────────────────────────────────
        for (r = 0; r < rows; ++r) {
            for (c = 0; c < cols; ++c) {
                if (g(r, c) !== 0) continue
                px = c*cell+2; py = r*cell+2; sz = cell-4
                ctx.fillStyle = EMPTY_COLOR
                roundedCell(px, py, sz, true, true, true, true)
                ctx.fill()
                lbl = boardLabels[r * cols + c]
                if (lbl) {
                    ctx.fillStyle = TEXT_COL
                    ctx.fillText(lbl, c*cell + cell/2, r*cell + cell/2)
                }
            }
        }

        // ── Pass 3: piece cells ────────────────────────────────────────────
        for (r = 0; r < rows; ++r) {
            for (c = 0; c < cols; ++c) {
                grp = g(r, c)
                if (grp < 1) continue
                var tO = g(r-1, c) !== grp
                var rO = g(r, c+1) !== grp
                var bO = g(r+1, c) !== grp
                var lO = g(r, c-1) !== grp
                ctx.fillStyle = PIECE_COLORS[grp]
                roundedCell(c*cell, r*cell, cell, tO&&lO, tO&&rO, bO&&rO, bO&&lO)
                ctx.fill()
            }
        }

        // ── Pass 4: piece borders with rounded exterior corners ───────────
        // Edges are shortened by R near convex corners; arcTo() rounds them.
        ctx.strokeStyle = BORDER_COL
        ctx.lineWidth   = Math.max(1.5, cell * 0.03)
        for (r = 0; r < rows; ++r) {
            for (c = 0; c < cols; ++c) {
                grp = g(r, c)
                if (grp < 1) continue
                var x0 = c*cell, y0 = r*cell
                var tO = g(r-1, c) !== grp
                var rO = g(r, c+1) !== grp
                var bO = g(r+1, c) !== grp
                var lO = g(r, c-1) !== grp
                if (!tO && !rO && !bO && !lO) continue

                ctx.beginPath()
                // Top edge
                if (tO) {
                    ctx.moveTo(x0 + (lO ? R : 0),        y0)
                    ctx.lineTo(x0 + cell - (rO ? R : 0), y0)
                }
                // TR corner arc
                if (tO && rO) {
                    ctx.moveTo(x0 + cell - R, y0)
                    ctx.arcTo(x0 + cell, y0, x0 + cell, y0 + R, R)
                }
                // Right edge
                if (rO) {
                    ctx.moveTo(x0 + cell, y0 + (tO ? R : 0))
                    ctx.lineTo(x0 + cell, y0 + cell - (bO ? R : 0))
                }
                // BR corner arc
                if (bO && rO) {
                    ctx.moveTo(x0 + cell, y0 + cell - R)
                    ctx.arcTo(x0 + cell, y0 + cell, x0 + cell - R, y0 + cell, R)
                }
                // Bottom edge
                if (bO) {
                    ctx.moveTo(x0 + cell - (rO ? R : 0), y0 + cell)
                    ctx.lineTo(x0 + (lO ? R : 0),        y0 + cell)
                }
                // BL corner arc
                if (bO && lO) {
                    ctx.moveTo(x0 + R, y0 + cell)
                    ctx.arcTo(x0, y0 + cell, x0, y0 + cell - R, R)
                }
                // Left edge
                if (lO) {
                    ctx.moveTo(x0, y0 + cell - (bO ? R : 0))
                    ctx.lineTo(x0, y0 + (tO ? R : 0))
                }
                // TL corner arc
                if (tO && lO) {
                    ctx.moveTo(x0, y0 + R)
                    ctx.arcTo(x0, y0, x0 + R, y0, R)
                }
                ctx.stroke()
            }
        }

        // ── Pass 5: date label cells ───────────────────────────────────────
        var dR = Math.round(R * 0.6)
        for (r = 0; r < rows; ++r) {
            for (c = 0; c < cols; ++c) {
                if (g(r, c) !== -1) continue
                px = c*cell+2; py = r*cell+2; sz = cell-4
                ctx.fillStyle   = DATE_COLOR
                ctx.strokeStyle = BORDER_COL
                ctx.lineWidth   = 1.5
                roundedCellR(px, py, sz, dR, true, true, true, true)
                ctx.fill()
                ctx.stroke()
                lbl = boardLabels[r * cols + c]
                if (lbl) {
                    ctx.font      = dateFont    // slightly larger than base labels
                    ctx.fillStyle = dk ? "white" : "#111111"   // higher contrast on beige
                    ctx.fillText(lbl, c*cell + cell/2, r*cell + cell/2)
                    ctx.font = baseFont         // restore
                }
            }
        }

        ctx.restore()   // undo translate(off, off)
    }
}
