#include "polyomino.h"
#include <algorithm>
#include <set>
#include <climits>
#include <sstream>

Cell applyTransform(Cell c, int t)
{
    auto [x, y] = c;
    switch (t) {
        case 0: return { x,  y};
        case 1: return { y, -x};
        case 2: return {-x, -y};
        case 3: return {-y,  x};
        case 4: return {-x,  y};
        case 5: return { y,  x};
        case 6: return { x, -y};
        case 7: return {-y, -x};
    }
    return c;
}

Shape normalize(Shape s)
{
    int minX = INT_MAX, minY = INT_MAX;
    for (auto [x, y] : s) { minX = std::min(minX, x); minY = std::min(minY, y); }
    for (auto& [x, y] : s) { x -= minX; y -= minY; }
    std::sort(s.begin(), s.end());
    return s;
}

Shape canonical(const Shape& s)
{
    Shape best;
    for (int t = 0; t < 8; ++t) {
        Shape ts;
        ts.reserve(s.size());
        for (auto c : s) ts.push_back(applyTransform(c, t));
        ts = normalize(ts);
        if (best.empty() || ts < best) best = ts;
    }
    return best;
}

std::vector<Shape> generatePolyominoes(int size)
{
    if (size < 1) return {};

    std::set<Shape> current;
    current.insert({{0, 0}});

    const int dx[] = {0, 0, 1, -1};
    const int dy[] = {1, -1, 0, 0};

    for (int n = 1; n < size; ++n) {
        std::set<Shape> next;
        for (const auto& poly : current) {
            std::set<Cell> occupied(poly.begin(), poly.end());
            for (auto [cx, cy] : poly) {
                for (int d = 0; d < 4; ++d) {
                    Cell nc = {cx + dx[d], cy + dy[d]};
                    if (occupied.count(nc)) continue;
                    Shape grown = poly;
                    grown.push_back(nc);
                    next.insert(canonical(grown));
                }
            }
        }
        current = std::move(next);
    }

    return std::vector<Shape>(current.begin(), current.end());
}

std::vector<Shape> uniqueTransforms(const Shape& s, bool bothSides)
{
    std::vector<Shape> result;
    std::set<Shape> seen;
    int maxT = bothSides ? 8 : 4;
    for (int t = 0; t < maxT; ++t) {
        Shape ts;
        ts.reserve(s.size());
        for (auto c : s) ts.push_back(applyTransform(c, t));
        ts = normalize(ts);
        if (seen.insert(ts).second)
            result.push_back(ts);
    }
    return result;
}

std::string shapeToString(const Shape& s)
{
    if (s.empty()) return "";
    int maxX = 0, maxY = 0;
    for (auto [x, y] : s) { maxX = std::max(maxX, x); maxY = std::max(maxY, y); }
    std::set<Cell> cells(s.begin(), s.end());
    std::ostringstream oss;
    for (int y = 0; y <= maxY; ++y) {
        for (int x = 0; x <= maxX; ++x)
            oss << (cells.count({x, y}) ? '#' : '.');
        oss << '\n';
    }
    return oss.str();
}

Shape shapeFromString(const std::string& s)
{
    Shape result;
    int x = 0, y = 0;
    for (char c : s) {
        if (c == '\n') { ++y; x = 0; }
        else           { if (c == '#') result.push_back({x, y}); ++x; }
    }
    std::sort(result.begin(), result.end());
    return result;
}
