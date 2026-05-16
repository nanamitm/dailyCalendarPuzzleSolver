#pragma once
#include <QWidget>
#include <QDate>
#include "solver.h"

class BoardWidget : public QWidget {
    Q_OBJECT
public:
    explicit BoardWidget(QWidget* parent = nullptr);

    void setBoard(const Board* board, QDate date);
    void setDate(QDate date);   // show empty board for the given date
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    static constexpr int CELL = 64;

    const Board* m_board = nullptr;
    QDate        m_date;

};
