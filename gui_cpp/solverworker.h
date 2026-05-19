#pragma once
#include <QThread>
#include <QDate>
#include <atomic>
#include "solver.h"

class SolverWorker : public QThread {
    Q_OBJECT
public:
    explicit SolverWorker(QObject* parent = nullptr) : QThread(parent) {}

    // Set before calling start()
    QDate          date;
    bool           findAll        = false;
    bool           useCustomPieces = false;
    LoadedPieceSet customPieceSet;  // deep copy – safe to write before start()

    // Read after solved() signal is received
    SolverOutput result;

    // Call from any thread to abort the search
    void requestCancel() { m_cancelled.store(true, std::memory_order_relaxed); }

signals:
    void solved();

protected:
    void run() override;

private:
    std::atomic<bool> m_cancelled{false};
};
