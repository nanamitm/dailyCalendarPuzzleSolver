#pragma once
#include <QThread>
#include <QStringList>
#include <atomic>
#include "maker_solver.h"

// Counts solutions per calendar date for a given piece set.
// Results are returned as a QStringList sorted by solution count ascending
// (hardest dates first), ready to display directly.
class AnalysisWorker : public QThread {
    Q_OBJECT
public:
    explicit AnalysisWorker(QObject* parent = nullptr) : QThread(parent) {}

    // Set before calling start()
    std::vector<MakerPiece> pieces;
    int maxCount = 50;

    void requestCancel() { m_cancelled.store(true, std::memory_order_relaxed); }

signals:
    void progressUpdated(int checked, int total);

    // sortedEntries : pre-formatted strings sorted by solution count ascending
    //                 (hardest / fewest solutions first)
    // totalChecked  : number of dates actually processed
    // minSol        : smallest solution count seen
    // maxSol        : largest solution count seen (may be capped at maxCount)
    // avgSol        : average (using capped values)
    // oneSolCount   : number of dates with exactly 1 solution
    // sortedEntries and dateData are parallel arrays (same order).
    // dateData[i] = month*10000 + day*100 + weekday  for the i-th entry.
    void analysisFinished(QStringList sortedEntries, QVector<int> dateData,
                          int totalChecked, int minSol, int maxSol,
                          double avgSol, int oneSolCount, bool wasCancelled);

protected:
    void run() override;

private:
    std::atomic<bool> m_cancelled{false};
};
