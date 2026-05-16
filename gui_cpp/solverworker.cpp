#include "solverworker.h"

void SolverWorker::run()
{
    // QDate::dayOfWeek() returns 1=Mon … 7=Sun, matching the solver convention
    m_cancelled.store(false, std::memory_order_relaxed);
    result = SolveDate(date.dayOfWeek(), date.day(), date.month(), findAll, m_cancelled);
    emit solved();
}
