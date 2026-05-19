#include "searchworker.h"
#include "maker_solver.h"
#include <functional>
#include <chrono>
#ifdef Q_OS_WIN
#  include <windows.h>
#elif defined(Q_OS_LINUX)
#  include <unistd.h>
#endif

// ── Helper ────────────────────────────────────────────────────────────────────

static long long binomial(int n, int k)
{
    if (k < 0 || k > n) return 0LL;
    if (k == 0 || k == n) return 1LL;
    k = std::min(k, n - k);
    long long r = 1;
    for (int i = 0; i < k; ++i) r = r * (n - i) / (i + 1);
    return r;
}

// ── Pause / progress ──────────────────────────────────────────────────────────

void SearchWorker::checkPause()
{
    if (!m_paused.load(std::memory_order_relaxed)) return;
    m_pauseMutex.lock();
    while (m_paused.load(std::memory_order_relaxed) &&
           !m_cancelled.load(std::memory_order_relaxed))
        m_pauseCond.wait(&m_pauseMutex);
    m_pauseMutex.unlock();
}

void SearchWorker::emitProgress()
{
    using clock = std::chrono::steady_clock;
    double elapsed = std::chrono::duration<double>(clock::now() - m_t0).count();
    emit progressUpdated(m_partIdx, m_totalParts,
                         m_globalCombo, m_globalTotal,
                         m_solFound, elapsed, m_partDesc);
}

// ── Recursive combination enumeration ────────────────────────────────────────

void SearchWorker::enumerate(
    const std::vector<std::vector<Shape>>& polyGroups,
    const std::vector<int>& counts,
    int groupIdx,
    int polyStart,
    int chosenInGroup,
    std::vector<Shape>& current)
{
    if (m_cancelled.load(std::memory_order_relaxed)) return;
    checkPause();

    int needed = counts[groupIdx] - chosenInGroup;

    if (needed == 0) {
        int nextGroup = groupIdx + 1;
        if (nextGroup == static_cast<int>(polyGroups.size())) {
            // All size groups filled → test this combination
            ++m_globalCombo;

            std::vector<MakerPiece> pieces;
            pieces.reserve(current.size());
            for (const auto& s : current)
                pieces.push_back(makePiece(s, m_bothSides));

            if (testAllDates(pieces, m_cancelled)) {
                ++m_solFound;

                // Solve one reference date (Jan 1, Monday) to get a board image
                std::vector<int> refBoard;
                quickSolve(1, 1, 1, pieces, m_cancelled, &refBoard);
                QVector<int> boardData(refBoard.begin(), refBoard.end());

                QStringList shapes;
                for (const auto& s : current)
                    shapes << QString::fromStdString(shapeToString(s));
                emit solutionFound(shapes, m_partDesc, boardData, m_bothSides);
            }

            if (m_globalCombo % 50 == 0) emitProgress();
        } else {
            enumerate(polyGroups, counts, nextGroup, 0, 0, current);
        }
        return;
    }

    const auto& pool = polyGroups[groupIdx];
    int poolSize = static_cast<int>(pool.size());

    for (int i = polyStart; i <= poolSize - needed; ++i) {
        if (m_cancelled.load(std::memory_order_relaxed)) return;
        checkPause();
        current.push_back(pool[i]);
        enumerate(polyGroups, counts, groupIdx, i + 1, chosenInGroup + 1, current);
        current.pop_back();
    }
}

// ── Main search loop ──────────────────────────────────────────────────────────

void SearchWorker::run()
{
#ifdef Q_OS_WIN
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
#elif defined(Q_OS_LINUX)
    nice(10);
#endif
    using clock = std::chrono::steady_clock;
    m_t0          = clock::now();
    m_cancelled.store(false, std::memory_order_relaxed);
    m_paused.store(false,    std::memory_order_relaxed);
    m_solFound    = 0;
    m_globalCombo = 0;
    m_globalTotal = 0;
    m_partIdx     = 0;
    m_bothSides   = bothSides;

    // ── 1. Generate polyominoes for each size in user range ───────────────────
    std::vector<int>                allSizes;
    std::vector<std::vector<Shape>> allPolys;
    for (int s = minSize; s <= maxSize; ++s) {
        auto polys = generatePolyominoes(s);
        if (!polys.empty()) {
            allSizes.push_back(s);
            allPolys.push_back(std::move(polys));
        }
    }

    int nSizes = static_cast<int>(allSizes.size());

    // ── 2. Enumerate all valid partitions of 47 ───────────────────────────────
    constexpr int TARGET = 47;

    struct Partition {
        std::vector<int> counts;
        long long combos;
        QString desc;
    };

    std::vector<Partition> partitions;
    std::vector<int> cc(nSizes, 0);

    std::function<void(int, int, int)> findParts = [&](int idx, int remArea, int totalPcs) {
        if (idx == nSizes) {
            if (remArea == 0 && totalPcs >= minPieces && totalPcs <= maxPieces) {
                Partition p;
                p.counts = cc;
                p.combos = 1;
                bool first = true;
                for (int i = 0; i < nSizes; ++i) {
                    if (cc[i] == 0) continue;
                    p.combos *= binomial(static_cast<int>(allPolys[i].size()), cc[i]);
                    if (!first) p.desc += " + ";
                    p.desc += QString::number(cc[i]) + "\xC3\x97" + QString::number(allSizes[i]);
                    first = false;
                }
                partitions.push_back(p);
            }
            return;
        }
        int s = allSizes[idx];
        int maxC = std::min({static_cast<int>(allPolys[idx].size()),
                             remArea / s,
                             maxPieces - totalPcs});
        for (int c = 0; c <= maxC; ++c) {
            cc[idx] = c;
            findParts(idx + 1, remArea - c * s, totalPcs + c);
        }
        cc[idx] = 0;
    };

    findParts(0, TARGET, 0);

    m_totalParts = static_cast<int>(partitions.size());
    for (auto& p : partitions) m_globalTotal += p.combos;

    emitProgress();

    // ── 3. For each partition, enumerate piece combinations ───────────────────
    for (int pi = 0; pi < m_totalParts; ++pi) {
        if (m_cancelled.load()) break;

        auto& part = partitions[pi];
        m_partIdx  = pi;
        m_partDesc = part.desc;
        emitProgress();

        std::vector<std::vector<Shape>> polyGroups;
        std::vector<int>                counts;
        for (int i = 0; i < nSizes; ++i) {
            if (part.counts[i] > 0) {
                polyGroups.push_back(allPolys[i]);
                counts.push_back(part.counts[i]);
            }
        }

        std::vector<Shape> current;
        current.reserve(maxPieces);
        enumerate(polyGroups, counts, 0, 0, 0, current);
    }

    // ── 4. Final signal ───────────────────────────────────────────────────────
    m_partIdx = m_totalParts;
    emitProgress();

    double elapsed = std::chrono::duration<double>(clock::now() - m_t0).count();
    emit searchFinished(m_solFound, elapsed, m_cancelled.load());
}
