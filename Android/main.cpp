#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "SolverBackend.h"

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    app.setOrganizationName("nanamitm");
    app.setApplicationName("PuzzleSolverAndroid");

    qmlRegisterType<SolverBackend>("PuzzleSolver", 1, 0, "SolverBackend");

    QQmlApplicationEngine engine;
    // Qt does not scan qrc:/ automatically; add it so the PuzzleSolver module is found.
    engine.addImportPath(QStringLiteral("qrc:/"));
    engine.load(QUrl(QStringLiteral("qrc:/PuzzleSolver/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
