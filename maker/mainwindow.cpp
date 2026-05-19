#include "mainwindow.h"
#include "searchworker.h"
#include "analysisworker.h"
#include "makerboardwidget.h"
#include "polyomino.h"
#include "maker_solver.h"
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QListWidget>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QScrollArea>
#include <QTabWidget>
#include <QFont>
#include <QDateTime>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

// ── Helpers ───────────────────────────────────────────────────────────────────

QString MainWindow::formatTime(double sec)
{
    int s = static_cast<int>(sec);
    return QString("%1:%2:%3")
        .arg(s / 3600,        2, 10, QChar('0'))
        .arg((s % 3600) / 60, 2, 10, QChar('0'))
        .arg(s % 60,          2, 10, QChar('0'));
}

QString MainWindow::formatLong(long long v)
{
    QString s = QString::number(v);
    for (int i = s.size() - 3; i > 0; i -= 3) s.insert(i, ',');
    return s;
}

// ── Constructor ───────────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setWindowTitle("Calendar Puzzle Maker");
    setMinimumSize(900, 680);

    // ════════════════════════════════════════════════════════════════════════
    //  LEFT PANEL – QTabWidget: "Board" and "Analysis" tabs
    // ════════════════════════════════════════════════════════════════════════
    m_leftTabs = new QTabWidget;

    // ── Tab 0: Board display ──────────────────────────────────────────────
    auto* boardTab    = new QWidget;
    auto* boardTabLay = new QVBoxLayout(boardTab);
    boardTabLay->setContentsMargins(4, 4, 4, 4);

    m_boardInfoLabel = new QLabel("← 結果を選択してください");
    m_boardInfoLabel->setAlignment(Qt::AlignCenter);
    m_boardInfoLabel->setWordWrap(true);

    m_saveJsonBtn = new QPushButton("JSONに保存…");
    m_saveJsonBtn->setEnabled(false);

    m_boardWidget = new MakerBoardWidget;

    auto* boardScroll = new QScrollArea;
    boardScroll->setWidget(m_boardWidget);
    boardScroll->setWidgetResizable(false);
    boardScroll->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    boardScroll->setFrameShape(QFrame::NoFrame);
    boardScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    boardTabLay->addWidget(m_boardInfoLabel);
    boardTabLay->addWidget(m_saveJsonBtn);
    boardTabLay->addWidget(boardScroll, 1);

    m_leftTabs->addTab(boardTab, "ボード表示");

    // ── Tab 1: Analysis ───────────────────────────────────────────────────
    auto* analysisTab    = new QWidget;
    auto* analysisTabLay = new QVBoxLayout(analysisTab);
    analysisTabLay->setContentsMargins(6, 6, 6, 6);

    // Controls row
    auto* ctrlRow = new QHBoxLayout;
    ctrlRow->addWidget(new QLabel("1日の最大解数:"));
    m_maxCountSpin = new QSpinBox;
    m_maxCountSpin->setRange(1, 500);
    m_maxCountSpin->setValue(50);
    ctrlRow->addWidget(m_maxCountSpin);
    m_analysisStartBtn = new QPushButton("▶ 分析開始");
    m_analysisStopBtn  = new QPushButton("■ 停止");
    m_analysisStopBtn->setEnabled(false);
    ctrlRow->addWidget(m_analysisStartBtn);
    ctrlRow->addWidget(m_analysisStopBtn);
    ctrlRow->addStretch();
    analysisTabLay->addLayout(ctrlRow);

    // Progress
    m_analysisProg = new QProgressBar;
    m_analysisProg->setRange(0, 10000);
    m_analysisProg->setValue(0);
    m_analysisProg->setFormat("%p%");
    analysisTabLay->addWidget(m_analysisProg);

    m_analysisProgLabel = new QLabel("0 / 0 日");
    m_analysisProgLabel->setAlignment(Qt::AlignRight);
    analysisTabLay->addWidget(m_analysisProgLabel);

    // Hardest dates list
    analysisTabLay->addWidget(new QLabel("解が少ない日付（難度の高い順）:"));

    m_hardDatesList = new QListWidget;
    QFont mono("Courier New"); mono.setPointSize(10);
    m_hardDatesList->setFont(mono);
    analysisTabLay->addWidget(m_hardDatesList, 1);

    // Stats
    m_statsLabel = new QLabel("―");
    m_statsLabel->setAlignment(Qt::AlignCenter);
    m_statsLabel->setWordWrap(true);
    analysisTabLay->addWidget(m_statsLabel);

    m_leftTabs->addTab(analysisTab, "分析");

    // ════════════════════════════════════════════════════════════════════════
    //  RIGHT PANEL
    // ════════════════════════════════════════════════════════════════════════

    // ── Settings ─────────────────────────────────────────────────────────────
    auto* grpSettings  = new QGroupBox("探索設定");
    auto* formSettings = new QFormLayout(grpSettings);

    auto* sizeRow = new QHBoxLayout;
    m_minSize = new QSpinBox; m_minSize->setRange(1,8); m_minSize->setValue(3);
    m_maxSize = new QSpinBox; m_maxSize->setRange(1,8); m_maxSize->setValue(5);
    sizeRow->addWidget(m_minSize); sizeRow->addWidget(new QLabel("〜"));
    sizeRow->addWidget(m_maxSize); sizeRow->addStretch();
    formSettings->addRow("ピースサイズ (マス数):", sizeRow);

    auto* pcsRow = new QHBoxLayout;
    m_minPieces = new QSpinBox; m_minPieces->setRange(1,20); m_minPieces->setValue(9);
    m_maxPieces = new QSpinBox; m_maxPieces->setRange(1,20); m_maxPieces->setValue(11);
    pcsRow->addWidget(m_minPieces); pcsRow->addWidget(new QLabel("〜"));
    pcsRow->addWidget(m_maxPieces); pcsRow->addStretch();
    formSettings->addRow("ピース枚数:", pcsRow);

    m_bothSides = new QCheckBox("両面使用（裏返し可）");
    m_bothSides->setChecked(true);
    formSettings->addRow("", m_bothSides);

    auto* btnRow = new QHBoxLayout;
    m_startBtn = new QPushButton("▶ 探索開始");
    m_pauseBtn = new QPushButton("⏸ 一時停止");
    m_stopBtn  = new QPushButton("■ 停止");
    m_pauseBtn->setEnabled(false); m_stopBtn->setEnabled(false);
    m_pauseBtn->setMinimumWidth(100);
    btnRow->addWidget(m_startBtn); btnRow->addWidget(m_pauseBtn);
    btnRow->addWidget(m_stopBtn);  btnRow->addStretch();
    formSettings->addRow("", btnRow);

    // ── Progress ──────────────────────────────────────────────────────────────
    auto* grpProgress = new QGroupBox("探索状況");
    auto* gridProg    = new QGridLayout(grpProgress);

    auto makeVal = [](const QString& init) {
        auto* l = new QLabel(init);
        l->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        return l;
    };
    gridProg->addWidget(new QLabel("分割パターン:"),    0, 0);
    m_lblPartition = makeVal("―"); gridProg->addWidget(m_lblPartition, 0, 1);
    gridProg->addWidget(new QLabel("現在のパターン:"),  1, 0);
    m_lblPattern   = makeVal("―"); gridProg->addWidget(m_lblPattern,   1, 1);
    gridProg->addWidget(new QLabel("組み合わせ:"),      2, 0);
    m_lblCombos    = makeVal("―"); gridProg->addWidget(m_lblCombos,    2, 1);
    gridProg->addWidget(new QLabel("発見済み:"),        3, 0);
    m_lblSolutions = makeVal("0 件"); gridProg->addWidget(m_lblSolutions, 3, 1);
    gridProg->addWidget(new QLabel("経過時間:"),        4, 0);
    m_lblElapsed   = makeVal("00:00:00"); gridProg->addWidget(m_lblElapsed, 4, 1);
    gridProg->setColumnStretch(1, 1);
    m_progressBar = new QProgressBar;
    m_progressBar->setRange(0, 10000); m_progressBar->setValue(0);
    m_progressBar->setFormat("%p%");
    gridProg->addWidget(m_progressBar, 5, 0, 1, 2);

    // ── Results ───────────────────────────────────────────────────────────────
    auto* grpResults = new QGroupBox("発見された組み合わせ（クリックでボード更新）");
    auto* vResults   = new QVBoxLayout(grpResults);
    m_resultList = new QListWidget;
    {
        QFont f("Courier New"); f.setPointSize(9);
        m_resultList->setFont(f);
    }
    vResults->addWidget(m_resultList);

    // Right splitter
    auto* topRight = new QWidget;
    {
        auto* lay = new QVBoxLayout(topRight);
        lay->setContentsMargins(0,0,0,0);
        lay->addWidget(grpSettings);
        lay->addWidget(grpProgress);
    }
    auto* rightSplitter = new QSplitter(Qt::Vertical);
    rightSplitter->addWidget(topRight);
    rightSplitter->addWidget(grpResults);
    rightSplitter->setStretchFactor(0, 0);
    rightSplitter->setStretchFactor(1, 1);

    auto* rightWidget = new QWidget;
    {
        auto* lay = new QVBoxLayout(rightWidget);
        lay->setContentsMargins(4, 8, 8, 8);
        lay->addWidget(rightSplitter);
    }

    // ════════════════════════════════════════════════════════════════════════
    //  OUTER SPLITTER
    // ════════════════════════════════════════════════════════════════════════
    auto* leftWidget = new QWidget;
    {
        auto* lay = new QVBoxLayout(leftWidget);
        lay->setContentsMargins(8, 8, 4, 8);
        lay->addWidget(m_leftTabs);
    }

    auto* outerSplitter = new QSplitter(Qt::Horizontal);
    outerSplitter->addWidget(leftWidget);
    outerSplitter->addWidget(rightWidget);
    outerSplitter->setStretchFactor(0, 0);
    outerSplitter->setStretchFactor(1, 1);
    outerSplitter->setSizes({m_boardWidget->sizeHint().width() + 48, 450});

    setCentralWidget(outerSplitter);

    // ── Connections ───────────────────────────────────────────────────────────
    connect(m_saveJsonBtn,      &QPushButton::clicked, this, &MainWindow::onSaveJson);
    connect(m_startBtn,         &QPushButton::clicked, this, &MainWindow::onStart);

    m_searchElapsedTimer = new QTimer(this);
    m_searchElapsedTimer->setInterval(1000);
    connect(m_searchElapsedTimer, &QTimer::timeout, this, [this]() {
        m_lblElapsed->setText(formatTime(m_searchElapsedClock.elapsed() / 1000.0));
    });
    connect(m_pauseBtn,         &QPushButton::clicked, this, &MainWindow::onPauseResume);
    connect(m_stopBtn,          &QPushButton::clicked, this, &MainWindow::onStop);
    connect(m_analysisStartBtn, &QPushButton::clicked, this, &MainWindow::onAnalysisStart);
    connect(m_analysisStopBtn,  &QPushButton::clicked, this, &MainWindow::onAnalysisStop);
    connect(m_resultList, &QListWidget::currentRowChanged,
            this, &MainWindow::onResultClicked);
    connect(m_hardDatesList, &QListWidget::currentRowChanged,
            this, &MainWindow::onHardDateClicked);

    connect(m_minSize,   QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int v){
        if (m_maxSize->value() < v) m_maxSize->setValue(v); });
    connect(m_minPieces, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int v){
        if (m_maxPieces->value() < v) m_maxPieces->setValue(v); });
}

