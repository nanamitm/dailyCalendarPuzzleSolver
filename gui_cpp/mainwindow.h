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
    void onMidnight();       // called when date changes at midnight

private:
    void buildUi();
    void scheduleSolve();    // cancel running worker + restart debounce timer
    void scheduleMidnight(); // arm the midnight timer
    void showSolution(int idx);
    void updateTodayMarker(); // highlight today on the calendar popup

    // ── Widgets ──────────────────────────────────────────────────────────
    QDateEdit*    m_dateEdit   = nullptr;
    QPushButton*  m_todayBtn   = nullptr;
    QCheckBox*    m_findAllChk   = nullptr;
    QAction*      m_autoAct      = nullptr;   // in gear menu
    QAction*      m_slideshowAct = nullptr;   // in gear menu
    QAction*      m_alwaysOnTop  = nullptr;   // in gear menu
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
    bool               m_userCancelled  = false;    // true when user pressed Cancel
    QElapsedTimer      m_elapsed;
    SolverWorker*      m_worker     = nullptr;
    SolveOverlay*      m_overlay    = nullptr;
    QVector<Board>     m_solutions;
    int                m_idx        = 0;
};
