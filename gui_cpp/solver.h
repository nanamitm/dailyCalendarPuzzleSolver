#pragma once
#include <vector>
#include <atomic>

// ── Board array dimensions ─────────────────────────────────────────────────
constexpr int BXL  = 13;   // total X length (with padding)
constexpr int BYL  = 14;   // total Y length (with padding)
constexpr int BOX  = 3;    // playable-area origin X
constexpr int BOY  = 3;    // playable-area origin Y
constexpr int BDXL = 7;    // playable X span
constexpr int BDYL = 8;    // playable Y span

enum Trans { up, right, down, left, upBack, leftBack, downBack, rightBack };

// ── Vect ───────────────────────────────────────────────────────────────────
class Vect {
public:
    int x = 0, y = 0;
    Vect() = default;
    Vect(int vx, int vy) : x(vx), y(vy) {}
    bool isNull() const { return x == 0 && y == 0; }
};

// ── Piece ──────────────────────────────────────────────────────────────────
class Piece {
public:
    Vect*         baseShape;
    Vect*         currShape;
    int           shapeLength;
    int           origin = 0;
    unsigned char value;
    Trans*        relevantTrans;
    int           nbRelevantTrans;

    Piece(Vect shape[], int shapeLen, int val, Trans relTrans[], int nbRelTrans);
    Piece(const Piece& o);
    Piece& operator=(const Piece& o);
    ~Piece();

    void transform(Trans t);
    Vect operator[](int index) const;
};

// ── Board ──────────────────────────────────────────────────────────────────
// cells[y][x]: -1 = off-board/date cell, 0 = empty, 1-10 = piece value
class Board {
public:
    int    cells[BYL][BXL];
    Board* next = nullptr;

    // weekday: 1=Monday … 7=Sunday
    Board(int weekday, int monthDay, int month);
    Board(const Board& o);
    Board& operator=(const Board& o);

    void   nextAvailablePos(int* ox, int* oy) const;
    Board* putPiece(Piece& piece, int posX, int posY) const;

private:
    bool putSquare(int value, int x, int y);
};

// ── Solver output ──────────────────────────────────────────────────────────
struct SolverOutput {
    std::vector<Board> solutions;
    int    tries      = 0;
    int    pcsPlaced  = 0;
    double elapsedMs  = 0.0;
};

// weekday: 1=Monday … 7=Sunday  (matches QDate::dayOfWeek())
// cancelled: set to true from another thread to abort the search early
SolverOutput SolveDate(int weekday, int day, int month, bool findAll,
                        std::atomic<bool>& cancelled);