MainWindow::~MainWindow()
{
    if (m_worker && m_worker->isRunning()) {
        m_worker->requestCancel(); m_worker->wait(3000);
    }
    if (m_analysisWorker && m_analysisWorker->isRunning()) {
        m_analysisWorker->requestCancel(); m_analysisWorker->wait(3000);
    }
}

// ── Search slots ──────────────────────────────────────────────────────────────

void MainWindow::onStart()
{
    if (m_worker && m_worker->isRunning()) return;
    m_resultList->clear(); m_solutions.clear();
    m_selectedRow = -1;
    m_boardWidget->clear();
    m_boardInfoLabel->setText("← 結果を選択してください");
    m_saveJsonBtn->setEnabled(false);
    m_hardDatesList->clear(); m_statsLabel->setText("―");
    m_isPaused = false;
    m_progressBar->setValue(0);
    m_lblPartition->setText("準備中…"); m_lblPattern->setText("―");
    m_lblCombos->setText("―");        m_lblSolutions->setText("0 件");
    m_lblElapsed->setText("00:00:00");

    m_worker = new SearchWorker(this);
    m_worker->minSize   = m_minSize->value();
    m_worker->maxSize   = m_maxSize->value();
    m_worker->minPieces = m_minPieces->value();
    m_worker->maxPieces = m_maxPieces->value();
    m_worker->bothSides = m_bothSides->isChecked();

    connect(m_worker, &SearchWorker::progressUpdated, this, &MainWindow::onProgress);
    connect(m_worker, &SearchWorker::solutionFound,   this, &MainWindow::onSolutionFound);
    connect(m_worker, &SearchWorker::searchFinished,  this, &MainWindow::onSearchFinished);
    connect(m_worker, &SearchWorker::finished,
            this, [this](){ m_worker = nullptr; });
    connect(m_worker, &SearchWorker::finished, m_worker, &QObject::deleteLater);

    setSearchRunning(true);
    m_searchElapsedClock.start();
    m_searchElapsedTimer->start();
    m_worker->start();
}

