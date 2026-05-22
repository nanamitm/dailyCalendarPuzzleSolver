#include "SolverBackend.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

#if defined(Q_OS_ANDROID)
#include <QJniObject>
#include <QtCore/qcoreapplication_platform.h>
#endif

// ── Label helpers ──────────────────────────────────────────────────────────

static const char* MONTH_ABBR[] = {
    "Jan","Feb","Mar","Apr","May","Jun",
    "Jul","Aug","Sep","Oct","Nov","Dec"
};
static const char* DAY_ABBR[] = {
    "Mon","Tue","Wed","Thu","Fri","Sat","Sun"
};

static QMap<QPair<int,int>, QString> makeDateMap(QDate d)
{
    QMap<QPair<int,int>, QString> m;
    if (!d.isValid()) return m;

    int mo = d.month() - 1;
    int dy = d.day()   - 1;
    int wd = d.dayOfWeek() - 1;  // 0=Mon … 6=Sun

    m[{(mo/6)+BOY,   (mo%6)+BOX}]  = MONTH_ABBR[mo];
    m[{(dy/7)+BOY+2, (dy%7)+BOX}]  = QString::number(d.day()).rightJustified(2, '0');

    if (wd == 6)
        m[{9, 6}] = DAY_ABBR[6];
    else
        m[{(wd/3)+9, (wd%3)+7}] = DAY_ABBR[wd];

    return m;
}

static const QMap<QPair<int,int>, QString>& allLabelsMap()
{
    static const QMap<QPair<int,int>, QString> s = []() {
        QMap<QPair<int,int>, QString> m;
        for (int i = 0; i < 12; ++i)
            m[{(i/6)+BOY, (i%6)+BOX}] = MONTH_ABBR[i];
        for (int i = 0; i < 31; ++i)
            m[{(i/7)+BOY+2, (i%7)+BOX}] = QString::number(i+1).rightJustified(2, '0');
        m[{9, 6}] = "Sun";
        static const char* WDAYS[] = {"Mon","Tue","Wed","Thu","Fri","Sat"};
        for (int i = 0; i < 6; ++i)
            m[{(i/3)+9, (i%3)+7}] = WDAYS[i];
        return m;
    }();
    return s;
}

// ── SolverBackend ──────────────────────────────────────────────────────────

SolverBackend::SolverBackend(QObject* parent) : QObject(parent)
{
    m_boardData.reserve(BDYL * BDXL);
    m_boardLabels.reserve(BDYL * BDXL);

    m_slideshowTimer = new QTimer(this);
    m_slideshowTimer->setInterval(2000);  // 2 seconds
    connect(m_slideshowTimer, &QTimer::timeout, this, &SolverBackend::onSlideshowTick);

    scanPieceSets();
    updateBoardData(nullptr, QDate::currentDate());
}

// ── Slideshow ──────────────────────────────────────────────────────────────

void SolverBackend::setSlideshow(bool on)
{
    if (m_slideshow == on) return;
    m_slideshow = on;
    emit slideshowChanged();

    if (on) {
        if ((int)m_solutions.size() > 1) {
            initLcg();
            m_slideshowTimer->start();
        }
        // If solutions aren't loaded yet, the timer starts in onSolved()
    } else {
        m_slideshowTimer->stop();
    }
}

void SolverBackend::initLcg()
{
    // m = smallest power-of-2 >= solution count → full-period Hull-Dobell LCG
    quint32 m = 1;
    while (m < (quint32)m_solutions.size()) m <<= 1;
    m_lcgM     = m;
    m_lcgState = (quint32)m_idx;
}

int SolverBackend::nextLcgIdx()
{
    int n = (int)m_solutions.size();
    do {
        m_lcgState = (5 * m_lcgState + 1) % m_lcgM;
    } while ((int)m_lcgState >= n);
    return (int)m_lcgState;
}

void SolverBackend::onSlideshowTick()
{
    if (m_solutions.empty()) return;
    m_idx = nextLcgIdx();
    showSolution(m_idx);
    m_solLabel = QString("Solution %1 / %2").arg(m_idx + 1).arg(m_solutions.size());
    emit boardChanged();
}

// ── Piece sets ─────────────────────────────────────────────────────────────

QString SolverBackend::piecesFolder() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/pieces";
}

