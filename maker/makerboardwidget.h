#pragma once
#include <QWidget>
#include <QVector>

// Displays a solved puzzle board produced by the maker's quickSolve().
// boardData: flat [kBYL * kBXL] array (row-major).
//   -1  = off-board or date cell
//   1..n = piece index (1-based)
class MakerBoardWidget : public QWidget {
    Q_OBJECT
public:
    explicit MakerBoardWidget(QWidget* parent = nullptr);

    // Pass the flat board, the reference date used for solving, and piece count.
    void setBoard(const QVector<int>& boardData,
                  int month, int day, int weekday,
                  int nPieces);
    void clear();

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent*) override;

private:
    static constexpr int CELL = 52; // pixel size of each cell

    QVector<int> m_cells;    // flat board data
    int m_month   = 0;
    int m_day     = 0;
    int m_weekday = 0;
    int m_nPieces = 0;

    // Returns the calendar label for cell (y, x), e.g. "Jan", "15", "Mon"
    static QString cellLabel(int y, int x);

    // True if (y, x) is one of the fixed cut-corner cells
    static bool isCutCorner(int y, int x);

    // Piece color palette (up to 12 pieces)
    static QColor pieceColor(int pieceIdx);
};
