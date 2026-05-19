#include "makerboardwidget.h"
#include "maker_solver.h"
#include <QPainter>
#include <QPen>
#include <QFont>
#include <set>

// ── Color palette ─────────────────────────────────────────────────────────────

QColor MakerBoardWidget::pieceColor(int idx)
{
    static const QColor kColors[] = {
        {255, 100, 100}, // red
        {255, 190,  80}, // orange
        {255, 240,  80}, // yellow
        { 80, 200,  80}, // green
        { 80, 220, 220}, // cyan
        { 80, 150, 255}, // blue
        {180,  80, 255}, // purple
        {255,  80, 210}, // pink
        {160, 255, 160}, // light green
        {255, 200, 150}, // peach
        {150, 200, 255}, // baby blue
        {200, 255, 200}, // mint
    };
    constexpr int N = static_cast<int>(sizeof(kColors) / sizeof(kColors[0]));
    return kColors[idx % N];
}

// ── Cell metadata ─────────────────────────────────────────────────────────────

bool MakerBoardWidget::isCutCorner(int y, int x)
{
    return ((y == 3 || y == 4) && x == 9) ||
           (y == 10 && x >= 3 && x <= 6);
}

QString MakerBoardWidget::cellLabel(int y, int x)
{
    // Month cells: rows 3-4, cols 3-8
    if (y == 3 && x >= 3 && x <= 8) {
        static const char* kM[] = {"Jan","Feb","Mar","Apr","May","Jun"};
        return kM[x - 3];
    }
    if (y == 4 && x >= 3 && x <= 8) {
        static const char* kM[] = {"Jul","Aug","Sep","Oct","Nov","Dec"};
        return kM[x - 3];
    }
    // Day cells: rows 5-9, cols 3-9, days 1-31
    if (y >= 5 && y <= 9 && x >= 3) {
        int d = (y - 5) * 7 + (x - 3) + 1;
        if (d >= 1 && d <= 31) return QString::number(d);
    }
    // Weekday cells
    if (y == 9 && x == 6) return "Sun";
    if (y == 9 && x == 7) return "Mon";
    if (y == 9 && x == 8) return "Tue";
    if (y == 9 && x == 9) return "Wed";
    if (y == 10 && x == 7) return "Thu";
    if (y == 10 && x == 8) return "Fri";
    if (y == 10 && x == 9) return "Sat";
    return "";
}

// ── Widget implementation ─────────────────────────────────────────────────────

MakerBoardWidget::MakerBoardWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(sizeHint());
}

QSize MakerBoardWidget::sizeHint() const
{
    return {kBDXL * CELL, kBDYL * CELL};
}

QSize MakerBoardWidget::minimumSizeHint() const
{
    return sizeHint();
}

void MakerBoardWidget::setBoard(const QVector<int>& boardData,
                                 int month, int day, int weekday, int nPieces)
{
    m_cells   = boardData;
    m_month   = month;
    m_day     = day;
    m_weekday = weekday;
    m_nPieces = nPieces;
    update();
}

void MakerBoardWidget::clear()
{
    m_cells.clear();
    update();
}

void MakerBoardWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    // Background – adapts to light/dark palette
    p.fillRect(rect(), palette().window());

    if (m_cells.isEmpty()) return;

    QFont labelFont = p.font();
    labelFont.setPointSize(9);
    labelFont.setBold(false);
    QFont dateFont = labelFont;
    dateFont.setBold(true);

    // Date cell: use system button colours so it contrasts with the background
    // in both light and dark mode.
    const QColor dateColor   = palette().button().color();
    const QColor dateFgColor = palette().buttonText().color();
    // Thin-border colour: mid tone between window and window-text.
    const QColor borderColor = palette().mid().color();

    for (int y = kBOY; y < kBOY + kBDYL; ++y) {
        for (int x = kBOX; x < kBOX + kBDXL; ++x) {
            if (isCutCorner(y, x)) continue;

            int wx = (x - kBOX) * CELL;
            int wy = (y - kBOY) * CELL;
            QRect cell(wx, wy, CELL, CELL);

            int val = m_cells.isEmpty() ? -1 : m_cells[y * kBXL + x];
            QString label = cellLabel(y, x);

            if (val == -1) {
                // Date cell (cut corners already skipped above)
                p.fillRect(cell, dateColor);
                p.setFont(dateFont);
                p.setPen(dateFgColor);
                p.drawText(cell, Qt::AlignCenter, label);
            } else {
                // Piece cell – piece colours are always bright, so black text
                // gives maximum contrast regardless of light/dark mode.
                QColor bg = pieceColor(val - 1);
                p.fillRect(cell, bg);
                p.setFont(labelFont);
                p.setPen(Qt::black);
                p.drawText(cell, Qt::AlignCenter, label);
            }

            // Thin cell border
            p.setPen(QPen(borderColor, 1));
            p.drawRect(cell);
        }
    }

    // Second pass: thick borders between different pieces (always dark for
    // contrast against the bright piece colours)
    QPen thickPen(QColor(40, 40, 40), 2);
    for (int y = kBOY; y < kBOY + kBDYL; ++y) {
        for (int x = kBOX; x < kBOX + kBDXL; ++x) {
            if (isCutCorner(y, x)) continue;
            int val  = m_cells[y * kBXL + x];
            int wx   = (x - kBOX) * CELL;
            int wy   = (y - kBOY) * CELL;

            // Right edge
            if (x + 1 < kBOX + kBDXL && !isCutCorner(y, x + 1)) {
                int rval = m_cells[y * kBXL + x + 1];
                if (val != rval) {
                    p.setPen(thickPen);
                    p.drawLine(wx + CELL, wy, wx + CELL, wy + CELL);
                }
            }
            // Bottom edge
            if (y + 1 < kBOY + kBDYL && !isCutCorner(y + 1, x)) {
                int bval = m_cells[(y + 1) * kBXL + x];
                if (val != bval) {
                    p.setPen(thickPen);
                    p.drawLine(wx, wy + CELL, wx + CELL, wy + CELL);
                }
            }
        }
    }
}
