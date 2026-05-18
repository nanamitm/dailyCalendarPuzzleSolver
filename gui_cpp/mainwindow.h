#pragma once
#include <QMainWindow>
#include <QDateEdit>
#include <QPushButton>
#include <QCheckBox>
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
    void onTriggerSolve();   // called by debounce timer
    void onSolved();
    void onPrev();
    void onNext();
    void onSlideshowTick();  // called by slideshow timer (LCG advance)
    void onMidnight();       // called when date changes at midnight

private:
    void buildUi();
    void scheduleSolve();    // restart debounce timer
    void scheduleMidnight(); // arm the midnight timer
    void showSolution(int idx);
    void updateTodayMarker(); // highlight today on the calendar popup
    void initLcg();          // initialise LCG state for current solution count
    int  nextLcgIdx();       // advance LCG and return next solution index

    // ── Widgets ──────────────────────────────────────────────────────────
    QDateEdit*    m_dateEdit   = nullptr;
    QPushButton*  m_todayBtn   = nullptr;
    QCheckBox*    m_findAllChk   = nullptr;
    QAction*      m_autoAct      = nullptr;   // in gear menu
    QAction*      m_onTopAct     = nullptr;   // in gear menu
    QAction*      m_slideshowAct = nullptr;   // in gear menu
    BoardWidget*  m_board      = nullptr;
    QPushButton*  m_prevBtn    = nullptr;
    QPushButton*  m_nextBtn    = nullptr;
    QLabel*       m_solLabel   = nullptr;
    QLabel*       m_statusLbl  = nullptr;

    // ── State ─────────────────────────────────────────────────────────────
    QTimer*            m_debounce    = nullptr;
    QTimer*            m_tickTimer   = nullptr;  // updates overlay every 100 ms
    QTimer*            m_midnight    = nullptr;  // fires once at next midnight
    QTimer*            m_slideshow      = nullptr;  // advances solution every 5 min
    bool               m_savedFindAll   = false;    // state before slideshow locked it
    bool               m_savedAutoMid   = false;
    QElapsedTimer      m_elapsed;
    SolverWorker*      m_worker     = nullptr;
    SolveOverlay*      m_overlay    = nullptr;
    QVector<Board>     m_solutions;
    int                m_idx        = 0;
    quint32            m_lcgState   = 0;  // current LCG value (= current index)
    quint32            m_lcgM       = 1;  // LCG modulus (next power of 2 >= N)
};
