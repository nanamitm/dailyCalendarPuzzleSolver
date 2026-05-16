#include "solver.h"
#include <chrono>
#include <cstring>

// ── Piece ──────────────────────────────────────────────────────────────────

Piece::Piece(Vect shape[], int shapeLen, int val, Trans relTrans[], int nbRelTrans)
    : shapeLength(shapeLen), value(val), nbRelevantTrans(nbRelTrans)
{
    baseShape     = new Vect[shapeLength];
    currShape     = new Vect[shapeLength];
    relevantTrans = new Trans[nbRelevantTrans];
    for (int i = 0; i < shapeLength;     ++i) baseShape[i]     = shape[i];
    for (int i = 0; i < shapeLength;     ++i) currShape[i]     = shape[i];
    for (int i = 0; i < nbRelevantTrans; ++i) relevantTrans[i] = relTrans[i];
}

Piece::Piece(const Piece& o)
    : shapeLength(o.shapeLength), origin(o.origin), value(o.value),
      nbRelevantTrans(o.nbRelevantTrans)
{
    baseShape     = new Vect[shapeLength];
    currShape     = new Vect[shapeLength];
    relevantTrans = new Trans[nbRelevantTrans];
    for (int i = 0; i < shapeLength;     ++i) baseShape[i]     = o.baseShape[i];
    for (int i = 0; i < shapeLength;     ++i) currShape[i]     = o.currShape[i];
    for (int i = 0; i < nbRelevantTrans; ++i) relevantTrans[i] = o.relevantTrans[i];
}

Piece& Piece::operator=(const Piece& o)
{
    if (this == &o) return *this;
    delete[] baseShape; delete[] currShape; delete[] relevantTrans;
    shapeLength      = o.shapeLength;
    origin           = o.origin;
    value            = o.value;
    nbRelevantTrans  = o.nbRelevantTrans;
    baseShape        = new Vect[shapeLength];
    currShape        = new Vect[shapeLength];
    relevantTrans    = new Trans[nbRelevantTrans];
    for (int i = 0; i < shapeLength;     ++i) baseShape[i]     = o.baseShape[i];
    for (int i = 0; i < shapeLength;     ++i) currShape[i]     = o.currShape[i];
    for (int i = 0; i < nbRelevantTrans; ++i) relevantTrans[i] = o.relevantTrans[i];
    return *this;
}

Piece::~Piece()
{
    delete[] baseShape;
    delete[] currShape;
    delete[] relevantTrans;
}

void Piece::transform(Trans t)
{
    for (int i = 0; i < shapeLength; ++i) {
        switch (t) {
        case up:        currShape[i]  = baseShape[i];                                  break;
        case right:     currShape[i]  = Vect( baseShape[i].y, -baseShape[i].x);        break;
        case down:      currShape[i]  = Vect(-baseShape[i].x, -baseShape[i].y);        break;
        case left:      currShape[i]  = Vect(-baseShape[i].y,  baseShape[i].x);        break;
        case upBack:    currShape[i]  = Vect(-baseShape[i].x,  baseShape[i].y);        break;
        case rightBack: currShape[i]  = Vect( baseShape[i].y,  baseShape[i].x);        break;
        case downBack:  currShape[i]  = Vect( baseShape[i].x, -baseShape[i].y);        break;
        case leftBack:  currShape[i]  = Vect(-baseShape[i].y, -baseShape[i].x);        break;
        }
    }
}

Vect Piece::operator[](int index) const
{
    int idx = index + origin;
    if (index > 0) idx -= 1;
    if (idx >= 0 && idx < shapeLength) return currShape[idx];
    return Vect();
}

// ── Board ──────────────────────────────────────────────────────────────────

