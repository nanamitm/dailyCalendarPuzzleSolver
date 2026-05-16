#pragma once
#include <QMainWindow>
#include <QDateEdit>
#include <QPushButton>
#include <QCheckBox>
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
    void scheduleSolve();    // restart debounce timer
    void scheduleMidnight(); // arm the midnight timer
    void showSolution(int idx);

    // ── Widgets ──────────────────────────────────────────────────────────
    QDateEdit*    m_dateEdit   = nullptr;
    QCheckBox*    m_findAllChk  = nullptr;
    QCheckBox*    m_autoChk     = nullptr;
    BoardWidget*  m_board      = nullptr;
    QPushButton*  m_prevBtn    = nullptr;
    QPushButton*  m_nextBtn    = nullptr;
    QLabel*       m_solLabel   = nullptr;
    QLabel*       m_statusLbl  = nullptr;

    // ── State ─────────────────────────────────────────────────────────────
    QTimer*            m_debounce    = nullptr;
    QTimer*            m_tickTimer   = nullptr;  // updates overlay every 100 ms
    QTimer*            m_midnight    = nullptr;  // fires once at next midnight
    QElapsedTimer      m_elapsed;
    SolverWorker*      m_worker     = nullptr;
    SolveOverlay*      m_overlay    = nullptr;
    QVector<Board>     m_solutions;
    int                m_idx        = 0;
};
