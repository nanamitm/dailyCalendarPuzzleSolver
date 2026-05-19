#pragma once
#include <QMainWindow>
#include <QDateEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QAction>
#include <QLabel>
#include <QTimer>
#include <QElapsedTimer>
#include <QVector>
#include "boardwidget.h"
#include "solverworker.h"

class SolveOverlay;  // defined in mainwindow.cpp

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onTriggerSolve();
    void onSolved();
    void onPrev();
    void onNext();
    void onSlideshowTick();
    void onMidnight();
    void onRefreshPieceSets();  // re-scan the pieces/ subfolder

private:
    void buildUi();
    void scheduleSolve();
    void scheduleMidnight();
    void showSolution(int idx);
    void updateTodayMarker();
    void initLcg();
    int  nextLcgIdx();
    void scanPieceSets();       // load all *.json from pieces/ subfolder
    void rebuildPieceCombo();   // refresh combo from m_loadedSets

    // ── Widgets ───────────────────────────────────────────────────────────────
    QDateEdit*    m_dateEdit     = nullptr;
    QPushButton*  m_todayBtn     = nullptr;
    QCheckBox*    m_findAllChk   = nullptr;
    QAction*      m_autoAct      = nullptr;
    QAction*      m_onTopAct     = nullptr;
    QAction*      m_slideshowAct = nullptr;
    BoardWidget*  m_board        = nullptr;
    QPushButton*  m_prevBtn      = nullptr;
    QPushButton*  m_nextBtn      = nullptr;
    QLabel*       m_solLabel     = nullptr;
    QLabel*       m_statusLbl    = nullptr;

    // ── Piece set selector ────────────────────────────────────────────────────
    QComboBox*    m_pieceCombo    = nullptr;
    QPushButton*  m_refreshBtn    = nullptr;

    // ── State ─────────────────────────────────────────────────────────────────
    QTimer*            m_debounce    = nullptr;
    QTimer*            m_tickTimer   = nullptr;
    QTimer*            m_midnight    = nullptr;
    QTimer*            m_slideshow   = nullptr;
    bool               m_savedFindAll   = false;
    bool               m_savedAutoMid   = false;
    QElapsedTimer      m_elapsed;
    SolverWorker*      m_worker     = nullptr;
    SolveOverlay*      m_overlay    = nullptr;
    QVector<Board>     m_solutions;
    int                m_idx        = 0;
    quint32            m_lcgState   = 0;
    quint32            m_lcgM       = 1;

    std::vector<LoadedPieceSet> m_loadedSets;
};
