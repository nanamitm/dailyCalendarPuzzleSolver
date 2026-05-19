#pragma once
#include <vector>
#include <utility>
#include <string>

using Cell  = std::pair<int,int>;
using Shape = std::vector<Cell>;   // sorted, min-x=0, min-y=0 (canonical form)

// Apply one of 8 rigid transforms to a cell (index 0-7)
Cell applyTransform(Cell c, int t);

// Translate shape so min-x=0, min-y=0 then sort
Shape normalize(Shape s);

// Canonical form: smallest normalized shape across all 8 transforms
Shape canonical(const Shape& s);

// All distinct free polyominoes of the given size (canonical forms)
std::vector<Shape> generatePolyominoes(int size);

// All unique orientations of s (normalized); 4 transforms if !bothSides, 8 if bothSides
std::vector<Shape> uniqueTransforms(const Shape& s, bool bothSides);

// Multi-line ASCII grid for display
std::string shapeToString(const Shape& s);

// Reconstruct a Shape from the ASCII grid produced by shapeToString
Shape shapeFromString(const std::string& s);
