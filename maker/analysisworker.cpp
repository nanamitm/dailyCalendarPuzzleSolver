#include "analysisworker.h"
#include <algorithm>
#include <thread>
#include <vector>
#include <atomic>
#ifdef Q_OS_WIN
#  include <windows.h>
#elif defined(Q_OS_LINUX)
#  include <unistd.h>
#endif

void AnalysisWorker::run()
{
    m_cancelled.store(false, std::memory_order_relaxed);

    static constexpr int kDays[] = {0,31,29,31,30,31,30,31,31,30,31,30,31};
    static const char* kWd[]     = {"","月","火","水","木","金","土","日"};

    // ── Enumerate all date configurations as three parallel int arrays ─────────
    std::vector<int> dtMonth, dtDay, dtWday;
    dtMonth.reserve(2562); dtDay.reserve(2562); dtWday.reserve(2562);
    for (int m = 1; m <= 12; ++m)
        for (int d = 1; d <= kDays[m]; ++d)
            for (int wd = 1; wd <= 7; ++wd) {
                dtMonth.push_back(m);
                dtDay.push_back(d);
                dtWday.push_back(wd);
            }
    int total = static_cast<int>(dtMonth.size());

    // ── One int slot per date (-1 = not yet processed) ────────────────────────
    // Each index is claimed by exactly one thread via nextIdx.fetch_add,
    // so concurrent writes go to distinct slots — no data race.
    std::vector<int> solCounts(total, -1);

    // Leave one core free for the OS and other applications.
    // hardware_concurrency() can return 0 on unusual systems, so clamp to [1, ...].
    int nThreads = static_cast<int>(
        std::max(1u, std::thread::hardware_concurrency() - 1u));
    std::atomic<int> nextIdx{0};
    std::atomic<int> doneCount{0};
    std::atomic<int> activeThreads{nThreads};

    // ── Worker function ────────────────────────────────────────────────────────
    auto workerFn = [&]() {
#ifdef Q_OS_WIN
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
#elif defined(Q_OS_LINUX)
        nice(10);   // lower priority for this thread only (Linux treats threads as tasks)
#endif
        while (true) {
            int idx = nextIdx.fetch_add(1, std::memory_order_relaxed);
            if (idx >= total || m_cancelled.load(std::memory_order_relaxed)) break;

            int cnt = countSolutions(dtMonth[idx], dtDay[idx], dtWday[idx],
                                     pieces, maxCount, m_cancelled);

            // Discard partial result if cancelled mid-count
            if (m_cancelled.load(std::memory_order_relaxed) && cnt < maxCount) break;

            solCounts[idx] = cnt;
            doneCount.fetch_add(1, std::memory_order_relaxed);
        }
        activeThreads.fetch_sub(1, std::memory_order_release);
    };

    // ── Launch worker threads ──────────────────────────────────────────────────
    std::vector<std::thread> threads;
    threads.reserve(nThreads);
    for (int t = 0; t < nThreads; ++t)
        threads.emplace_back(workerFn);

    // ── Emit progress from the QThread context while workers run (every 100ms) ─
    while (activeThreads.load(std::memory_order_acquire) > 0) {
        emit progressUpdated(doneCount.load(std::memory_order_relaxed), total);
        QThread::msleep(100);
    }
    // Join ensures all slot writes are visible before we read them
    for (auto& t : threads) t.join();
    emit progressUpdated(doneCount.load(), total);

    // ── Collect processed results ──────────────────────────────────────────────
    // Build four parallel int arrays: count, month, day, weekday
    std::vector<int> resCount, resMonth, resDay, resWday;
    resCount.reserve(doneCount.load());
    for (int i = 0; i < total; ++i) {
        if (solCounts[i] < 0) continue;
        resCount.push_back(solCounts[i]);
        resMonth.push_back(dtMonth[i]);
        resDay.push_back(dtDay[i]);
        resWday.push_back(dtWday[i]);
    }

    int n = static_cast<int>(resCount.size());
    if (n == 0) {
        emit analysisFinished({}, {}, 0, 0, 0, 0.0, 0, m_cancelled.load());
        return;
    }

    // ── Sort hardest (fewest solutions) first ──────────────────────────────────
    // Build index array and sort by count
    std::vector<int> order(n);
    for (int i = 0; i < n; ++i) order[i] = i;
    std::sort(order.begin(), order.end(),
        [&](int a, int b) { return resCount[a] < resCount[b]; });

    int minSol      = resCount[order.front()];
    int maxSol      = resCount[order.back()];
    long long sumSol = 0;
    int oneSolCount = 0;

    QStringList labels;
    QVector<int> dateData;
    labels.reserve(n);
    dateData.reserve(n);

    for (int i : order) {
        int cnt = resCount[i];
        sumSol += cnt;
        if (cnt == 1) ++oneSolCount;

        QString countStr = (cnt == maxCount)
            ? QString("≥%1").arg(cnt)
            : QString::number(cnt);

        labels << QString("%1解  %2月%3日 (%4)")
            .arg(countStr, 5)
            .arg(resMonth[i], 2)
            .arg(resDay[i],   2)
            .arg(kWd[resWday[i]]);
        dateData << (resMonth[i] * 10000 + resDay[i] * 100 + resWday[i]);
    }

    double avg = double(sumSol) / double(n);
    emit analysisFinished(labels, dateData, n,
                          minSol, maxSol, avg, oneSolCount, m_cancelled.load());
}
