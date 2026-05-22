#pragma once
#include <QObject>
#include <QDate>
#include <QVariantList>
#include <QStringList>
#include <QTimer>
#include <QMap>
#include <QPair>
#include "solver.h"
#include "solverworker.h"

class SolverBackend : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool solving       READ solving       NOTIFY solvingChanged)
    Q_PROPERTY(int  solutionCount READ solutionCount NOTIFY boardChanged)
    Q_PROPERTY(int  currentIndex  READ currentIndex  NOTIFY boardChanged)
    Q_PROPERTY(QString statusText READ statusText    NOTIFY statusTextChanged)
    Q_PROPERTY(QString solLabel   READ solLabel      NOTIFY boardChanged)
    Q_PROPERTY(QVariantList boardData   READ boardData   NOTIFY boardChanged)
    Q_PROPERTY(QVariantList boardLabels READ boardLabels NOTIFY boardChanged)
    Q_PROPERTY(QStringList  pieceSetNames READ pieceSetNames NOTIFY pieceSetNamesChanged)
    Q_PROPERTY(int currentPieceSet READ currentPieceSet
               WRITE setCurrentPieceSet NOTIFY pieceSetNamesChanged)
    Q_PROPERTY(bool slideshow READ slideshow WRITE setSlideshow NOTIFY slideshowChanged)

public:
    explicit SolverBackend(QObject* parent = nullptr);

    bool         solving()         const { return m_solving; }
    int          solutionCount()   const { return (int)m_solutions.size(); }
    int          currentIndex()    const { return m_idx; }
    QString      statusText()      const { return m_statusText; }
    QString      solLabel()        const { return m_solLabel; }
    QVariantList boardData()       const { return m_boardData; }
    QVariantList boardLabels()     const { return m_boardLabels; }
    QStringList  pieceSetNames()   const { return m_pieceSetNames; }
    int          currentPieceSet() const { return m_currentPieceSet; }
    void         setCurrentPieceSet(int idx);
    bool         slideshow()       const { return m_slideshow; }
    Q_INVOKABLE void setSlideshow(bool on);

    Q_INVOKABLE void solve(int year, int month, int day, bool findAll);
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void prevSolution();
    Q_INVOKABLE void nextSolution();
    Q_INVOKABLE void refreshPieceSets();

signals:
    void solvingChanged();
    void boardChanged();
    void statusTextChanged();
    void pieceSetNamesChanged();
    void slideshowChanged();

private slots:
    void onSolved();
    void doSolve();
    void onSlideshowTick();

private:
    void updateBoardData(const Board* board, QDate date);
    void showSolution(int idx);
    void scanPieceSets();
    QString piecesFolder() const;
    void initLcg();
    int  nextLcgIdx();

    bool   m_solving    = false;
    int    m_idx        = 0;
    QString m_statusText;
    QString m_solLabel;
    QVariantList m_boardData;
    QVariantList m_boardLabels;
    QStringList  m_pieceSetNames;
    int m_currentPieceSet = 0;

    QDate m_pendingDate;
    bool  m_pendingFindAll = false;
    bool  m_hasPending     = false;

    bool     m_slideshow      = false;
    quint32  m_lcgState       = 0;
    quint32  m_lcgM           = 1;

    std::vector<Board>          m_solutions;
    std::vector<LoadedPieceSet> m_loadedSets;
    SolverWorker* m_worker          = nullptr;
    QTimer*       m_slideshowTimer  = nullptr;
    QDate         m_solvedDate;
};
