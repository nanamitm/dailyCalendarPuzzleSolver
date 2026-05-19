#include "solverworker.h"

void SolverWorker::run()
{
    m_cancelled.store(false, std::memory_order_relaxed);
    // QDate::dayOfWeek() returns 1=Mon … 7=Sun, matching the solver convention
    if (useCustomPieces)
        result = SolveDateCustom(date.dayOfWeek(), date.day(), date.month(),
                                  findAll, customPieceSet, m_cancelled);
    else
        result = SolveDate(date.dayOfWeek(), date.day(), date.month(),
                            findAll, m_cancelled);
    emit solved();
}
