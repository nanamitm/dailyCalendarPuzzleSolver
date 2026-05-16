#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QSettings>

// ── Solving overlay ────────────────────────────────────────────────────────
class SolveOverlay : public QWidget {
    Q_OBJECT
public:
    explicit SolveOverlay(QWidget* parent) : QWidget(parent)
    {
        setAttribute(Qt::WA_NoSystemBackground);
        setGeometry(parent->rect());

        // Time label
        m_timeLabel = new QLabel("0.0 s", this);
        QFont f = m_timeLabel->font();
        f.setPointSize(22); f.setBold(true);
        m_timeLabel->setFont(f);
        m_timeLabel->setAlignment(Qt::AlignCenter);
        m_timeLabel->setStyleSheet("color: white; background: transparent;");

        // Cancel button
        m_cancelBtn = new QPushButton("Cancel", this);
        m_cancelBtn->setStyleSheet(
            "QPushButton { background:#e74c3c; color:white; border:none;"
            "  border-radius:6px; padding:6px 22px; font-size:13px; }"
            "QPushButton:hover   { background:#c0392b; }"
            "QPushButton:pressed { background:#a93226; }");
        connect(m_cancelBtn, &QPushButton::clicked,
                this,        &SolveOverlay::cancelClicked);

        layoutWidgets();
        hide();
    }

    void updateTime(qint64 ms) {
        m_timeLabel->setText(QString("%1 s").arg(ms / 1000.0, 0, 'f', 1));
    }

signals:
    void cancelClicked();

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.fillRect(rect(), QColor(0, 0, 0, 110));   // dim overlay
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(28, 28, 28, 230));         // dark panel
        QPainterPath path;
        path.addRoundedRect(panelRect(), 14, 14);
        p.drawPath(path);
    }

    void resizeEvent(QResizeEvent*) override { layoutWidgets(); }

private:
    QLabel*      m_timeLabel;
    QPushButton* m_cancelBtn;

    QRect panelRect() const {
        return QRect(0, 0, 180, 120)
            .translated((width() - 180) / 2, (height() - 120) / 2);
    }

    void layoutWidgets() {
        QRect pr = panelRect();
        m_timeLabel->setGeometry(pr.left(), pr.top() + 14,
                                  pr.width(), 50);
        m_cancelBtn->adjustSize();
        int bw = qMax(m_cancelBtn->sizeHint().width(), 90);
        int bh = m_cancelBtn->sizeHint().height();
        m_cancelBtn->setGeometry(pr.center().x() - bw / 2,
                                  pr.bottom() - bh - 12, bw, bh);
    }
};

// ── MainWindow ─────────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setWindowTitle("Daily Calendar Puzzle Solver");
    buildUi();
}

MainWindow::~MainWindow()
{
    QSettings s;
    s.setValue("findAll",     m_findAllChk->isChecked());
    s.setValue("autoMidnight", m_autoChk->isChecked());

    if (m_worker) { m_worker->quit(); m_worker->wait(); }
}

void MainWindow::buildUi()
{
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* vbox = new QVBoxLayout(central);
    vbox->setSpacing(8);
    vbox->setContentsMargins(12, 12, 12, 12);

    // ── Date + Find all ───────────────────────────────────────────────────
    auto* topRow = new QHBoxLayout;
    topRow->addWidget(new QLabel("Date:", this));

    auto* prevDay = new QPushButton("◀", this);
    prevDay->setFixedWidth(28);
    topRow->addWidget(prevDay);

    m_dateEdit = new QDateEdit(QDate::currentDate(), this);
    m_dateEdit->setCalendarPopup(true);
    // Follow the system locale date format; ensure 4-digit year
    QString dateFmt = QLocale::system().dateFormat(QLocale::ShortFormat);
    if (!dateFmt.contains("yyyy"))
        dateFmt.replace("yy", "yyyy");
    m_dateEdit->setDisplayFormat(dateFmt);
    topRow->addWidget(m_dateEdit);

    auto* nextDay = new QPushButton("▶", this);
    nextDay->setFixedWidth(28);
    topRow->addWidget(nextDay);

    connect(prevDay, &QPushButton::clicked, this, [this]() {
        m_dateEdit->setDate(m_dateEdit->date().addDays(-1));
    });
    connect(nextDay, &QPushButton::clicked, this, [this]() {
        m_dateEdit->setDate(m_dateEdit->date().addDays(1));
    });

    m_findAllChk = new QCheckBox("Find all solutions", this);
    topRow->addWidget(m_findAllChk);

    m_autoChk = new QCheckBox("Auto-update at midnight", this);
    topRow->addWidget(m_autoChk);
    topRow->addStretch();

    // ── Restore saved checkbox states ─────────────────────────────────────
    QSettings s;
    m_findAllChk->setChecked(s.value("findAll", false).toBool());
    m_autoChk->setChecked(s.value("autoMidnight", false).toBool());
    vbox->addLayout(topRow);

    // ── Board ─────────────────────────────────────────────────────────────
    m_board = new BoardWidget(this);
    m_board->setDate(QDate::currentDate());
    vbox->addWidget(m_board, 0, Qt::AlignHCenter);

    // ── Navigation ────────────────────────────────────────────────────────
    auto* navRow = new QHBoxLayout;
    m_prevBtn = new QPushButton("◀", this);  m_prevBtn->setFixedWidth(40);
    m_nextBtn = new QPushButton("▶", this);  m_nextBtn->setFixedWidth(40);
    m_solLabel = new QLabel("", this);
    m_solLabel->setAlignment(Qt::AlignCenter);
    m_prevBtn->setEnabled(false);
    m_nextBtn->setEnabled(false);
    navRow->addWidget(m_prevBtn);
    navRow->addWidget(m_solLabel, 1);
    navRow->addWidget(m_nextBtn);
    vbox->addLayout(navRow);

    // ── Status ────────────────────────────────────────────────────────────
    m_statusLbl = new QLabel("", this);
    m_statusLbl->setAlignment(Qt::AlignCenter);
    vbox->addWidget(m_statusLbl);

    // ── Solving overlay (child of board widget) ───────────────────────────
    auto* overlay = new SolveOverlay(m_board);
    m_overlay = overlay;
    connect(overlay, &SolveOverlay::cancelClicked, this, [this]() {
        if (m_worker && m_worker->isRunning())
            m_worker->requestCancel();
    });

    // ── Tick timer: updates overlay time every 100 ms ─────────────────────
    m_tickTimer = new QTimer(this);
    m_tickTimer->setInterval(100);
    connect(m_tickTimer, &QTimer::timeout, this, [this, overlay]() {
        overlay->updateTime(m_elapsed.elapsed());
    });

    // ── Midnight auto-update timer ────────────────────────────────────────
    m_midnight = new QTimer(this);
    m_midnight->setSingleShot(true);
    connect(m_midnight, &QTimer::timeout, this, &MainWindow::onMidnight);
    scheduleMidnight();

    // ── Debounce timer ────────────────────────────────────────────────────
    m_debounce = new QTimer(this);
    m_debounce->setSingleShot(true);
    m_debounce->setInterval(300);
    connect(m_debounce, &QTimer::timeout, this, &MainWindow::onTriggerSolve);

    // ── Connections ───────────────────────────────────────────────────────
    connect(m_prevBtn,    &QPushButton::clicked,        this, &MainWindow::onPrev);
    connect(m_nextBtn,    &QPushButton::clicked,        this, &MainWindow::onNext);
    connect(m_dateEdit,   &QDateEdit::dateChanged,      this, [this](QDate d) {
        m_board->setDate(d);
        scheduleSolve();
    });
    connect(m_findAllChk, &QCheckBox::toggled, this, [this](bool) {
        scheduleSolve();
    });

    adjustSize();
    setFixedSize(sizeHint());

    // Initial solve on startup
    scheduleSolve();
}