void SolverBackend::scanPieceSets()
{
    QDir dir(piecesFolder());
    dir.mkpath(".");

    m_loadedSets.clear();
    m_pieceSetNames = {"デフォルト (元の10ピース)"};

    const auto files = dir.entryInfoList({"*.json"}, QDir::Files, QDir::Name);
    for (const auto& fi : files) {
        LoadedPieceSet set;
        QString err;
        if (loadPieceSetFromJson(fi.absoluteFilePath(), set, err)) {
            m_loadedSets.push_back(std::move(set));
            QFileInfo f(m_loadedSets.back().filePath);
            QString label = m_loadedSets.back().description.isEmpty()
                            ? f.baseName()
                            : m_loadedSets.back().description;
            m_pieceSetNames.append(label);
        }
    }
    emit pieceSetNamesChanged();
}

void SolverBackend::refreshPieceSets()
{
    int prev = m_currentPieceSet;
    scanPieceSets();
    m_currentPieceSet = qBound(0, prev, (int)m_pieceSetNames.size() - 1);
    emit pieceSetNamesChanged();
}

void SolverBackend::setCurrentPieceSet(int idx)
{
    if (m_currentPieceSet == idx) return;
    m_currentPieceSet = idx;
    emit pieceSetNamesChanged();
}

// ── Solve ──────────────────────────────────────────────────────────────────

void SolverBackend::solve(int year, int month, int day, bool findAll)
{
    m_pendingDate    = QDate(year, month, day);
    // Slideshow always requires findAll
    m_pendingFindAll = findAll || m_slideshow;
    m_hasPending     = true;

    if (m_worker && m_worker->isRunning()) {
        m_worker->requestCancel();
        return;
    }
    doSolve();
}

void SolverBackend::doSolve()
{
    if (!m_hasPending) return;
    m_hasPending = false;

    QDate date   = m_pendingDate;
    bool findAll = m_pendingFindAll;

    m_slideshowTimer->stop();
    m_solutions.clear();
    m_idx = 0;
    updateBoardData(nullptr, date);
    m_solLabel = "Solving…";
    emit boardChanged();

    if (m_worker) {
        m_worker->wait();   // ensure thread has fully exited before delete
        delete m_worker;
    }
    m_worker = new SolverWorker(this);
    m_worker->date    = date;
    m_worker->findAll = findAll;

    if (m_currentPieceSet > 0 &&
        m_currentPieceSet - 1 < (int)m_loadedSets.size())
    {
        m_worker->useCustomPieces = true;
        m_worker->customPieceSet  = m_loadedSets[m_currentPieceSet - 1];
    } else {
        m_worker->useCustomPieces = false;
    }

    m_solving = true;
    emit solvingChanged();

    connect(m_worker, &SolverWorker::solved, this, &SolverBackend::onSolved);
    m_worker->start();
}

void SolverBackend::cancel()
{
    if (m_worker && m_worker->isRunning())
        m_worker->requestCancel();
}

void SolverBackend::onSolved()
{
    m_solving = false;
    emit solvingChanged();

    const SolverOutput& out = m_worker->result;
    m_solvedDate            = m_worker->date;

    m_solutions.assign(out.solutions.begin(), out.solutions.end());
    m_idx = 0;

    if (!m_solutions.empty()) {
        showSolution(0);
        m_solLabel = QString("Solution 1 / %1").arg(m_solutions.size());
        // (Re)start slideshow timer if enabled and there are multiple solutions
        if (m_slideshow && (int)m_solutions.size() > 1) {
            initLcg();
            m_slideshowTimer->start();
        }
    } else {
        updateBoardData(nullptr, m_solvedDate);
        m_solLabel = "No solution found";
    }

    m_statusText = QString("%1 解  ·  %2 ms")
        .arg(m_solutions.size())
        .arg(out.elapsedMs, 0, 'f', 1);

    emit boardChanged();
    emit statusTextChanged();

    if (m_hasPending) doSolve();
}

void SolverBackend::prevSolution()
{
    if (m_solutions.empty()) return;
    m_idx = (m_idx - 1 + (int)m_solutions.size()) % (int)m_solutions.size();
    showSolution(m_idx);
    m_solLabel = QString("Solution %1 / %2").arg(m_idx + 1).arg(m_solutions.size());
    m_lcgState = (quint32)m_idx;
    if (m_slideshowTimer->isActive()) m_slideshowTimer->start(); // reset interval
    emit boardChanged();
}

void SolverBackend::nextSolution()
{
    if (m_solutions.empty()) return;
    m_idx = (m_idx + 1) % (int)m_solutions.size();
    showSolution(m_idx);
    m_solLabel = QString("Solution %1 / %2").arg(m_idx + 1).arg(m_solutions.size());
    m_lcgState = (quint32)m_idx;
    if (m_slideshowTimer->isActive()) m_slideshowTimer->start(); // reset interval
    emit boardChanged();
}

void SolverBackend::showSolution(int idx)
{
    updateBoardData(&m_solutions[idx], m_solvedDate);
}