void MainWindow::onStop()
{
    if (m_worker) {
        if (m_isPaused) m_worker->requestResume();
        m_worker->requestCancel();
    }
}

void MainWindow::onPauseResume()
{
    if (!m_worker) return;
    if (!m_isPaused) {
        m_isPaused = true;
        m_worker->requestPause();
        m_pauseBtn->setText("▶ 再開");
    } else {
        m_isPaused = false;
        m_worker->requestResume();
        m_pauseBtn->setText("⏸ 一時停止");
    }
}

void MainWindow::onProgress(int partIdx, int totalParts,
                             long long globalCombo, long long globalTotal,
                             int solFound, double elapsedSec, QString partDesc)
{
    m_lblPartition->setText(QString("%1 / %2")
        .arg(partIdx+1).arg(totalParts > 0 ? totalParts : 1));
    m_lblPattern->setText(partDesc.isEmpty() ? "―" : partDesc);
    m_lblCombos->setText(QString("%1 / %2")
        .arg(formatLong(globalCombo)).arg(formatLong(globalTotal)));
    m_lblSolutions->setText(QString("%1 件").arg(solFound));
    m_lblElapsed->setText(formatTime(elapsedSec));
    if (globalTotal > 0)
        m_progressBar->setValue(int(
            std::min(10000LL, globalCombo * 10000LL / globalTotal)));
}

