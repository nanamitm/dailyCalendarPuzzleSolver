#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "SolverBackend.h"

#if defined(Q_OS_ANDROID)
#include <QJniObject>
#include <QtCore/qcoreapplication_platform.h>
#endif

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

#if defined(Q_OS_ANDROID)
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([]() {
        QJniObject activity = QNativeInterface::QAndroidApplication::context();
        if (!activity.isValid()) return;
        QJniObject window = activity.callObjectMethod(
            "getWindow", "()Landroid/view/Window;");
        if (!window.isValid()) return;
        // WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON = 0x00000080
        window.callMethod<void>("addFlags", "(I)V", 0x00000080);
    });
#endif

    return app.exec();
}