// ── Board data computation ─────────────────────────────────────────────────

void SolverBackend::updateBoardData(const Board* board, QDate date)
{
    auto dateMap = makeDateMap(date);
    const auto& labelMap = (board == nullptr) ? allLabelsMap() : dateMap;

    m_boardData.clear();
    m_boardLabels.clear();

    for (int r = 0; r < BDYL; ++r) {
        int row = r + BOY;
        for (int c = 0; c < BDXL; ++c) {
            int col = c + BOX;

            int group;
            if (board) {
                int v = board->cells[row][col];
                if (v == -1)
                    group = dateMap.contains({row, col}) ? -1 : -2;
                else
                    group = v;
            } else if (date.isValid()) {
                bool corner = ((row == 3 || row == 4) && col == 9) ||
                              (row == 10 && col >= 3 && col <= 6);
                if (corner)
                    group = dateMap.contains({row, col}) ? -1 : -2;
                else
                    group = 0;
            } else {
                group = 0;
            }

            m_boardData.append(group);

            auto key = QPair<int,int>(row, col);
            m_boardLabels.append(labelMap.contains(key) ? labelMap[key] : QString());
        }
    }
}

// ── Piece set import ───────────────────────────────────────────────────────

void SolverBackend::importPieceSet(const QUrl& fileUrl)
{
    if (!fileUrl.isValid()) return;

    // On Android fileUrl may be a content:// URI — QFile handles both
    QString sourcePath = fileUrl.isLocalFile()
                       ? fileUrl.toLocalFile()
                       : fileUrl.toString();

    QFile src(sourcePath);
    if (!src.open(QIODevice::ReadOnly)) {
        emit importFailed("ファイルを開けません: " + src.errorString());
        return;
    }
    QByteArray data = src.readAll();
    src.close();

    // Determine filename
    QString fileName = fileUrl.fileName();
    if (fileName.isEmpty()) fileName = "imported.json";
    if (!fileName.endsWith(".json", Qt::CaseInsensitive)) fileName += ".json";

    // Write to pieces folder (overwrite if same name)
    QString destDir = piecesFolder();
    QDir().mkpath(destDir);
    QString destPath = destDir + "/" + fileName;

    QFile dst(destPath);
    if (!dst.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        emit importFailed("保存できません: " + dst.errorString());
        return;
    }
    dst.write(data);
    dst.close();

    // Validate JSON
    LoadedPieceSet ps;
    QString error;
    if (!loadPieceSetFromJson(destPath, ps, error)) {
        QFile::remove(destPath);
        emit importFailed("不正な形式: " + error);
        return;
    }

    refreshPieceSets();

    // Auto-select the imported set
    int newIdx = m_pieceSetNames.indexOf(ps.description.isEmpty() ? fileName : ps.description);
    if (newIdx > 0) m_currentPieceSet = newIdx;

    emit importSucceeded(ps.description.isEmpty() ? fileName : ps.description);
}

// ── Incoming intent (Android VIEW / SEND) ─────────────────────────────────

void SolverBackend::checkIncomingIntent()
{
#if defined(Q_OS_ANDROID)
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([this]() {
        QJniObject activity = QNativeInterface::QAndroidApplication::context();
        if (!activity.isValid()) return;

        QJniObject intent = activity.callObjectMethod(
            "getIntent", "()Landroid/content/Intent;");
        if (!intent.isValid()) return;

        QJniObject action = intent.callObjectMethod(
            "getAction", "()Ljava/lang/String;");
        if (!action.isValid()) return;

        const QString actionStr = action.toString();
        QJniObject uri;

        if (actionStr == QLatin1String("android.intent.action.VIEW")) {
            uri = intent.callObjectMethod("getData", "()Landroid/net/Uri;");
        } else if (actionStr == QLatin1String("android.intent.action.SEND")) {
            QJniObject key = QJniObject::fromString("android.intent.extra.STREAM");
            uri = intent.callObjectMethod(
                "getParcelableExtra",
                "(Ljava/lang/String;)Landroid/os/Parcelable;",
                key.object<jstring>());
        }

        if (!uri.isValid()) return;

        const QString uriStr = uri.callObjectMethod<jstring>("toString").toString();
        if (!uriStr.endsWith(".json", Qt::CaseInsensitive) &&
            !uriStr.contains("json", Qt::CaseInsensitive)) return;

        QMetaObject::invokeMethod(this, [this, uriStr]() {
            importPieceSet(QUrl(uriStr));
            // Consume the intent so it isn't processed again on restart
        }, Qt::QueuedConnection);
    });
#endif
}
