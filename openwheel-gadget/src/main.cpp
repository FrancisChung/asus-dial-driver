// openwheel-gadget/src/main.cpp
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>

#include "FunctionRegistry.h"
#include "ProcessRunner.h"
#include "QtDBusCaller.h"
#include "LogindSession.h"
#include "Backlight.h"
#include "DBusListener.h"
#include "DialController.h"
#include "TrayController.h"
#include "TrayIcon.h"
#include "functions/VolumeFunction.h"
#include "functions/BrightnessFunction.h"
#include "functions/MediaFunction.h"
#include "functions/ScrollFunction.h"
#include "functions/ScrollBackendFactory.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    QProcessRunner processRunner;
    QtDBusCaller dbusCaller;
    const QString sessionPath = resolveLogindSessionPath(&dbusCaller);
    const BacklightInfo backlight = readBacklightInfo();

    VolumeFunction volumeFunction(&processRunner);
    BrightnessFunction brightnessFunction(&dbusCaller, sessionPath, backlight.device,
                                          backlight.current, backlight.max);
    MediaFunction mediaFunction(&dbusCaller);
    ScrollFunction scrollFunction(createScrollBackend());

    FunctionRegistry registry;
    registry.registerFunction(&volumeFunction);
    registry.registerFunction(&scrollFunction);
    registry.registerFunction(&brightnessFunction);
    registry.registerFunction(&mediaFunction);

    DialController dialController(&registry);
    DBusListener dbusListener;
    TrayController trayController;
    TrayIcon trayIcon(&trayController);

    QObject::connect(&dbusListener, &DBusListener::rotated, &dialController, &DialController::onRotated);
    QObject::connect(&dbusListener, &DBusListener::pressChanged, &dialController,
                     &DialController::onPressChanged);
    QObject::connect(&trayController, &TrayController::enabledChanged, &dialController,
                     &DialController::setEnabled);
    QObject::connect(&trayController, &TrayController::quitRequested, &app, &QApplication::quit);
    QObject::connect(&dbusListener, &DBusListener::daemonConnectedChanged, &trayIcon,
                     &TrayIcon::setDaemonConnected);

    trayIcon.show();

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("dialController"), &dialController);
    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/OpenWheelGadget/qml/DialOverlay.qml")));
    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