void MainWindow::onSolutionFound(QStringList shapes, QString desc,
                                  QVector<int> boardData, bool bothSides)
{
    SolutionData sol;
    sol.shapes    = shapes;
    sol.board     = boardData;
    sol.desc      = desc;
    sol.nPieces   = shapes.size();
    sol.foundTime = QDateTime::currentDateTime().toString("HH:mm:ss");
    sol.bothSides = bothSides;
    m_solutions.append(sol);

    int n = m_solutions.size();
    m_resultList->addItem(
        QString("#%1  %2  %3枚  %4")
            .arg(n, 2)
            .arg(desc.leftJustified(22))
            .arg(sol.nPieces, 2)
            .arg(sol.foundTime));
}

void MainWindow::onSearchFinished(int total, double elapsedSec, bool wasCancelled)
{
    m_searchElapsedTimer->stop();
    setSearchRunning(false);
    m_isPaused = false;
    if (!wasCancelled) m_progressBar->setValue(10000);
    m_lblElapsed->setText(formatTime(elapsedSec));
    m_lblSolutions->setText(QString("%1 件").arg(total));
    QMessageBox::information(this, "完了",
        total > 0
            ? QString("探索完了: %1 件の組み合わせを発見しました。").arg(total)
            : "探索完了: 有効な組み合わせは見つかりませんでした。");
}

void MainWindow::onResultClicked(int row)
{
    if (row < 0 || row >= m_solutions.size()) return;
    m_selectedRow = row;
    showBoardForSolution(row);
    m_hardDatesList->clear();
    m_statsLabel->setText("―");
    m_analysisProg->setValue(0);
    m_analysisProgLabel->setText("0 / 0 日");
}