Board::Board(int weekday, int monthDay, int month)
{
    next = nullptr;
    // Fill entire array as off-board (-1)
    for (int y = 0; y < BYL; ++y)
        for (int x = 0; x < BXL; ++x)
            cells[y][x] = -1;
    // Mark playable area as empty (0)
    for (int y = BOY; y < BOY + BDYL; ++y)
        for (int x = BOX; x < BOX + BDXL; ++x)
            cells[y][x] = 0;
    // Remove non-playable corners inside the playable bounding box
    cells[3][9] = -1;
    cells[4][9] = -1;
    cells[10][3] = -1;
    cells[10][4] = -1;
    cells[10][5] = -1;
    cells[10][6] = -1;
    // Mark the three date cells as occupied (-1)
    cells[((month-1)/6)+3][((month-1)%6)+3]       = -1;
    cells[((monthDay-1)/7)+5][((monthDay-1)%7)+3]  = -1;
    if ((weekday-1) == 6)
        cells[9][6] = -1;
    else
        cells[((weekday-1)/3)+9][((weekday-1)%3)+7] = -1;
}

Board::Board(const Board& o)
{
    next = nullptr;  // never copy the linked-list pointer
    std::memcpy(cells, o.cells, sizeof(cells));
}

Board& Board::operator=(const Board& o)
{
    if (this != &o) {
        next = nullptr;
        std::memcpy(cells, o.cells, sizeof(cells));
    }
    return *this;
}

void Board::nextAvailablePos(int* ox, int* oy) const
{
    for (int y = BOY; y < BOY + BDYL; ++y) {
        for (int x = BOX; x < BOX + BDXL; ++x) {
            if (cells[y][x] == 0) {
                *ox = x - BOX;
                *oy = y - BOY;
                return;
            }
        }
    }
    *ox = -1; *oy = -1;
}

bool Board::putSquare(int value, int x, int y)
{
    if (cells[BOY + y][BOX + x] == 0) {
        cells[BOY + y][BOX + x] = value;
        return true;
    }
    return false;
}

Board* Board::putPiece(Piece& piece, int posX, int posY) const
{
    Board* nb = new Board(*this);

    if (!nb->putSquare(piece.value, posX, posY)) { delete nb; return nullptr; }

    // Walk backward through the piece vectors
    int idx = -1;
    int cx = posX, cy = posY;
    Vect v = piece[idx];
    while (!v.isNull()) {
        cx -= v.x; cy -= v.y;
        if (!nb->putSquare(piece.value, cx, cy)) { delete nb; return nullptr; }
        --idx;
        v = piece[idx];
    }
    // Walk forward
    idx = 1; cx = posX; cy = posY;
    v = piece[idx];
    while (!v.isNull()) {
        cx += v.x; cy += v.y;
        if (!nb->putSquare(piece.value, cx, cy)) { delete nb; return nullptr; }
        ++idx;
        v = piece[idx];
    }
    return nb;
}

// ── Recursive solver ───────────────────────────────────────────────────────

static bool Solve(Board& board, Piece* pieces[], int nbPieces, Board** sols,
                  int* nbTries, int* nbPlPcs, bool isUniqueSol, bool& keep,
                  std::atomic<bool>& cancelled)
{
    bool isSol = false;
    if (nbPieces != 0) {
        int posX = 0, posY = 0;
        board.nextAvailablePos(&posX, &posY);

        for (int pi = 0; pi < nbPieces && keep; ++pi) {
            if (cancelled.load(std::memory_order_relaxed)) { keep = false; break; }
            Piece* piece = pieces[pi];
            for (int origin = 0; origin <= piece->shapeLength && keep; ++origin) {
                piece->origin = origin;
                for (int ti = 0; ti < piece->nbRelevantTrans && keep; ++ti) {
                    piece->transform(piece->relevantTrans[ti]);
                    Board* nb = board.putPiece(*piece, posX, posY);
                    ++(*nbTries);
                    if (nb) {
                        ++(*nbPlPcs);
                        int newN = nbPieces - 1;
                        Piece** np = newN > 0 ? new Piece*[newN] : nullptr;
                        for (int i = 0, j = 0; i < nbPieces; ++i)
                            if (i != pi) np[j++] = pieces[i];

                        isSol = Solve(*nb, np, newN, sols, nbTries, nbPlPcs,
                                      isUniqueSol, keep, cancelled);
                        if (!isSol) delete nb;
                        if (isUniqueSol && isSol) keep = false;
                        isSol = false;
                        delete[] np;
                    }
                }
            }
        }
    } else {
        // Solution found: prepend to linked list
        if (*sols) board.next = *sols;
        *sols  = &board;
        isSol  = true;
    }
    return isSol;
}

