#include "boardwidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QMap>
#include <QPair>

// Build a rect path where each corner is independently rounded or sharp.
// tlR/trR/brR/blR = true → round that corner with radius r.
static QPainterPath cellPath(int px, int py, int sz, int r,
                              bool tlR, bool trR, bool brR, bool blR)
{
    QPainterPath path;
    path.moveTo(px + (tlR ? r : 0), py);
    // top edge → top-right
    if (trR) { path.lineTo(px+sz-r, py);     path.arcTo(px+sz-2*r, py,       2*r, 2*r,  90, -90); }
    else      { path.lineTo(px+sz,   py); }
    // right edge → bottom-right
    if (brR) { path.lineTo(px+sz,   py+sz-r); path.arcTo(px+sz-2*r, py+sz-2*r, 2*r, 2*r,   0, -90); }
    else      { path.lineTo(px+sz,   py+sz); }
    // bottom edge → bottom-left
    if (blR) { path.lineTo(px+r,    py+sz);   path.arcTo(px,       py+sz-2*r, 2*r, 2*r, 270, -90); }
    else      { path.lineTo(px,      py+sz); }
    // left edge → top-left
    if (tlR) { path.lineTo(px,      py+r);    path.arcTo(px,       py,        2*r, 2*r, 180, -90); }
    else      { path.lineTo(px,      py); }
    path.closeSubpath();
    return path;
}

// Piece colours (same palette as Python version)
static const QColor PIECE_COLORS[11] = {
    {},                         //  0  unused
    QColor( 70, 130, 180),      //  1  I   – steel blue
    QColor(255, 160,  50),      //  2  s   – orange
    QColor( 60, 179, 113),      //  3  Ls  – sea green
    QColor(220,  80,  80),      //  4  T   – red
    QColor(148, 103, 189),      //  5  Q   – purple
    QColor( 64, 200, 200),      //  6  S   – cyan
    QColor(240, 200,  50),      //  7  sl  – yellow
    QColor(220, 100, 180),      //  8  L   – magenta
    QColor(100, 200,  80),      //  9  U   – lime
    QColor( 50, 180, 170),      // 10  LL  – teal
};

static const QColor DATE_BG  ( 240, 235, 210);
static const QColor BORDER   (  80,  80,  80);
static const QColor TEXT_COL (  30,  30,  30);

static const char* MONTH_ABBR[] = {
    "Jan","Feb","Mar","Apr","May","Jun",
    "Jul","Aug","Sep","Oct","Nov","Dec"
};
static const char* DAY_ABBR[] = {
    "Mon","Tue","Wed","Thu","Fri","Sat","Sun"
};

BoardWidget::BoardWidget(QWidget* parent) : QWidget(parent)
{
    setFixedSize(BDXL * CELL, BDYL * CELL);
}

void BoardWidget::setBoard(const Board* board, QDate date)
{
    m_board = board;
    m_date  = date;
    update();
}

void BoardWidget::setDate(QDate date)
{
    m_board = nullptr;
    m_date  = date;
    update();
}

void BoardWidget::clear()
{
    m_board = nullptr;
    m_date  = QDate();
    update();
}

using CellMap = QMap<QPair<int,int>, QString>;

// All labels for every playable cell (used when no board is loaded)
static CellMap buildAllLabelsMap()
{
    CellMap m;
    static const char* MONTHS[] = {
        "Jan","Feb","Mar","Apr","May","Jun",
        "Jul","Aug","Sep","Oct","Nov","Dec"
    };
    static const char* WEEKDAYS[] = { "Mon","Tue","Wed","Thu","Fri","Sat" };

    for (int i = 0; i < 12; ++i)
        m[{(i/6)+BOY, (i%6)+BOX}] = MONTHS[i];

    for (int i = 0; i < 31; ++i)
        m[{(i/7)+BOY+2, (i%7)+BOX}] = QString("%1").arg(i+1, 2, 10, QChar('0'));

    m[{9, 6}] = "Sun";
    for (int i = 0; i < 6; ++i)
        m[{(i/3)+9, (i%3)+7}] = WEEKDAYS[i];

    return m;
}

// Build a map of (row,col) → date label for the current date
static CellMap buildDateMap(QDate d)
{
    CellMap m;
    if (!d.isValid()) return m;

    int mo  = d.month() - 1;         // 0-based
    int dy  = d.day() - 1;           // 0-based
    int wd  = d.dayOfWeek() - 1;     // 0=Mon … 6=Sun

    m[{(mo/6)+BOY,       (mo%6)+BOX}]  = MONTH_ABBR[mo];
    m[{(dy/7)+BOY+2,     (dy%7)+BOX}]  = QString("%1").arg(d.day(), 2, 10, QChar('0'));

    if (wd == 6)
        m[{9, 6}] = DAY_ABBR[6];
    else
        m[{(wd/3)+9, (wd%3)+7}] = DAY_ABBR[wd];

    return m;
}

void BoardWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setFont(QFont("Segoe UI", 9, QFont::Bold));

    CellMap dateMap = buildDateMap(m_date);
    // When no board is loaded show every cell's label; when solved only date cells get labels
    const CellMap& labelMap = (m_board == nullptr) ? buildAllLabelsMap() : dateMap;

    // Returns the "group" of a board cell:
    //   -2 = off-board/corner   -1 = date label
    //    0 = empty               1-10 = piece value
    auto cellGroup = [&](int row, int col) -> int {
        if (row < BOY || row >= BOY+BDYL || col < BOX || col >= BOX+BDXL)
            return -2;

        int v = -1;
        if (m_board) {
            v = m_board->cells[row][col];
        } else if (m_date.isValid()) {
            bool corner = ((row==3||row==4) && col==9) ||
                           (row==10 && col>=3 && col<=6);
            v = corner ? -1 : 0;
        }

        if (v == -1)
            return dateMap.contains({row, col}) ? -1 : -2;
        return v;
    };

    constexpr int R = 10;  // corner radius for pieces and empty cells

    // ── Pass 1: off-board background (use window palette for dark-mode compat.)
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setPen(Qt::NoPen);
    p.setBrush(palette().color(QPalette::Window));
    for (int r = 0; r < BDYL; ++r) {
        for (int c = 0; c < BDXL; ++c) {
            if (cellGroup(r+BOY, c+BOX) == -2)
                p.drawRect(c*CELL, r*CELL, CELL, CELL);
        }
    }

    // ── Pass 2: empty cells as rounded rects with optional label ─────────
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setBrush(palette().color(QPalette::Mid));
    for (int r = 0; r < BDYL; ++r) {
        for (int c = 0; c < BDXL; ++c) {
            int row = r+BOY, col = c+BOX;
            if (cellGroup(row, col) != 0) continue;
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(c*CELL+2, r*CELL+2, CELL-4, CELL-4, R, R);
            auto key = QPair<int,int>(row, col);
            if (labelMap.contains(key)) {
                p.setPen(palette().color(QPalette::Text));
                p.drawText(c*CELL, r*CELL, CELL, CELL, Qt::AlignCenter, labelMap[key]);
            }
        }
    }

    // ── Pass 3: piece cells with selective corner rounding ────────────────
    for (int r = 0; r < BDYL; ++r) {
        for (int c = 0; c < BDXL; ++c) {
            int row = r+BOY, col = c+BOX;
            int g   = cellGroup(row, col);
            if (g < 1) continue;

            int px = c*CELL, py = r*CELL;
            bool tO = cellGroup(row-1, col) != g;   // top open
            bool rO = cellGroup(row, col+1) != g;   // right open
            bool bO = cellGroup(row+1, col) != g;   // bottom open
            bool lO = cellGroup(row, col-1) != g;   // left open

            auto path = cellPath(px, py, CELL, R,
                                 tO && lO,   // top-left
                                 tO && rO,   // top-right
                                 bO && rO,   // bottom-right
                                 bO && lO);  // bottom-left
            p.setPen(Qt::NoPen);
            p.setBrush(PIECE_COLORS[g]);
            p.drawPath(path);
        }
    }

    // ── Pass 4: piece perimeter border lines ──────────────────────────────
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setPen(QPen(BORDER, 2));
    for (int r = 0; r < BDYL; ++r) {
        for (int c = 0; c < BDXL; ++c) {
            int row = r+BOY, col = c+BOX;
            int g   = cellGroup(row, col);
            if (g < 1) continue;

            int px = c*CELL, py = r*CELL;
            if (cellGroup(row-1, col) != g) p.drawLine(px,      py,       px+CELL, py      );
            if (cellGroup(row+1, col) != g) p.drawLine(px,      py+CELL,  px+CELL, py+CELL );
            if (cellGroup(row, col-1) != g) p.drawLine(px,      py,       px,      py+CELL );
            if (cellGroup(row, col+1) != g) p.drawLine(px+CELL, py,       px+CELL, py+CELL );
        }
    }

    // ── Pass 3: date label cells (rounded rect + text, drawn on top) ─────
    p.setRenderHint(QPainter::Antialiasing, true);
    for (int r = 0; r < BDYL; ++r) {
        for (int c = 0; c < BDXL; ++c) {
            int row = r + BOY, col = c + BOX;
            if (cellGroup(row, col) != -1) continue;

            int px = c * CELL, py = r * CELL;
            p.setPen(QPen(BORDER, 1.5));
            p.setBrush(DATE_BG);
            p.drawRoundedRect(px+2, py+2, CELL-4, CELL-4, 6, 6);
            p.setPen(TEXT_COL);
            p.drawText(px, py, CELL, CELL, Qt::AlignCenter,
                       dateMap[{row, col}]);
        }
    }
}
