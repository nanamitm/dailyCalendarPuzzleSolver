#include <QApplication>
#include <QIcon>
#include "mainwindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("puzzlemaker");
    app.setApplicationName("PuzzleMakerGUI");
    app.setStyle("Fusion");
    app.setWindowIcon(QIcon(":/icon_maker.png"));

    MainWindow w;
    w.show();
    return app.exec();
}
