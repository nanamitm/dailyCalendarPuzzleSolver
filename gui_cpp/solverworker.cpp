#include "solverworker.h"

void SolverWorker::run()
{
    // Each worker is created for one search. Preserve cancellation requested
    // after start() but before this thread enters run().
    // QDate::dayOfWeek() returns 1=Mon … 7=Sun, matching the solver convention
    if (useCustomPieces)
        result = SolveDateCustom(date.dayOfWeek(), date.day(), date.month(),
                                  findAll, customPieceSet, m_cancelled);
    else
        result = SolveDate(date.dayOfWeek(), date.day(), date.month(),
                            findAll, m_cancelled);
    emit solved();
}
