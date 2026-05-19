#pragma once
#include <QMainWindow>
#include <QList>
#include <QStringList>
#include <QVector>
#include <QTimer>
#include <QElapsedTimer>

class QSpinBox;
class QCheckBox;
class QPushButton;
class QLabel;
class QProgressBar;
class QListWidget;
class QTabWidget;
class SearchWorker;
class AnalysisWorker;
class MakerBoardWidget;

struct SolutionData {
    QStringList shapes;      // ASCII grid per piece
    QVector<int> board;      // flat [kBYL * kBXL] solved board (Jan 1 Mon)
    QString      desc;       // partition description e.g. "3×4 + 7×5"
    int          nPieces  = 0;
    QString      foundTime;  // "HH:mm:ss"
    bool         bothSides = true;
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    // Search
    void onStart();
    void onStop();
    void onPauseResume();
    void onProgress(int partIdx, int totalParts,
                    long long globalCombo, long long globalTotal,
                    int solFound, double elapsedSec, QString partDesc);
    void onSolutionFound(QStringList shapes, QString desc,
                         QVector<int> boardData, bool bothSides);
    void onSearchFinished(int total, double elapsedSec, bool wasCancelled);
    void onResultClicked(int row);

    // Analysis
    void onAnalysisStart();
    void onAnalysisStop();
    void onAnalysisProgress(int checked, int total);
    void onAnalysisFinished(QStringList sortedEntries, QVector<int> dateData,
                            int totalChecked, int minSol, int maxSol,
                            double avgSol, int oneSolCount, bool wasCancelled);
    void onHardDateClicked(int row);
    void onSaveJson();

private:
    // ── Left panel ────────────────────────────────────────────────────────────
    QTabWidget*       m_leftTabs;

    // Board tab
    MakerBoardWidget* m_boardWidget;
    QLabel*           m_boardInfoLabel;
    QPushButton*      m_saveJsonBtn;

    // Analysis tab
    QSpinBox*         m_maxCountSpin;
    QPushButton*      m_analysisStartBtn;
    QPushButton*      m_analysisStopBtn;
    QProgressBar*     m_analysisProg;
    QLabel*           m_analysisProgLabel;
    QListWidget*      m_hardDatesList;   // sorted hardest-first
    QLabel*           m_statsLabel;

    // ── Right panel ───────────────────────────────────────────────────────────
    QSpinBox*    m_minSize;
    QSpinBox*    m_maxSize;
    QSpinBox*    m_minPieces;
    QSpinBox*    m_maxPieces;
    QCheckBox*   m_bothSides;
    QPushButton* m_startBtn;
    QPushButton* m_pauseBtn;
    QPushButton* m_stopBtn;

    QLabel*       m_lblPartition;
    QLabel*       m_lblPattern;
    QLabel*       m_lblCombos;
    QLabel*       m_lblSolutions;
    QLabel*       m_lblElapsed;
    QProgressBar* m_progressBar;

    QListWidget*        m_resultList;
    QList<SolutionData> m_solutions;
    int                 m_selectedRow = -1;

    SearchWorker*   m_worker         = nullptr;
    AnalysisWorker* m_analysisWorker = nullptr;
    bool            m_isPaused       = false;

    QTimer*       m_searchElapsedTimer = nullptr;
    QElapsedTimer m_searchElapsedClock;

    void setSearchRunning(bool running);
    void setAnalysisRunning(bool running);
    void showBoardForSolution(int row);

    static QString formatTime(double sec);
    static QString formatLong(long long v);
};