// ── Internal helper: one solve pass with configurable piece flipping ────────
//
// flipSmallS (piece 2): add upBack/rightBack transforms
// flipQ      (piece 5): add all 4 back-side transforms
//
// Phase 1 default (frosted side only, no flip): minimises search space.
// Phase 2 fallback (flip Q + SmallS): covers every date that has no
//         frosted-side solution — matches the original CLI "-t 5 2" suggestion.

static SolverOutput solvePhase(int weekday, int day, int month, bool findAll,
                                bool flipSmallS, bool flipQ,
                                std::atomic<bool>& cancelled)
{
    Trans faceT[4]  = { up, right, down, left };
    Trans upRT2[2]  = { up, right };
    Trans upRT4[4]  = { up, right, upBack, rightBack };
    Trans allT8[8]  = { up, right, down, left, upBack, rightBack, downBack, leftBack };

    Vect fourFlatV[3]   = { {0,1},{0,1},{0,1} };
    Vect smallSV[3]     = { {0,1},{1,0},{0,1} };
    Vect smallLV[3]     = { {0,1},{1,0},{1,0} };
    Vect tV[4]          = { {1,0},{1,0},{-1,1},{0,1} };
    Vect qV[4]          = { {0,1},{1,0},{0,1},{-1,0} };
    Vect bigSV[4]       = { {1,0},{0,1},{0,1},{1,0} };
    Vect smallsTailV[4] = { {1,0},{0,1},{1,0},{1,0} };
    Vect bigLV[4]       = { {0,1},{1,0},{1,0},{1,0} };
    Vect uV[4]          = { {0,1},{1,0},{1,0},{0,-1} };
    Vect lequalV[4]     = { {0,1},{0,1},{1,0},{1,0} };

    // Frosted-side-only defaults; selectively widen for flipped pieces
    Piece FourFlat   (fourFlatV,   3,  1, upRT2,                2);
    Piece SmallS     (smallSV,     3,  2, flipSmallS ? upRT4 : upRT2, flipSmallS ? 4 : 2);
    Piece SmallL     (smallLV,     3,  3, faceT,                4);
    Piece T          (tV,          4,  4, faceT,                4);
    Piece Q          (qV,          4,  5, flipQ     ? allT8 : faceT,  flipQ      ? 8 : 4);
    Piece BigS       (bigSV,       4,  6, upRT2,                2);
    Piece SmallsTail (smallsTailV, 4,  7, faceT,                4);
    Piece BigL       (bigLV,       4,  8, faceT,                4);
    Piece U          (uV,          4,  9, faceT,                4);
    Piece Lequal     (lequalV,     4, 10, faceT,                4);

    Piece* puzzlePieces[10] = {
        &FourFlat, &U, &Q, &SmallsTail, &SmallL,
        &BigL, &SmallS, &Lequal, &BigS, &T
    };

    Board  puzzle(weekday, day, month);
    Board* sols    = nullptr;
    int    tries   = 0;
    int    plPcs   = 0;
    bool   keep    = true;

    Solve(puzzle, puzzlePieces, 10, &sols, &tries, &plPcs, !findAll, keep, cancelled);

    SolverOutput out;
    out.tries     = tries;
    out.pcsPlaced = plPcs;

    Board* cur = sols;
    while (cur) {
        out.solutions.push_back(*cur);
        Board* nxt = cur->next;
        delete cur;
        cur = nxt;
    }
    return out;
}

// ── Public entry point ─────────────────────────────────────────────────────

SolverOutput SolveDate(int weekday, int day, int month, bool findAll,
                        std::atomic<bool>& cancelled)
{
    using clock = std::chrono::high_resolution_clock;
    auto t0 = clock::now();

    // Phase 1: frosted side only — fast, covers most dates
    SolverOutput out = solvePhase(weekday, day, month, findAll, false, false, cancelled);

    // Phase 2: if no solution found, retry flipping Q (5) and SmallS (2)
    //          This covers every date, matching the original "-t 5 2" fallback.
    if (out.solutions.empty() && !cancelled.load()) {
        out = solvePhase(weekday, day, month, findAll, true, true, cancelled);
    }

    out.elapsedMs =
        std::chrono::duration<double, std::milli>(clock::now() - t0).count();
    return out;
}
