#include "maker_solver.h"
#include <cstring>

// Use the board layout constants exposed in maker_solver.h
static constexpr int BXL  = kBXL;
static constexpr int BYL  = kBYL;
static constexpr int BOX  = kBOX;
static constexpr int BOY  = kBOY;
static constexpr int BDXL = kBDXL;
static constexpr int BDYL = kBDYL;

// ── MakerPiece ────────────────────────────────────────────────────────────────

MakerPiece makePiece(const Shape& s, bool bothSides)
{
    MakerPiece p;
    p.canonical   = s;
    p.transforms  = uniqueTransforms(s, bothSides);
    return p;
}

// ── Board initialisation ──────────────────────────────────────────────────────

static void initBoard(int board[BYL][BXL], int month, int day, int weekday)
{
    // Everything off-board
    for (int y = 0; y < BYL; ++y)
        for (int x = 0; x < BXL; ++x)
            board[y][x] = -1;

    // Mark playable area empty
    for (int y = BOY; y < BOY + BDYL; ++y)
        for (int x = BOX; x < BOX + BDXL; ++x)
            board[y][x] = 0;

    // Non-playable corners inside the bounding box
    board[3][9] = -1;  board[4][9] = -1;
    board[10][3] = -1; board[10][4] = -1;
    board[10][5] = -1; board[10][6] = -1;

    // Three date cells
    board[((month-1)/6)+3][((month-1)%6)+3]     = -1;
    board[((day-1)/7)+5]  [((day-1)%7)+3]       = -1;
    if (weekday == 7)
        board[9][6] = -1;
    else
        board[((weekday-1)/3)+9][((weekday-1)%3)+7] = -1;
}

// ── Recursive backtracking solver ─────────────────────────────────────────────

static bool solveRec(int board[BYL][BXL],
                     const std::vector<MakerPiece>& pieces,
                     int usedMask,
                     std::atomic<bool>& cancelled)
{
    // Find the first empty cell (top-left scan)
    int px = -1, py = -1;
    for (int y = BOY; y < BOY + BDYL && px < 0; ++y)
        for (int x = BOX; x < BOX + BDXL && px < 0; ++x)
            if (board[y][x] == 0) { px = x; py = y; }

    if (px < 0) return true;   // board fully covered → solution found
    if (cancelled.load(std::memory_order_relaxed)) return false;

    int n = static_cast<int>(pieces.size());
    for (int pi = 0; pi < n; ++pi) {
        if (usedMask & (1 << pi)) continue;

        for (const Shape& trans : pieces[pi].transforms) {
            // Try each cell in this orientation as the anchor landing on (px, py)
            for (auto [cx, cy] : trans) {
                int placed[7][2];
                int np = 0;
                bool ok = true;

                for (auto [dx, dy] : trans) {
                    int nx = px + dx - cx;
                    int ny = py + dy - cy;
                    if (ny < 0 || ny >= BYL || nx < 0 || nx >= BXL ||
                        board[ny][nx] != 0) { ok = false; break; }
                    placed[np][0] = nx;
                    placed[np][1] = ny;
                    ++np;
                }
                if (!ok) continue;

                for (int i = 0; i < np; ++i) board[placed[i][1]][placed[i][0]] = pi + 1;
                if (solveRec(board, pieces, usedMask | (1 << pi), cancelled))
                    return true;
                for (int i = 0; i < np; ++i) board[placed[i][1]][placed[i][0]] = 0;
            }
        }
    }
    return false;
}

// ── Public API ────────────────────────────────────────────────────────────────

bool quickSolve(int month, int day, int weekday,
                const std::vector<MakerPiece>& pieces,
                std::atomic<bool>& cancelled,
                std::vector<int>* outBoard)
{
    int board[BYL][BXL];
    initBoard(board, month, day, weekday);
    bool found = solveRec(board, pieces, 0, cancelled);
    if (found && outBoard) {
        // When solveRec returns true it does NOT undo the last placement,
        // so `board` holds the complete solution at this point.
        outBoard->resize(BYL * BXL);
        for (int y = 0; y < BYL; ++y)
            for (int x = 0; x < BXL; ++x)
                (*outBoard)[y * BXL + x] = board[y][x];
    }
    return found;
}

bool testAllDates(const std::vector<MakerPiece>& pieces,
                  std::atomic<bool>& cancelled)
{
    // Days per month; Feb gets 29 (covers leap years)
    static constexpr int kDays[] = {0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    for (int month = 1; month <= 12; ++month)
        for (int day = 1; day <= kDays[month]; ++day)
            for (int weekday = 1; weekday <= 7; ++weekday) {
                if (cancelled.load(std::memory_order_relaxed)) return false;
                if (!quickSolve(month, day, weekday, pieces, cancelled))
                    return false;
            }
    return true;
}

// ── Solution counter ──────────────────────────────────────────────────────────

static void countRec(int board[BYL][BXL],
                     const std::vector<MakerPiece>& pieces,
                     int usedMask,
                     int maxCount,
                     int& count,
                     std::atomic<bool>& cancelled)
{
    if (cancelled.load(std::memory_order_relaxed) || count >= maxCount) return;

    int px = -1, py = -1;
    for (int y = BOY; y < BOY + BDYL && px < 0; ++y)
        for (int x = BOX; x < BOX + BDXL && px < 0; ++x)
            if (board[y][x] == 0) { px = x; py = y; }

    if (px < 0) { ++count; return; }   // solution found, keep counting

    int n = static_cast<int>(pieces.size());
    for (int pi = 0; pi < n; ++pi) {
        if (usedMask & (1 << pi)) continue;
        for (const Shape& trans : pieces[pi].transforms) {
            for (auto [cx, cy] : trans) {
                int placed[7][2]; int np = 0; bool ok = true;
                for (auto [dx, dy] : trans) {
                    int nx = px + dx - cx, ny = py + dy - cy;
                    if (ny < 0 || ny >= BYL || nx < 0 || nx >= BXL ||
                        board[ny][nx] != 0) { ok = false; break; }
                    placed[np][0] = nx; placed[np][1] = ny; ++np;
                }
                if (!ok) continue;
                for (int i = 0; i < np; ++i) board[placed[i][1]][placed[i][0]] = pi + 1;
                countRec(board, pieces, usedMask | (1 << pi), maxCount, count, cancelled);
                for (int i = 0; i < np; ++i) board[placed[i][1]][placed[i][0]] = 0;
                if (count >= maxCount || cancelled.load()) return;
            }
        }
    }
}

int countSolutions(int month, int day, int weekday,
                   const std::vector<MakerPiece>& pieces,
                   int maxCount,
                   std::atomic<bool>& cancelled)
{
    int board[BYL][BXL];
    initBoard(board, month, day, weekday);
    int count = 0;
    countRec(board, pieces, 0, maxCount, count, cancelled);
    return count;
}
