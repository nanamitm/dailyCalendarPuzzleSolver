#pragma once
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QStringList>
#include <QVector>
#include <atomic>
#include <vector>
#include <chrono>
#include "polyomino.h"

class SearchWorker : public QThread {
    Q_OBJECT
public:
    explicit SearchWorker(QObject* parent = nullptr) : QThread(parent) {}

    // Configure before calling start()
    int  minSize   = 3;
    int  maxSize   = 5;
    int  minPieces = 9;
    int  maxPieces = 11;
    bool bothSides = true;

    void requestCancel() { m_cancelled.store(true,  std::memory_order_relaxed); }
    void requestPause()  { m_paused.store(true,     std::memory_order_relaxed); }
    void requestResume() {
        m_paused.store(false, std::memory_order_relaxed);
        m_pauseCond.wakeAll();
    }

signals:
    // Emitted periodically (every ~50 combinations)
    void progressUpdated(int partIdx, int totalParts,
                         long long globalCombo, long long globalTotal,
                         int solFound, double elapsedSec,
                         QString partDesc);

    // Emitted each time a valid piece combination is found.
    // shapeStrings : one ASCII-grid string per piece (via shapeToString)
    // comboDesc    : human-readable partition description
    // boardData    : flat [kBYL * kBXL] solved board for reference date Jan 1 Mon
    //                values: -1=off/date, 1..n=piece index (1-based)
    // bothSides    : whether pieces were used with both sides (needed for analysis)
    void solutionFound(QStringList shapeStrings, QString comboDesc,
                       QVector<int> boardData, bool bothSides);

    // Emitted when the entire search is done or cancelled
    void searchFinished(int totalSolutions, double elapsedSec, bool wasCancelled);

protected:
    void run() override;

private:
    std::atomic<bool> m_cancelled{false};
    std::atomic<bool> m_paused{false};
    QMutex            m_pauseMutex;
    QWaitCondition    m_pauseCond;

    // State that lives for the duration of run()
    int       m_totalParts  = 0;
    int       m_solFound    = 0;
    long long m_globalCombo = 0;
    long long m_globalTotal = 0;
    int       m_partIdx     = 0;
    QString   m_partDesc;
    bool      m_bothSides   = true;
    std::chrono::steady_clock::time_point m_t0;

    void enumerate(const std::vector<std::vector<Shape>>& polyGroups,
                   const std::vector<int>& counts,
                   int groupIdx,
                   int polyStart,
                   int chosenInGroup,
                   std::vector<Shape>& current);

    void checkPause();  // blocks while paused; returns when resumed or cancelled
    void emitProgress();
};
