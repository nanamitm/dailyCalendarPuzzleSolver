#include <QApplication>
#include "mainwindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("nanamitm");
    app.setApplicationName("PuzzleSolverGUI");
    app.setStyle("Fusion");

    MainWindow w;
    w.show();
    return app.exec();
}