void MainWindow::scheduleSolve()
{
    m_debounce->start();   // restarts the timer if already running
}

void MainWindow::scheduleMidnight()
{
    // Calculate milliseconds until 1 second past the next midnight
    QDateTime now  = QDateTime::currentDateTime();
    QDateTime next = QDateTime(now.date().addDays(1), QTime(0, 0, 1));
    m_midnight->start(static_cast<int>(now.msecsTo(next)));
}

void MainWindow::onMidnight()
{
    if (m_autoChk->isChecked())
        m_dateEdit->setDate(QDate::currentDate()); // triggers dateChanged → scheduleSolve
    scheduleMidnight(); // arm for the following midnight
}

void MainWindow::onTriggerSolve()
{
    if (m_worker && m_worker->isRunning()) {
        // Worker is busy — reschedule for after it finishes
        connect(m_worker, &SolverWorker::solved, this, [this]() {
            scheduleSolve();
        }, Qt::SingleShotConnection);
        return;
    }

    delete m_worker;
    m_worker = new SolverWorker(this);
    m_worker->date    = m_dateEdit->date();
    m_worker->findAll = m_findAllChk->isChecked();

    m_solutions.clear();
    m_idx = 0;
    m_prevBtn->setEnabled(false);
    m_nextBtn->setEnabled(false);
    m_solLabel->setText("Solving…");
    m_statusLbl->clear();

    // Show overlay and start tick timer
    m_elapsed.start();
    m_overlay->updateTime(0);
    m_overlay->setGeometry(m_board->rect());
    m_overlay->show();
    m_overlay->raise();
    m_tickTimer->start();

    connect(m_worker, &SolverWorker::solved, this, &MainWindow::onSolved);
    m_worker->start();
}

void MainWindow::onSolved()
{
    m_tickTimer->stop();
    m_overlay->hide();

    const SolverOutput& out = m_worker->result;
    const QDate date = m_worker->date;

    for (const Board& b : out.solutions)
        m_solutions.append(b);

    m_idx = 0;

    if (!m_solutions.isEmpty()) {
        showSolution(0);
        bool multi = m_solutions.size() > 1;
        m_prevBtn->setEnabled(multi);
        m_nextBtn->setEnabled(multi);
        m_solLabel->setText(QString("Solution 1 / %1").arg(m_solutions.size()));
    } else {
        m_solLabel->setText("No solution found");
        m_board->setDate(date);
    }

    m_statusLbl->setText(
        QString("%1 solution(s)  ·  %2 ms  ·  %3 tries  ·  %4 pieces placed")
            .arg(m_solutions.size())
            .arg(out.elapsedMs, 0, 'f', 1)
            .arg(out.tries)
            .arg(out.pcsPlaced));
}

void MainWindow::showSolution(int idx)
{
    m_board->setBoard(&m_solutions[idx], m_worker->date);
}

void MainWindow::onPrev()
{
    m_idx = (m_idx - 1 + m_solutions.size()) % m_solutions.size();
    showSolution(m_idx);
    m_solLabel->setText(
        QString("Solution %1 / %2").arg(m_idx+1).arg(m_solutions.size()));
}

void MainWindow::onNext()
{
    m_idx = (m_idx + 1) % m_solutions.size();
    showSolution(m_idx);
    m_solLabel->setText(
        QString("Solution %1 / %2").arg(m_idx+1).arg(m_solutions.size()));
}

#include "mainwindow.moc"
