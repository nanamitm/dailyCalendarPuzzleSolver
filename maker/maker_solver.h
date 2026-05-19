#pragma once
#include <vector>
#include <atomic>
#include "polyomino.h"

// ── Board layout constants (shared with board widget) ─────────────────────────
constexpr int kBXL  = 13;
constexpr int kBYL  = 14;
constexpr int kBOX  = 3;
constexpr int kBOY  = 3;
constexpr int kBDXL = 7;
constexpr int kBDYL = 8;

struct MakerPiece {
    Shape canonical;
    std::vector<Shape> transforms; // all unique orientations (normalized)
};

MakerPiece makePiece(const Shape& s, bool bothSides);

// Returns true if at least one solution exists for this date configuration.
// weekday: 1=Mon … 7=Sun
// If outBoard is non-null and a solution is found, it is filled with the solved
// board as a flat [kBYL * kBXL] array (row-major). Values: -1=off/date, 1..n=piece.
bool quickSolve(int month, int day, int weekday,
                const std::vector<MakerPiece>& pieces,
                std::atomic<bool>& cancelled,
                std::vector<int>* outBoard = nullptr);

// Tests all valid calendar date configurations (month 1-12, day 1-{28/29/30/31},
// weekday 1-7).  Returns true only if every configuration is solvable.
bool testAllDates(const std::vector<MakerPiece>& pieces,
                  std::atomic<bool>& cancelled);

// Counts solutions for one date up to maxCount (returns maxCount if there are
// ≥ maxCount solutions).  weekday: 1=Mon … 7=Sun.
int countSolutions(int month, int day, int weekday,
                   const std::vector<MakerPiece>& pieces,
                   int maxCount,
                   std::atomic<bool>& cancelled);
