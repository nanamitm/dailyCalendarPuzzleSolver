#include <QApplication>
#include <QIcon>
#include "mainwindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("nanamitm");
    app.setApplicationName("PuzzleSolverGUI");
    app.setStyle("Fusion");

    // Set application icon (used for taskbar, dock, window title bar)
    app.setWindowIcon(QIcon(":/puzzlesolver.svg"));

    MainWindow w;
    w.show();
    return app.exec();
}
