#include "solver.h"
#include "maker_solver.h"
#include "boardwidget.h"
#include <QApplication>
#include <QDir>
#include <QImage>
#include <QTemporaryFile>
#include <climits>
#include <cstdlib>
#include <iostream>

static void check(bool condition, const char* message)
{
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}

static bool parse(const QByteArray& json)
{
    QTemporaryFile file;
    check(file.open(), "temporary JSON file failed");
    file.write(json); file.flush();
    LoadedPieceSet set;
    QString error;
    return loadPieceSetFromJson(file.fileName(), set, error);
}

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    Trans upOnly[] = {up};
    Board board(1, 1, 1);
    for (int coordinate : {100, -100, INT_MAX, INT_MIN}) {
        Vect vector[] = {{coordinate, 0}};
        Piece piece(vector, 1, 1, upOnly, 1);
        check(board.putPiece(piece, 1, 0) == nullptr, "out-of-bounds placement accepted");
    }
    std::atomic<bool> cancelled{false};
    LoadedPieceSet shortSet;
    shortSet.pieces.emplace_back(nullptr, 0, 1, upOnly, 1);
    check(SolveDateCustom(1,1,1,false,shortSet,cancelled).solutions.empty(),
          "underfilled set accepted");
    for (const QByteArray& json : {
            QByteArray("[]"), QByteArray("{\"pieces\":[]}"),
            QByteArray("{\"pieces\":[{\"vectors\":[[100,0]]}]}"),
            QByteArray("{\"pieces\":[{\"vectors\":[[0.5,0]]}]}"),
            QByteArray("{\"pieces\":[{\"vectors\":[[0,0]]}]}"),
            QByteArray("{\"pieces\":[{\"vectors\":[[2,0]]}]}"),
            QByteArray("{\"pieces\":[{\"vectors\":[]}]}")})
        check(!parse(json), "invalid JSON piece set accepted");
    QDir presets(QString::fromLocal8Bit(argv[1]));
    for (const QString& name : presets.entryList({"*.json"}, QDir::Files)) {
        LoadedPieceSet set;
        QString error;
        check(loadPieceSetFromJson(presets.filePath(name), set, error), "bundled preset rejected");
    }
    LoadedPieceSet singles;
    for (int i = 1; i <= 47; ++i)
        singles.pieces.emplace_back(nullptr, 0, i, upOnly, 1);
    auto full = SolveDateCustom(1,1,1,false,singles,cancelled);
    check(full.solutions.size() == 1, "full set has no solution");
    for (const auto& row : full.solutions[0].cells)
        for (int cell : row) check(cell != 0, "solution contains an empty cell");
    BoardWidget widget;
    widget.setBoard(&full.solutions[0], QDate(2024,1,1));
    QImage image(widget.size(), QImage::Format_ARGB32);
    widget.render(&image);  // exercise all 47 colour indices

    Shape octomino{{0,0},{1,0},{2,0},{3,0},{4,0},{5,0},{6,0},{0,1}};
    std::vector<MakerPiece> octs{makePiece(octomino, true)};
    check(!quickSolve(1,1,1,octs,cancelled), "one octomino filled the board");
    check(countSolutions(1,1,1,octs,10,cancelled) == 0, "octomino count was incorrect");
    auto defaultResult = SolveDate(1,27,3,true,cancelled);
    check(defaultResult.solutions.size() == 1 && defaultResult.tries == 3026228,
          "default solver regression");
    std::cout << "Native solver regressions passed\n";
}
