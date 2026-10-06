#include "SolverBackend.h"
#include "searchworker.h"
#include "analysisworker.h"
#include <QCoreApplication>
#include <cstdlib>
#include <iostream>

static void check(bool condition, const char* message)
{
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    for (int i = 0; i < 20; ++i) {
        SolverWorker worker;
        worker.date = QDate(2023,3,27);
        worker.findAll = true;
        worker.requestCancel();
        worker.start();
        check(worker.wait(3000), "solver cancellation timed out");
        check(worker.result.tries == 0, "startup lost solver cancellation");
        // Destroy the backend immediately after starting a search.
        SolverBackend backend;
        backend.solve(2023,3,27,true);
    }
    SearchWorker search;
    search.requestPause();
    search.start();
    QThread::msleep(200);  // let search reach the condition-variable wait
    search.requestCancel();
    check(search.wait(3000), "paused search did not wake on cancellation");

    SearchWorker generating;
    generating.minSize = generating.maxSize = 8;
    generating.start();
    QThread::msleep(10);
    generating.requestCancel();
    check(generating.wait(3000), "polyomino generation ignored cancellation");

    AnalysisWorker analysis;
    analysis.requestCancel();
    analysis.start();
    check(analysis.wait(3000), "analysis cancellation timed out");
    std::cout << "Thread shutdown regressions passed\n";
}