void MainWindow::showBoardForSolution(int row)
{
    const SolutionData& sol = m_solutions[row];
    if (!sol.board.isEmpty())
        m_boardWidget->setBoard(sol.board, 1, 1, 1, sol.nPieces);
    else
        m_boardWidget->clear();

    m_boardInfoLabel->setText(
        QString("<b>#%1  %2</b><br>%3 枚構成 ／ 参照日: 1月1日(月)<br>"
                "<small>発見: %4</small>")
            .arg(row+1).arg(sol.desc).arg(sol.nPieces).arg(sol.foundTime));
    m_saveJsonBtn->setEnabled(true);
}

// ── Analysis slots ────────────────────────────────────────────────────────────

void MainWindow::onAnalysisStart()
{
    if (m_analysisWorker && m_analysisWorker->isRunning()) return;
    if (m_selectedRow < 0 || m_selectedRow >= m_solutions.size()) {
        QMessageBox::information(this, "分析",
            "先に結果リストから組み合わせを選択してください。");
        return;
    }

    const SolutionData& sol = m_solutions[m_selectedRow];
    std::vector<MakerPiece> pieces;
    pieces.reserve(sol.shapes.size());
    for (const QString& s : sol.shapes)
        pieces.push_back(makePiece(shapeFromString(s.toStdString()), sol.bothSides));

    m_hardDatesList->clear();
    m_statsLabel->setText("分析中…");
    m_analysisProg->setValue(0);
    m_analysisProgLabel->setText("0 / 0 日");

    m_analysisWorker = new AnalysisWorker(this);
    m_analysisWorker->pieces   = std::move(pieces);
    m_analysisWorker->maxCount = m_maxCountSpin->value();

    connect(m_analysisWorker, &AnalysisWorker::progressUpdated,
            this, &MainWindow::onAnalysisProgress);
    connect(m_analysisWorker, &AnalysisWorker::analysisFinished,
            this, &MainWindow::onAnalysisFinished);
    connect(m_analysisWorker, &AnalysisWorker::finished,
            this, [this](){ m_analysisWorker = nullptr; });
    connect(m_analysisWorker, &AnalysisWorker::finished,
            m_analysisWorker, &QObject::deleteLater);

    setAnalysisRunning(true);
    m_analysisWorker->start();
}

void MainWindow::onAnalysisStop()
{
    if (m_analysisWorker) m_analysisWorker->requestCancel();
}

void MainWindow::onAnalysisProgress(int checked, int total)
{
    m_analysisProgLabel->setText(
        QString("%1 / %2 日").arg(formatLong(checked)).arg(formatLong(total)));
    if (total > 0)
        m_analysisProg->setValue(int(
            std::min(10000LL, (long long)checked * 10000LL / total)));
}

void MainWindow::onAnalysisFinished(QStringList sortedEntries, QVector<int> dateData,
                                     int totalChecked, int minSol, int maxSol,
                                     double avgSol, int oneSolCount, bool wasCancelled)
{
    setAnalysisRunning(false);
    if (!wasCancelled) m_analysisProg->setValue(10000);

    m_hardDatesList->clear();
    for (int i = 0; i < sortedEntries.size(); ++i) {
        auto* item = new QListWidgetItem(sortedEntries[i]);
        if (i < dateData.size())
            item->setData(Qt::UserRole, dateData[i]);
        m_hardDatesList->addItem(item);
    }

    int maxCount = m_maxCountSpin->value();
    bool minCapped = (minSol == maxCount);  // all dates hit the cap
    bool anyCapped = (maxSol == maxCount);  // at least one date hit the cap
    double onePct  = totalChecked > 0
        ? double(oneSolCount) * 100.0 / totalChecked : 0.0;

    // Prefix with "≥" wherever capping makes the value an underestimate.
    // minCapped → min is "≥N"
    // anyCapped → max and avg are "≥N" (avg is underestimate when any value is capped)
    m_statsLabel->setText(
        QString("最小: %1%2解  最大: %3%4解  平均: %5%6解\n唯一解の日: %7日 (%8%)")
            .arg(minCapped ? "≥" : "").arg(minSol)
            .arg(anyCapped ? "≥" : "").arg(maxSol)
            .arg(anyCapped ? "≥" : "").arg(avgSol, 0, 'f', 1)
            .arg(oneSolCount)
            .arg(onePct, 0, 'f', 1));
}

void MainWindow::onSaveJson()
{
    if (m_selectedRow < 0 || m_selectedRow >= m_solutions.size()) return;
    const SolutionData& sol = m_solutions[m_selectedRow];

    // Build JSON
    QJsonObject root;
    root["description"] = sol.desc;
    root["foundAt"]     = QDateTime::currentDateTime().toString(Qt::ISODate);
    root["bothSides"]   = sol.bothSides;

    QJsonArray piecesArr;
    for (int i = 0; i < sol.shapes.size(); ++i) {
        // Parse ASCII art back to canonical cell list
        Shape cells = shapeFromString(sol.shapes[i].toStdString());

        // Convert sorted cell list to vector chain (diff between consecutive cells)
        QJsonArray vecs;
        for (int j = 1; j < static_cast<int>(cells.size()); ++j) {
            QJsonArray v;
            v.append(cells[j].first  - cells[j-1].first);
            v.append(cells[j].second - cells[j-1].second);
            vecs.append(v);
        }

        QJsonObject piece;
        piece["name"]    = QString("P%1").arg(i + 1, 2, 10, QChar('0'));
        piece["vectors"] = vecs;
        piecesArr.append(piece);
    }
    root["pieces"] = piecesArr;

    // Save dialog
    QString defaultName = sol.desc;
    defaultName.remove(' ');    // remove spaces for a cleaner filename
    QString path = QFileDialog::getSaveFileName(
        this, "JSONに保存", defaultName + ".json", "JSON Files (*.json)");
    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "保存エラー", f.errorString());
        return;
    }
    f.write(QJsonDocument(root).toJson());
}

void MainWindow::onHardDateClicked(int row)
{
    if (m_selectedRow < 0 || m_selectedRow >= m_solutions.size()) return;
    auto* item = m_hardDatesList->item(row);
    if (!item) return;

    int encoded = item->data(Qt::UserRole).toInt();
    if (encoded == 0) return;
    int month   = encoded / 10000;
    int day     = (encoded / 100) % 100;
    int weekday = encoded % 100;

    const SolutionData& sol = m_solutions[m_selectedRow];

    // Rebuild pieces and solve for the clicked date (guaranteed solvable, fast)
    std::vector<MakerPiece> pieces;
    pieces.reserve(sol.shapes.size());
    for (const QString& s : sol.shapes)
        pieces.push_back(makePiece(shapeFromString(s.toStdString()), sol.bothSides));

    std::atomic<bool> cancelled{false};
    std::vector<int> board;
    quickSolve(month, day, weekday, pieces, cancelled, &board);
    if (board.empty()) return;

    static const char* kWd[] = {"","月","火","水","木","金","土","日"};

    QVector<int> qboard(board.begin(), board.end());
    m_boardWidget->setBoard(qboard, month, day, weekday, sol.nPieces);
    m_boardInfoLabel->setText(
        QString("<b>#%1  %2</b><br>%3月%4日 (%5)<br><small>発見: %6</small>")
            .arg(m_selectedRow + 1).arg(sol.desc)
            .arg(month).arg(day).arg(kWd[weekday])
            .arg(sol.foundTime));

    m_leftTabs->setCurrentIndex(0); // Switch to board tab
}

// ── UI state ──────────────────────────────────────────────────────────────────

void MainWindow::setSearchRunning(bool running)
{
    m_startBtn->setEnabled(!running);
    m_pauseBtn->setEnabled(running);
    m_stopBtn->setEnabled(running);
    m_minSize->setEnabled(!running);   m_maxSize->setEnabled(!running);
    m_minPieces->setEnabled(!running); m_maxPieces->setEnabled(!running);
    m_bothSides->setEnabled(!running);
    if (!running) m_pauseBtn->setText("⏸ 一時停止");
}

void MainWindow::setAnalysisRunning(bool running)
{
    m_analysisStartBtn->setEnabled(!running);
    m_analysisStopBtn->setEnabled(running);
    m_maxCountSpin->setEnabled(!running);
}
