# openwheel-gadget v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build `openwheel-gadget`, a Qt6/QML tray + overlay companion app that turns `openwheel-daemon`'s existing `Rotate`/`Press` D-Bus signals into a Surface-Dial-style radial menu controlling volume, brightness, scroll, and media seek.

**Architecture:** Single process. `DBusListener` receives daemon signals and forwards them to `DialController`, a state machine that times press-and-hold to open a radial menu, routes rotation either to menu navigation or to the active `DialFunction`. Four `DialFunction` implementations (Volume, Brightness, Scroll, Media) live behind a common interface in a `FunctionRegistry`, making it trivial to add more later. A `TrayController`/`TrayIcon` pair provides enable/disable + quit. QML (`DialOverlay.qml`) renders the radial menu and a lightweight HUD.

**Tech Stack:** Qt 6.4+ (Core, Gui, Widgets, Qml, Quick, DBus, Test), CMake, C++17, libXtst (X11 scroll), Linux uinput (Wayland scroll, best-effort).

## Global Constraints

- Qt 6.4 minimum (this is what's available via standard apt on the target dev distro; `qt_add_qml_module`, used for the QML module, has existed since Qt 6.2, so 6.4 is sufficient). QML is loaded via `engine.load(QUrl("qrc:/qt/qml/OpenWheelGadget/DialOverlay.qml"))`, not `loadFromModule()` (a 6.5+-only convenience wrapper around the same qrc path) — no visual/behavioral difference, purely which C++ API loads the same QML files.
- No changes to `openwheel-daemon` — v1 uses only the existing `Rotate`(int32 ±1) / `Press`(int32 1|0) signals on `org.asus.dial` / `/org/asus/dial`, session bus.
- Fixed function set for v1: system volume, screen brightness, scroll, media seek. No per-app context, no rotation-speed sensitivity (daemon doesn't report it).
- Hold threshold: 400ms (`Press=1` to menu-open).
- Volume/brightness step: 5% per rotate tick. Media seek step: 5 seconds per tick.
- Scroll: X11 via `libXtst` is the default/required path; Wayland via `uinput` is best-effort, not a release blocker — if unavailable, the Scroll entry is disabled, not crash.
- Brightness uses `org.freedesktop.login1.Session.SetBrightness` on the **system** bus (not session bus) — this is how `logind` exposes brightness control, and it's callable by the session's own user without extra privilege setup.
- Media uses MPRIS (`org.mpris.MediaPlayer2.*`) on the **session** bus.
- Tray menu: enable/disable toggle + quit only (no quick-switch — that's what the dial's own radial menu is for).
- All process-spawning and D-Bus-calling logic must be behind an injectable interface (`ProcessRunner`, `DBusCaller`) so unit tests don't touch real system state.
- Qt Test runs need `QT_QPA_PLATFORM=offscreen` set (no display required in CI/dev sandbox).
- Adding a new `DialFunction` later must require touching only one new class + one registry line — no changes to `DialController`, `TrayIcon`, or QML.

---

## File Structure

```
openwheel-gadget/
  CMakeLists.txt
  src/
    main.cpp
    DialFunction.h
    FunctionRegistry.h / .cpp
    ProcessRunner.h / .cpp
    DBusCaller.h
    QtDBusCaller.h / .cpp
    LogindSession.h / .cpp
    Backlight.h / .cpp
    DBusListener.h / .cpp
    DialController.h / .cpp
    TrayController.h / .cpp
    TrayIcon.h / .cpp
    functions/
      VolumeFunction.h / .cpp
      BrightnessFunction.h / .cpp
      MediaFunction.h / .cpp
      ScrollBackend.h
      X11ScrollBackend.h / .cpp
      UinputScrollBackend.h / .cpp
      ScrollBackendFactory.h / .cpp
      ScrollFunction.h / .cpp
  qml/
    DialOverlay.qml
    RadialMenu.qml
    Hud.qml
  tests/
    CMakeLists.txt
    FakeDBusCaller.h
    FakeDialFunction.h
    test_functionregistry.cpp
    test_volumefunction.cpp
    test_logindsession.cpp
    test_brightnessfunction.cpp
    test_mediafunction.cpp
    test_scrollfunction.cpp
    test_dbuslistener.cpp
    test_dialcontroller.cpp
    test_traycontroller.cpp
```

---

### Task 1: Project scaffolding — Qt6 CMake + smoke-test executable

**Files:**
- Modify: `openwheel-gadget/CMakeLists.txt`
- Create: `openwheel-gadget/src/main.cpp`
- Create: `openwheel-gadget/qml/DialOverlay.qml` (already exists, empty — overwrite)
- Create: `openwheel-gadget/tests/CMakeLists.txt`

**Interfaces:**
- Produces: a linkable `openwheel-gadget-core` static library target (empty for now, later tasks add sources to it) and an `openwheel-gadget` executable target.

- [ ] **Step 1: Write the CMake project file**

```cmake
# openwheel-gadget/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(openwheel-gadget CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Widgets Qml Quick DBus Test)
find_package(PkgConfig REQUIRED)
pkg_check_modules(XTST REQUIRED xtst)

qt_standard_project_setup()

qt_add_library(openwheel-gadget-core STATIC
    src/DialFunction.h
)
target_link_libraries(openwheel-gadget-core PUBLIC
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Qml Qt6::DBus
)
target_link_libraries(openwheel-gadget-core PRIVATE ${XTST_LIBRARIES})
target_include_directories(openwheel-gadget-core PUBLIC src)
target_include_directories(openwheel-gadget-core PRIVATE ${XTST_INCLUDE_DIRS})

qt_add_executable(openwheel-gadget src/main.cpp)
target_link_libraries(openwheel-gadget PRIVATE openwheel-gadget-core Qt6::Quick)

qt_add_qml_module(openwheel-gadget
    URI OpenWheelGadget
    VERSION 1.0
    QML_FILES
        qml/DialOverlay.qml
)

enable_testing()
add_subdirectory(tests)
```

- [ ] **Step 2: Write the empty `DialFunction` interface (needed so the core library has at least one source)**

```cpp
// openwheel-gadget/src/DialFunction.h
#pragma once
#include <QString>

class DialFunction {
public:
    virtual ~DialFunction() = default;
    virtual QString id() const = 0;
    virtual QString displayName() const = 0;
    virtual QString iconName() const = 0;
    virtual bool isAvailable() const = 0;
    virtual void adjust(int direction) = 0;
    virtual QString currentValueLabel() const = 0;
};
```

- [ ] **Step 3: Write a minimal `main.cpp` smoke test**

```cpp
// openwheel-gadget/src/main.cpp
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QUrl>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/OpenWheelGadget/DialOverlay.qml")));
    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
```

- [ ] **Step 4: Write a placeholder `DialOverlay.qml` so the module loads**

```qml
// openwheel-gadget/qml/DialOverlay.qml
import QtQuick
import QtQuick.Window

Window {
    visible: false
    width: 1
    height: 1
}
```

- [ ] **Step 5: Write the tests CMakeLists (empty for now)**

```cmake
# openwheel-gadget/tests/CMakeLists.txt
# Test executables are added by later tasks.
```

- [ ] **Step 6: Build**

Run:
```bash
cd openwheel-gadget
rm -rf build && mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
```
Expected: build succeeds, produces `openwheel-gadget/build/openwheel-gadget` executable. If `Qt6` isn't found, install `qt6-base-dev qt6-declarative-dev libqt6svg6-dev libxtst-dev` (Debian/Ubuntu naming) first. Note: these are build-time (`-dev`) packages only — the runtime smoke test in Step 7 also needs `qml6-module-qtquick qml6-module-qtquick-window qml6-module-qtqml-workerscript` installed, or the executable will build fine but exit immediately with "module ... is not installed" QML errors.

- [ ] **Step 7: Runtime smoke test (headless)**

Run:
```bash
QT_QPA_PLATFORM=offscreen ./openwheel-gadget & sleep 1; kill %1
```
Expected: process starts without crashing and is killed cleanly (no assertion/crash output before the `kill`).

- [ ] **Step 8: Commit**

```bash
git add openwheel-gadget/CMakeLists.txt openwheel-gadget/src/main.cpp openwheel-gadget/src/DialFunction.h openwheel-gadget/qml/DialOverlay.qml openwheel-gadget/tests/CMakeLists.txt
git commit -m "gadget: scaffold Qt6/QML project with smoke-test executable"
```

---

### Task 2: FunctionRegistry

**Files:**
- Create: `openwheel-gadget/src/FunctionRegistry.h`, `.cpp`
- Create: `openwheel-gadget/tests/FakeDialFunction.h`
- Create: `openwheel-gadget/tests/test_functionregistry.cpp`
- Modify: `openwheel-gadget/CMakeLists.txt` (add `src/FunctionRegistry.h`, `src/FunctionRegistry.cpp` to the `qt_add_library` source list)
- Modify: `openwheel-gadget/tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `DialFunction` (Task 1).
- Produces: `class FunctionRegistry { void registerFunction(DialFunction*); int count() const; DialFunction *at(int index) const; int indexOf(const QString &id) const; };` — used by `DialController` (Task 8) and QML (Task 10).

- [ ] **Step 1: Write the shared test fake**

```cpp
// openwheel-gadget/tests/FakeDialFunction.h
#pragma once
#include "DialFunction.h"

class FakeDialFunction : public DialFunction {
public:
    FakeDialFunction(QString id, bool available = true)
        : m_id(std::move(id)), m_available(available) {}

    QString id() const override { return m_id; }
    QString displayName() const override { return m_id; }
    QString iconName() const override { return m_id + "-icon"; }
    bool isAvailable() const override { return m_available; }
    void adjust(int direction) override { lastDirection = direction; adjustCallCount++; }
    QString currentValueLabel() const override { return QStringLiteral("42"); }

    QString m_id;
    bool m_available;
    int lastDirection = 0;
    int adjustCallCount = 0;
};
```

- [ ] **Step 2: Write the failing test**

```cpp
// openwheel-gadget/tests/test_functionregistry.cpp
#include <QtTest/QtTest>
#include "FunctionRegistry.h"
#include "FakeDialFunction.h"

class TestFunctionRegistry : public QObject {
    Q_OBJECT
private slots:
    void countsRegisteredFunctions();
    void indexOfFindsById();
    void indexOfReturnsMinusOneWhenMissing();
};

void TestFunctionRegistry::countsRegisteredFunctions()
{
    FunctionRegistry registry;
    FakeDialFunction volume("volume");
    FakeDialFunction scroll("scroll");
    registry.registerFunction(&volume);
    registry.registerFunction(&scroll);

    QCOMPARE(registry.count(), 2);
    QCOMPARE(registry.at(0)->id(), QStringLiteral("volume"));
    QCOMPARE(registry.at(1)->id(), QStringLiteral("scroll"));
}

void TestFunctionRegistry::indexOfFindsById()
{
    FunctionRegistry registry;
    FakeDialFunction volume("volume");
    FakeDialFunction scroll("scroll");
    registry.registerFunction(&volume);
    registry.registerFunction(&scroll);

    QCOMPARE(registry.indexOf("scroll"), 1);
}

void TestFunctionRegistry::indexOfReturnsMinusOneWhenMissing()
{
    FunctionRegistry registry;
    FakeDialFunction volume("volume");
    registry.registerFunction(&volume);

    QCOMPARE(registry.indexOf("brightness"), -1);
}

QTEST_MAIN(TestFunctionRegistry)
#include "test_functionregistry.moc"
```

- [ ] **Step 3: Add the test target and run to verify it fails (missing FunctionRegistry.h)**

Modify `openwheel-gadget/tests/CMakeLists.txt`:

```cmake
# openwheel-gadget/tests/CMakeLists.txt
qt_add_executable(test_functionregistry test_functionregistry.cpp)
target_link_libraries(test_functionregistry PRIVATE openwheel-gadget-core Qt6::Test)
add_test(NAME FunctionRegistryTest COMMAND test_functionregistry)
```

Run:
```bash
cd openwheel-gadget/build && cmake --build . 2>&1 | tail -20
```
Expected: FAIL — `fatal error: FunctionRegistry.h: No such file or directory`.

- [ ] **Step 4: Write `FunctionRegistry.h` / `.cpp`**

```cpp
// openwheel-gadget/src/FunctionRegistry.h
#pragma once
#include <QList>
#include <QString>
#include "DialFunction.h"

class FunctionRegistry {
public:
    void registerFunction(DialFunction *function);
    int count() const;
    DialFunction *at(int index) const;
    int indexOf(const QString &id) const;

private:
    QList<DialFunction *> m_functions;
};
```

```cpp
// openwheel-gadget/src/FunctionRegistry.cpp
#include "FunctionRegistry.h"

void FunctionRegistry::registerFunction(DialFunction *function)
{
    m_functions.append(function);
}

int FunctionRegistry::count() const
{
    return m_functions.count();
}

DialFunction *FunctionRegistry::at(int index) const
{
    if (index < 0 || index >= m_functions.count()) {
        return nullptr;
    }
    return m_functions.at(index);
}

int FunctionRegistry::indexOf(const QString &id) const
{
    for (int i = 0; i < m_functions.count(); ++i) {
        if (m_functions.at(i)->id() == id) {
            return i;
        }
    }
    return -1;
}
```

Modify `openwheel-gadget/CMakeLists.txt`'s `qt_add_library` block to add the new sources:

```cmake
qt_add_library(openwheel-gadget-core STATIC
    src/DialFunction.h
    src/FunctionRegistry.h
    src/FunctionRegistry.cpp
)
```

- [ ] **Step 5: Run tests, verify pass**

Run:
```bash
cd openwheel-gadget/build && cmake --build . && QT_QPA_PLATFORM=offscreen ctest --output-on-failure -R FunctionRegistryTest
```
Expected: `100% tests passed, 0 tests failed out of 1`.

- [ ] **Step 6: Commit**

```bash
git add openwheel-gadget/CMakeLists.txt openwheel-gadget/src/FunctionRegistry.h openwheel-gadget/src/FunctionRegistry.cpp openwheel-gadget/tests/FakeDialFunction.h openwheel-gadget/tests/test_functionregistry.cpp openwheel-gadget/tests/CMakeLists.txt
git commit -m "gadget: add FunctionRegistry"
```

---

### Task 3: ProcessRunner + VolumeFunction

**Files:**
- Create: `openwheel-gadget/src/ProcessRunner.h`, `.cpp`
- Create: `openwheel-gadget/src/functions/VolumeFunction.h`, `.cpp`
- Create: `openwheel-gadget/tests/test_volumefunction.cpp`
- Modify: `openwheel-gadget/CMakeLists.txt`
- Modify: `openwheel-gadget/tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `DialFunction` (Task 1).
- Produces: `struct ProcessResult { int exitCode; QString standardOutput; };` `class ProcessRunner { virtual ProcessResult run(const QString&, const QStringList&) = 0; };` `class QProcessRunner : public ProcessRunner;` and `class VolumeFunction : public DialFunction { explicit VolumeFunction(ProcessRunner *runner); ... };` — the `ProcessRunner` interface is reused by no other function in v1, but is the pattern later contributors should reuse for any future shell-out-based function.

- [ ] **Step 1: Write `ProcessRunner.h` / `.cpp`**

```cpp
// openwheel-gadget/src/ProcessRunner.h
#pragma once
#include <QString>
#include <QStringList>

struct ProcessResult {
    int exitCode = -1;
    QString standardOutput;
};

class ProcessRunner {
public:
    virtual ~ProcessRunner() = default;
    virtual ProcessResult run(const QString &program, const QStringList &arguments) = 0;
};

class QProcessRunner : public ProcessRunner {
public:
    ProcessResult run(const QString &program, const QStringList &arguments) override;
};
```

```cpp
// openwheel-gadget/src/ProcessRunner.cpp
#include "ProcessRunner.h"
#include <QProcess>

ProcessResult QProcessRunner::run(const QString &program, const QStringList &arguments)
{
    QProcess process;
    process.start(program, arguments);
    process.waitForFinished(2000);
    return ProcessResult{process.exitCode(), QString::fromUtf8(process.readAllStandardOutput())};
}
```

- [ ] **Step 2: Write the failing test for VolumeFunction**

```cpp
// openwheel-gadget/tests/test_volumefunction.cpp
#include <QtTest/QtTest>
#include "functions/VolumeFunction.h"

class FakeProcessRunner : public ProcessRunner {
public:
    ProcessResult run(const QString &program, const QStringList &arguments) override
    {
        lastProgram = program;
        lastArguments = arguments;
        callCount++;
        return nextResult;
    }
    QString lastProgram;
    QStringList lastArguments;
    int callCount = 0;
    ProcessResult nextResult{0, QStringLiteral("Volume: front-left: 32768 /  50% / 0.00 dB")};
};

class TestVolumeFunction : public QObject {
    Q_OBJECT
private slots:
    void adjustUpCallsPactlWithPlus5Percent();
    void adjustDownCallsPactlWithMinus5Percent();
    void currentValueLabelParsesPercentage();
};

void TestVolumeFunction::adjustUpCallsPactlWithPlus5Percent()
{
    FakeProcessRunner runner;
    VolumeFunction volume(&runner);
    volume.adjust(1);

    QCOMPARE(runner.lastProgram, QStringLiteral("pactl"));
    QCOMPARE(runner.lastArguments, QStringList({"set-sink-volume", "@DEFAULT_SINK@", "+5%"}));
}

void TestVolumeFunction::adjustDownCallsPactlWithMinus5Percent()
{
    FakeProcessRunner runner;
    VolumeFunction volume(&runner);
    volume.adjust(-1);

    QCOMPARE(runner.lastArguments, QStringList({"set-sink-volume", "@DEFAULT_SINK@", "-5%"}));
}

void TestVolumeFunction::currentValueLabelParsesPercentage()
{
    FakeProcessRunner runner;
    runner.nextResult = ProcessResult{0, QStringLiteral("Volume: front-left: 32768 /  50% / 0.00 dB")};
    VolumeFunction volume(&runner);

    QCOMPARE(volume.currentValueLabel(), QStringLiteral("50%"));
}

QTEST_MAIN(TestVolumeFunction)
#include "test_volumefunction.moc"
```

- [ ] **Step 3: Add test target, run to verify it fails**

Add to `openwheel-gadget/tests/CMakeLists.txt`:

```cmake
qt_add_executable(test_volumefunction test_volumefunction.cpp)
target_link_libraries(test_volumefunction PRIVATE openwheel-gadget-core Qt6::Test)
add_test(NAME VolumeFunctionTest COMMAND test_volumefunction)
```

Run: `cd openwheel-gadget/build && cmake --build . 2>&1 | tail -20`
Expected: FAIL — `fatal error: functions/VolumeFunction.h: No such file or directory`.

- [ ] **Step 4: Write `VolumeFunction.h` / `.cpp`**

```cpp
// openwheel-gadget/src/functions/VolumeFunction.h
#pragma once
#include "DialFunction.h"
#include "ProcessRunner.h"

class VolumeFunction : public DialFunction {
public:
    explicit VolumeFunction(ProcessRunner *runner);

    QString id() const override;
    QString displayName() const override;
    QString iconName() const override;
    bool isAvailable() const override;
    void adjust(int direction) override;
    QString currentValueLabel() const override;

private:
    ProcessRunner *m_runner;
};
```

```cpp
// openwheel-gadget/src/functions/VolumeFunction.cpp
#include "VolumeFunction.h"
#include <QRegularExpression>

VolumeFunction::VolumeFunction(ProcessRunner *runner) : m_runner(runner) {}

QString VolumeFunction::id() const { return QStringLiteral("volume"); }
QString VolumeFunction::displayName() const { return QStringLiteral("Volume"); }
QString VolumeFunction::iconName() const { return QStringLiteral("audio-volume-high"); }

bool VolumeFunction::isAvailable() const
{
    return m_runner->run(QStringLiteral("pactl"), {QStringLiteral("info")}).exitCode == 0;
}

void VolumeFunction::adjust(int direction)
{
    const QString delta = direction > 0 ? QStringLiteral("+5%") : QStringLiteral("-5%");
    m_runner->run(QStringLiteral("pactl"),
                  {QStringLiteral("set-sink-volume"), QStringLiteral("@DEFAULT_SINK@"), delta});
}

QString VolumeFunction::currentValueLabel() const
{
    const ProcessResult result = m_runner->run(
        QStringLiteral("pactl"), {QStringLiteral("get-sink-volume"), QStringLiteral("@DEFAULT_SINK@")});
    const QRegularExpressionMatch match =
        QRegularExpression(QStringLiteral("(\\d+)%")).match(result.standardOutput);
    return match.hasMatch() ? match.captured(1) + QStringLiteral("%") : QStringLiteral("--");
}
```

Modify `openwheel-gadget/CMakeLists.txt`'s `qt_add_library` block:

```cmake
qt_add_library(openwheel-gadget-core STATIC
    src/DialFunction.h
    src/FunctionRegistry.h
    src/FunctionRegistry.cpp
    src/ProcessRunner.h
    src/ProcessRunner.cpp
    src/functions/VolumeFunction.h
    src/functions/VolumeFunction.cpp
)
```

- [ ] **Step 5: Run tests, verify pass**

Run: `cd openwheel-gadget/build && cmake --build . && QT_QPA_PLATFORM=offscreen ctest --output-on-failure -R VolumeFunctionTest`
Expected: `100% tests passed, 0 tests failed out of 1`.

- [ ] **Step 6: Commit**

```bash
git add openwheel-gadget/CMakeLists.txt openwheel-gadget/src/ProcessRunner.h openwheel-gadget/src/ProcessRunner.cpp openwheel-gadget/src/functions/VolumeFunction.h openwheel-gadget/src/functions/VolumeFunction.cpp openwheel-gadget/tests/test_volumefunction.cpp openwheel-gadget/tests/CMakeLists.txt
git commit -m "gadget: add ProcessRunner and VolumeFunction"
```

---

### Task 4: DBusCaller + LogindSession + Backlight + BrightnessFunction

**Files:**
- Create: `openwheel-gadget/src/DBusCaller.h`, `openwheel-gadget/src/QtDBusCaller.h`, `.cpp`
- Create: `openwheel-gadget/src/LogindSession.h`, `.cpp`
- Create: `openwheel-gadget/src/Backlight.h`, `.cpp`
- Create: `openwheel-gadget/src/functions/BrightnessFunction.h`, `.cpp`
- Create: `openwheel-gadget/tests/FakeDBusCaller.h`
- Create: `openwheel-gadget/tests/test_logindsession.cpp`
- Create: `openwheel-gadget/tests/test_brightnessfunction.cpp`
- Modify: `openwheel-gadget/CMakeLists.txt`, `openwheel-gadget/tests/CMakeLists.txt`

**Interfaces:**
- Produces: `enum class DBusBus { Session, System };` `class DBusCaller { virtual QDBusMessage call(DBusBus, const QString &service, const QString &path, const QString &interface, const QString &method, const QVariantList &args = {}) = 0; };` and `class QtDBusCaller : public DBusCaller;` — reused by Task 5 (`MediaFunction`). `QString resolveLogindSessionPath(DBusCaller *caller);` and `struct BacklightInfo { QString device; int current; int max; }; BacklightInfo readBacklightInfo();` — consumed only by `main.cpp` (Task 11). `class BrightnessFunction : public DialFunction { BrightnessFunction(DBusCaller *caller, QString sessionPath, QString device, int current, int max); };`

- [ ] **Step 1: Write `DBusCaller.h` and `QtDBusCaller.h`/`.cpp`**

```cpp
// openwheel-gadget/src/DBusCaller.h
#pragma once
#include <QDBusMessage>
#include <QString>
#include <QVariantList>

enum class DBusBus { Session, System };

class DBusCaller {
public:
    virtual ~DBusCaller() = default;
    virtual QDBusMessage call(DBusBus bus, const QString &service, const QString &path,
                               const QString &interface, const QString &method,
                               const QVariantList &args = {}) = 0;
};
```

```cpp
// openwheel-gadget/src/QtDBusCaller.h
#pragma once
#include "DBusCaller.h"

class QtDBusCaller : public DBusCaller {
public:
    QDBusMessage call(DBusBus bus, const QString &service, const QString &path,
                       const QString &interface, const QString &method,
                       const QVariantList &args = {}) override;
};
```

```cpp
// openwheel-gadget/src/QtDBusCaller.cpp
#include "QtDBusCaller.h"
#include <QDBusConnection>

QDBusMessage QtDBusCaller::call(DBusBus bus, const QString &service, const QString &path,
                                 const QString &interface, const QString &method,
                                 const QVariantList &args)
{
    QDBusMessage message = QDBusMessage::createMethodCall(service, path, interface, method);
    message.setArguments(args);
    const QDBusConnection connection =
        bus == DBusBus::System ? QDBusConnection::systemBus() : QDBusConnection::sessionBus();
    return connection.call(message);
}
```

- [ ] **Step 2: Write the shared FakeDBusCaller test double**

```cpp
// openwheel-gadget/tests/FakeDBusCaller.h
#pragma once
#include "DBusCaller.h"

class FakeDBusCaller : public DBusCaller {
public:
    QDBusMessage call(DBusBus bus, const QString &service, const QString &path,
                       const QString &interface, const QString &method,
                       const QVariantList &args = {}) override
    {
        lastBus = bus;
        lastService = service;
        lastPath = path;
        lastInterface = interface;
        lastMethod = method;
        lastArgs = args;
        callCount++;

        QDBusMessage request = QDBusMessage::createMethodCall(service, path, interface, method);
        return request.createReply(nextReplyArgs);
    }

    DBusBus lastBus = DBusBus::Session;
    QString lastService;
    QString lastPath;
    QString lastInterface;
    QString lastMethod;
    QVariantList lastArgs;
    int callCount = 0;
    QVariantList nextReplyArgs;
};
```

- [ ] **Step 3: Write the failing test for BrightnessFunction**

```cpp
// openwheel-gadget/tests/test_brightnessfunction.cpp
#include <QtTest/QtTest>
#include "functions/BrightnessFunction.h"
#include "FakeDBusCaller.h"

class TestBrightnessFunction : public QObject {
    Q_OBJECT
private slots:
    void adjustUpCallsSetBrightnessOnSystemBus();
    void adjustClampsAtMax();
    void currentValueLabelIsPercentage();
};

void TestBrightnessFunction::adjustUpCallsSetBrightnessOnSystemBus()
{
    FakeDBusCaller caller;
    BrightnessFunction brightness(&caller, QStringLiteral("/org/freedesktop/login1/session/_31"),
                                  QStringLiteral("intel_backlight"), 50, 100);
    brightness.adjust(1);

    QCOMPARE(caller.lastBus, DBusBus::System);
    QCOMPARE(caller.lastService, QStringLiteral("org.freedesktop.login1"));
    QCOMPARE(caller.lastPath, QStringLiteral("/org/freedesktop/login1/session/_31"));
    QCOMPARE(caller.lastInterface, QStringLiteral("org.freedesktop.login1.Session"));
    QCOMPARE(caller.lastMethod, QStringLiteral("SetBrightness"));
    QCOMPARE(caller.lastArgs.at(0).toString(), QStringLiteral("backlight"));
    QCOMPARE(caller.lastArgs.at(1).toString(), QStringLiteral("intel_backlight"));
    QCOMPARE(caller.lastArgs.at(2).toUInt(), 55u);
}

void TestBrightnessFunction::adjustClampsAtMax()
{
    FakeDBusCaller caller;
    BrightnessFunction brightness(&caller, QStringLiteral("/session/_31"),
                                  QStringLiteral("intel_backlight"), 98, 100);
    brightness.adjust(1);

    QCOMPARE(caller.lastArgs.at(2).toUInt(), 100u);
}

void TestBrightnessFunction::currentValueLabelIsPercentage()
{
    FakeDBusCaller caller;
    BrightnessFunction brightness(&caller, QStringLiteral("/session/_31"),
                                  QStringLiteral("intel_backlight"), 25, 100);

    QCOMPARE(brightness.currentValueLabel(), QStringLiteral("25%"));
}

QTEST_MAIN(TestBrightnessFunction)
#include "test_brightnessfunction.moc"
```

- [ ] **Step 4: Add test target, run to verify it fails**

Add to `openwheel-gadget/tests/CMakeLists.txt`:

```cmake
qt_add_executable(test_brightnessfunction test_brightnessfunction.cpp)
target_link_libraries(test_brightnessfunction PRIVATE openwheel-gadget-core Qt6::Test Qt6::DBus)
add_test(NAME BrightnessFunctionTest COMMAND test_brightnessfunction)
```

Run: `cd openwheel-gadget/build && cmake --build . 2>&1 | tail -20`
Expected: FAIL — `fatal error: functions/BrightnessFunction.h: No such file or directory`.

- [ ] **Step 5: Write `BrightnessFunction.h` / `.cpp`**

```cpp
// openwheel-gadget/src/functions/BrightnessFunction.h
#pragma once
#include "DialFunction.h"
#include "DBusCaller.h"

class BrightnessFunction : public DialFunction {
public:
    BrightnessFunction(DBusCaller *caller, QString sessionPath, QString device, int current, int max);

    QString id() const override;
    QString displayName() const override;
    QString iconName() const override;
    bool isAvailable() const override;
    void adjust(int direction) override;
    QString currentValueLabel() const override;

private:
    DBusCaller *m_caller;
    QString m_sessionPath;
    QString m_device;
    int m_current;
    int m_max;
};
```

```cpp
// openwheel-gadget/src/functions/BrightnessFunction.cpp
#include "BrightnessFunction.h"
#include <QVariant>

BrightnessFunction::BrightnessFunction(DBusCaller *caller, QString sessionPath, QString device,
                                       int current, int max)
    : m_caller(caller), m_sessionPath(std::move(sessionPath)), m_device(std::move(device)),
      m_current(current), m_max(max)
{
}

QString BrightnessFunction::id() const { return QStringLiteral("brightness"); }
QString BrightnessFunction::displayName() const { return QStringLiteral("Brightness"); }
QString BrightnessFunction::iconName() const { return QStringLiteral("display-brightness"); }

bool BrightnessFunction::isAvailable() const
{
    return !m_device.isEmpty() && m_max > 0 && !m_sessionPath.isEmpty();
}

void BrightnessFunction::adjust(int direction)
{
    const int step = qMax(1, m_max * 5 / 100);
    m_current = qBound(0, m_current + direction * step, m_max);
    m_caller->call(DBusBus::System, QStringLiteral("org.freedesktop.login1"), m_sessionPath,
                   QStringLiteral("org.freedesktop.login1.Session"), QStringLiteral("SetBrightness"),
                   {QStringLiteral("backlight"), m_device, static_cast<uint>(m_current)});
}

QString BrightnessFunction::currentValueLabel() const
{
    const int percent = m_max > 0 ? m_current * 100 / m_max : 0;
    return QString::number(percent) + QStringLiteral("%");
}
```

- [ ] **Step 6: Write `LogindSession.h` / `.cpp`**

```cpp
// openwheel-gadget/src/LogindSession.h
#pragma once
#include <QString>
#include "DBusCaller.h"

QString resolveLogindSessionPath(DBusCaller *caller);
```

```cpp
// openwheel-gadget/src/LogindSession.cpp
#include "LogindSession.h"
#include <QDBusObjectPath>

QString resolveLogindSessionPath(DBusCaller *caller)
{
    const QString sessionId = qEnvironmentVariable("XDG_SESSION_ID");
    if (sessionId.isEmpty()) {
        return QString();
    }

    const QDBusMessage reply =
        caller->call(DBusBus::System, QStringLiteral("org.freedesktop.login1"),
                     QStringLiteral("/org/freedesktop/login1"),
                     QStringLiteral("org.freedesktop.login1.Manager"), QStringLiteral("GetSession"),
                     {sessionId});

    if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().isEmpty()) {
        return QString();
    }
    return qvariant_cast<QDBusObjectPath>(reply.arguments().first()).path();
}
```

- [ ] **Step 7: Write the failing test for `resolveLogindSessionPath`**

```cpp
// openwheel-gadget/tests/test_logindsession.cpp
#include <QtTest/QtTest>
#include <QDBusObjectPath>
#include "LogindSession.h"
#include "FakeDBusCaller.h"

class TestLogindSession : public QObject {
    Q_OBJECT
private slots:
    void returnsEmptyWhenNoSessionIdEnvVar();
    void resolvesSessionPathViaSystemBus();
};

void TestLogindSession::returnsEmptyWhenNoSessionIdEnvVar()
{
    qunsetenv("XDG_SESSION_ID");
    FakeDBusCaller caller;

    QVERIFY(resolveLogindSessionPath(&caller).isEmpty());
    QCOMPARE(caller.callCount, 0);
}

void TestLogindSession::resolvesSessionPathViaSystemBus()
{
    qputenv("XDG_SESSION_ID", "3");
    FakeDBusCaller caller;
    caller.nextReplyArgs = {QVariant::fromValue(QDBusObjectPath("/org/freedesktop/login1/session/_33"))};

    const QString path = resolveLogindSessionPath(&caller);

    QCOMPARE(caller.lastBus, DBusBus::System);
    QCOMPARE(caller.lastService, QStringLiteral("org.freedesktop.login1"));
    QCOMPARE(caller.lastMethod, QStringLiteral("GetSession"));
    QCOMPARE(caller.lastArgs.at(0).toString(), QStringLiteral("3"));
    QCOMPARE(path, QStringLiteral("/org/freedesktop/login1/session/_33"));

    qunsetenv("XDG_SESSION_ID");
}

QTEST_MAIN(TestLogindSession)
#include "test_logindsession.moc"
```

- [ ] **Step 8: Add test target, run to verify it fails**

Add to `openwheel-gadget/tests/CMakeLists.txt`:

```cmake
qt_add_executable(test_logindsession test_logindsession.cpp)
target_link_libraries(test_logindsession PRIVATE openwheel-gadget-core Qt6::Test Qt6::DBus)
add_test(NAME LogindSessionTest COMMAND test_logindsession)
```

Run: `cd openwheel-gadget/build && cmake --build . 2>&1 | tail -20`
Expected: FAIL — `fatal error: LogindSession.h: No such file or directory`.

- [ ] **Step 9: Write `Backlight.h` / `.cpp`**

```cpp
// openwheel-gadget/src/Backlight.h
#pragma once
#include <QString>

struct BacklightInfo {
    QString device;
    int current = 0;
    int max = 0;
};

BacklightInfo readBacklightInfo();
```

```cpp
// openwheel-gadget/src/Backlight.cpp
#include "Backlight.h"
#include <QDir>
#include <QFile>

static int readIntFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return 0;
    }
    return file.readAll().trimmed().toInt();
}

BacklightInfo readBacklightInfo()
{
    BacklightInfo info;
    QDir backlightDir(QStringLiteral("/sys/class/backlight"));
    const QStringList entries = backlightDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    if (entries.isEmpty()) {
        return info;
    }

    info.device = entries.first();
    const QString basePath = backlightDir.filePath(info.device);
    info.current = readIntFile(basePath + QStringLiteral("/brightness"));
    info.max = readIntFile(basePath + QStringLiteral("/max_brightness"));
    return info;
}
```

- [ ] **Step 10: Wire new sources into CMakeLists.txt**

Modify `openwheel-gadget/CMakeLists.txt`'s `qt_add_library` block:

```cmake
qt_add_library(openwheel-gadget-core STATIC
    src/DialFunction.h
    src/FunctionRegistry.h
    src/FunctionRegistry.cpp
    src/ProcessRunner.h
    src/ProcessRunner.cpp
    src/DBusCaller.h
    src/QtDBusCaller.h
    src/QtDBusCaller.cpp
    src/LogindSession.h
    src/LogindSession.cpp
    src/Backlight.h
    src/Backlight.cpp
    src/functions/VolumeFunction.h
    src/functions/VolumeFunction.cpp
    src/functions/BrightnessFunction.h
    src/functions/BrightnessFunction.cpp
)
```

- [ ] **Step 11: Run tests, verify pass**

Run: `cd openwheel-gadget/build && cmake --build . && QT_QPA_PLATFORM=offscreen ctest --output-on-failure -R "BrightnessFunctionTest|LogindSessionTest"`
Expected: `100% tests passed, 0 tests failed out of 2`.

- [ ] **Step 12: Commit**

```bash
git add openwheel-gadget/CMakeLists.txt openwheel-gadget/src/DBusCaller.h openwheel-gadget/src/QtDBusCaller.h openwheel-gadget/src/QtDBusCaller.cpp openwheel-gadget/src/LogindSession.h openwheel-gadget/src/LogindSession.cpp openwheel-gadget/src/Backlight.h openwheel-gadget/src/Backlight.cpp openwheel-gadget/src/functions/BrightnessFunction.h openwheel-gadget/src/functions/BrightnessFunction.cpp openwheel-gadget/tests/FakeDBusCaller.h openwheel-gadget/tests/test_brightnessfunction.cpp openwheel-gadget/tests/test_logindsession.cpp openwheel-gadget/tests/CMakeLists.txt
git commit -m "gadget: add DBusCaller, LogindSession, Backlight, and BrightnessFunction"
```

---

### Task 5: MediaFunction (MPRIS)

**Files:**
- Create: `openwheel-gadget/src/functions/MediaFunction.h`, `.cpp`
- Create: `openwheel-gadget/tests/test_mediafunction.cpp`
- Modify: `openwheel-gadget/CMakeLists.txt`, `openwheel-gadget/tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `DBusCaller`/`DBusBus` (Task 4), `FakeDBusCaller` (Task 4, test-only).
- Produces: `class MediaFunction : public DialFunction { explicit MediaFunction(DBusCaller *caller); };`

- [ ] **Step 1: Write the failing test**

```cpp
// openwheel-gadget/tests/test_mediafunction.cpp
#include <QtTest/QtTest>
#include "functions/MediaFunction.h"
#include "FakeDBusCaller.h"

class TestMediaFunction : public QObject {
    Q_OBJECT
private slots:
    void isAvailableFindsMprisPlayer();
    void isAvailableFalseWhenNoPlayer();
    void adjustSeeksFiveSecondsInMicroseconds();
    void currentValueLabelStripsPrefix();
};

void TestMediaFunction::isAvailableFindsMprisPlayer()
{
    FakeDBusCaller caller;
    caller.nextReplyArgs = {QStringList{"org.freedesktop.DBus", "org.mpris.MediaPlayer2.spotify"}};
    MediaFunction media(&caller);

    QVERIFY(media.isAvailable());
}

void TestMediaFunction::isAvailableFalseWhenNoPlayer()
{
    FakeDBusCaller caller;
    caller.nextReplyArgs = {QStringList{"org.freedesktop.DBus"}};
    MediaFunction media(&caller);

    QVERIFY(!media.isAvailable());
}

void TestMediaFunction::adjustSeeksFiveSecondsInMicroseconds()
{
    FakeDBusCaller caller;
    caller.nextReplyArgs = {QStringList{"org.mpris.MediaPlayer2.spotify"}};
    MediaFunction media(&caller);

    media.adjust(1);

    QCOMPARE(caller.lastService, QStringLiteral("org.mpris.MediaPlayer2.spotify"));
    QCOMPARE(caller.lastPath, QStringLiteral("/org/mpris/MediaPlayer2"));
    QCOMPARE(caller.lastInterface, QStringLiteral("org.mpris.MediaPlayer2.Player"));
    QCOMPARE(caller.lastMethod, QStringLiteral("Seek"));
    QCOMPARE(caller.lastArgs.at(0).toLongLong(), 5000000LL);
}

void TestMediaFunction::currentValueLabelStripsPrefix()
{
    FakeDBusCaller caller;
    caller.nextReplyArgs = {QStringList{"org.mpris.MediaPlayer2.spotify"}};
    MediaFunction media(&caller);

    QCOMPARE(media.currentValueLabel(), QStringLiteral("spotify"));
}

QTEST_MAIN(TestMediaFunction)
#include "test_mediafunction.moc"
```

- [ ] **Step 2: Add test target, run to verify it fails**

Add to `openwheel-gadget/tests/CMakeLists.txt`:

```cmake
qt_add_executable(test_mediafunction test_mediafunction.cpp)
target_link_libraries(test_mediafunction PRIVATE openwheel-gadget-core Qt6::Test Qt6::DBus)
add_test(NAME MediaFunctionTest COMMAND test_mediafunction)
```

Run: `cd openwheel-gadget/build && cmake --build . 2>&1 | tail -20`
Expected: FAIL — `fatal error: functions/MediaFunction.h: No such file or directory`.

- [ ] **Step 3: Write `MediaFunction.h` / `.cpp`**

```cpp
// openwheel-gadget/src/functions/MediaFunction.h
#pragma once
#include "DialFunction.h"
#include "DBusCaller.h"

class MediaFunction : public DialFunction {
public:
    explicit MediaFunction(DBusCaller *caller);

    QString id() const override;
    QString displayName() const override;
    QString iconName() const override;
    bool isAvailable() const override;
    void adjust(int direction) override;
    QString currentValueLabel() const override;

private:
    QString findActivePlayer() const;
    DBusCaller *m_caller;
};
```

```cpp
// openwheel-gadget/src/functions/MediaFunction.cpp
#include "MediaFunction.h"

MediaFunction::MediaFunction(DBusCaller *caller) : m_caller(caller) {}

QString MediaFunction::id() const { return QStringLiteral("media"); }
QString MediaFunction::displayName() const { return QStringLiteral("Media"); }
QString MediaFunction::iconName() const { return QStringLiteral("media-playback-start"); }

QString MediaFunction::findActivePlayer() const
{
    const QDBusMessage reply =
        m_caller->call(DBusBus::Session, QStringLiteral("org.freedesktop.DBus"),
                       QStringLiteral("/org/freedesktop/DBus"), QStringLiteral("org.freedesktop.DBus"),
                       QStringLiteral("ListNames"));
    if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().isEmpty()) {
        return QString();
    }
    const QStringList names = reply.arguments().first().toStringList();
    for (const QString &name : names) {
        if (name.startsWith(QStringLiteral("org.mpris.MediaPlayer2."))) {
            return name;
        }
    }
    return QString();
}

bool MediaFunction::isAvailable() const
{
    return !findActivePlayer().isEmpty();
}

void MediaFunction::adjust(int direction)
{
    const QString service = findActivePlayer();
    if (service.isEmpty()) {
        return;
    }
    const qlonglong offsetMicroseconds = static_cast<qlonglong>(direction) * 5 * 1000 * 1000;
    m_caller->call(DBusBus::Session, service, QStringLiteral("/org/mpris/MediaPlayer2"),
                   QStringLiteral("org.mpris.MediaPlayer2.Player"), QStringLiteral("Seek"),
                   {offsetMicroseconds});
}

QString MediaFunction::currentValueLabel() const
{
    const QString service = findActivePlayer();
    if (service.isEmpty()) {
        return QStringLiteral("--");
    }
    return service.mid(QStringLiteral("org.mpris.MediaPlayer2.").length());
}
```

- [ ] **Step 4: Wire into CMakeLists.txt**

Modify `openwheel-gadget/CMakeLists.txt`'s `qt_add_library` block, adding after the `BrightnessFunction` lines:

```cmake
    src/functions/MediaFunction.h
    src/functions/MediaFunction.cpp
```

- [ ] **Step 5: Run tests, verify pass**

Run: `cd openwheel-gadget/build && cmake --build . && QT_QPA_PLATFORM=offscreen ctest --output-on-failure -R MediaFunctionTest`
Expected: `100% tests passed, 0 tests failed out of 1`.

- [ ] **Step 6: Commit**

```bash
git add openwheel-gadget/CMakeLists.txt openwheel-gadget/src/functions/MediaFunction.h openwheel-gadget/src/functions/MediaFunction.cpp openwheel-gadget/tests/test_mediafunction.cpp openwheel-gadget/tests/CMakeLists.txt
git commit -m "gadget: add MediaFunction (MPRIS seek)"
```

---

### Task 6: ScrollBackend + X11/Uinput backends + ScrollFunction

**Files:**
- Create: `openwheel-gadget/src/functions/ScrollBackend.h`
- Create: `openwheel-gadget/src/functions/X11ScrollBackend.h`, `.cpp`
- Create: `openwheel-gadget/src/functions/UinputScrollBackend.h`, `.cpp`
- Create: `openwheel-gadget/src/functions/ScrollFunction.h`, `.cpp`
- Create: `openwheel-gadget/tests/test_scrollfunction.cpp`
- Modify: `openwheel-gadget/CMakeLists.txt`, `openwheel-gadget/tests/CMakeLists.txt`

**Interfaces:**
- Produces: `class ScrollBackend { virtual bool isAvailable() const = 0; virtual void scroll(int direction) = 0; };`, `class ScrollFunction : public DialFunction { explicit ScrollFunction(std::unique_ptr<ScrollBackend> backend); };`, `std::unique_ptr<ScrollBackend> createScrollBackend();` (consumed by `main.cpp`, Task 11).
- Note: `X11ScrollBackend` and `UinputScrollBackend` talk to real X11/`uinput` and are **not** unit tested here (no display/no `/dev/uinput` write access in CI) — only `ScrollFunction`'s routing logic is unit tested, via a fake backend. The real backends get a manual verification step.

- [ ] **Step 1: Write `ScrollBackend.h`**

```cpp
// openwheel-gadget/src/functions/ScrollBackend.h
#pragma once

class ScrollBackend {
public:
    virtual ~ScrollBackend() = default;
    virtual bool isAvailable() const = 0;
    virtual void scroll(int direction) = 0;
};
```

- [ ] **Step 2: Write the failing test for ScrollFunction**

```cpp
// openwheel-gadget/tests/test_scrollfunction.cpp
#include <QtTest/QtTest>
#include "functions/ScrollFunction.h"

class FakeScrollBackend : public ScrollBackend {
public:
    explicit FakeScrollBackend(bool available) : m_available(available) {}
    bool isAvailable() const override { return m_available; }
    void scroll(int direction) override { lastDirection = direction; scrollCallCount++; }

    bool m_available;
    int lastDirection = 0;
    int scrollCallCount = 0;
};

class TestScrollFunction : public QObject {
    Q_OBJECT
private slots:
    void adjustForwardsToAvailableBackend();
    void adjustNoOpsWhenBackendUnavailable();
    void adjustNoOpsWhenBackendNull();
};

void TestScrollFunction::adjustForwardsToAvailableBackend()
{
    auto backend = std::make_unique<FakeScrollBackend>(true);
    FakeScrollBackend *raw = backend.get();
    ScrollFunction scroll(std::move(backend));

    scroll.adjust(-1);

    QCOMPARE(raw->scrollCallCount, 1);
    QCOMPARE(raw->lastDirection, -1);
    QVERIFY(scroll.isAvailable());
}

void TestScrollFunction::adjustNoOpsWhenBackendUnavailable()
{
    auto backend = std::make_unique<FakeScrollBackend>(false);
    FakeScrollBackend *raw = backend.get();
    ScrollFunction scroll(std::move(backend));

    scroll.adjust(1);

    QCOMPARE(raw->scrollCallCount, 0);
    QVERIFY(!scroll.isAvailable());
}

void TestScrollFunction::adjustNoOpsWhenBackendNull()
{
    ScrollFunction scroll(nullptr);

    scroll.adjust(1);

    QVERIFY(!scroll.isAvailable());
}

QTEST_MAIN(TestScrollFunction)
#include "test_scrollfunction.moc"
```

- [ ] **Step 3: Add test target, run to verify it fails**

Add to `openwheel-gadget/tests/CMakeLists.txt`:

```cmake
qt_add_executable(test_scrollfunction test_scrollfunction.cpp)
target_link_libraries(test_scrollfunction PRIVATE openwheel-gadget-core Qt6::Test)
add_test(NAME ScrollFunctionTest COMMAND test_scrollfunction)
```

Run: `cd openwheel-gadget/build && cmake --build . 2>&1 | tail -20`
Expected: FAIL — `fatal error: functions/ScrollFunction.h: No such file or directory`.

- [ ] **Step 4: Write `ScrollFunction.h` / `.cpp`**

```cpp
// openwheel-gadget/src/functions/ScrollFunction.h
#pragma once
#include <memory>
#include "DialFunction.h"
#include "ScrollBackend.h"

class ScrollFunction : public DialFunction {
public:
    explicit ScrollFunction(std::unique_ptr<ScrollBackend> backend);

    QString id() const override;
    QString displayName() const override;
    QString iconName() const override;
    bool isAvailable() const override;
    void adjust(int direction) override;
    QString currentValueLabel() const override;

private:
    std::unique_ptr<ScrollBackend> m_backend;
};
```

```cpp
// openwheel-gadget/src/functions/ScrollFunction.cpp
#include "ScrollFunction.h"

ScrollFunction::ScrollFunction(std::unique_ptr<ScrollBackend> backend) : m_backend(std::move(backend)) {}

QString ScrollFunction::id() const { return QStringLiteral("scroll"); }
QString ScrollFunction::displayName() const { return QStringLiteral("Scroll"); }
QString ScrollFunction::iconName() const { return QStringLiteral("input-mouse"); }

bool ScrollFunction::isAvailable() const
{
    return m_backend && m_backend->isAvailable();
}

void ScrollFunction::adjust(int direction)
{
    if (isAvailable()) {
        m_backend->scroll(direction);
    }
}

QString ScrollFunction::currentValueLabel() const { return QStringLiteral("Scroll"); }
```

- [ ] **Step 5: Wire into CMakeLists.txt, run tests, verify pass**

Add to `qt_add_library` block:

```cmake
    src/functions/ScrollBackend.h
    src/functions/ScrollFunction.h
    src/functions/ScrollFunction.cpp
```

Run: `cd openwheel-gadget/build && cmake --build . && QT_QPA_PLATFORM=offscreen ctest --output-on-failure -R ScrollFunctionTest`
Expected: `100% tests passed, 0 tests failed out of 3`.

- [ ] **Step 6: Commit the tested routing logic**

```bash
git add openwheel-gadget/CMakeLists.txt openwheel-gadget/src/functions/ScrollBackend.h openwheel-gadget/src/functions/ScrollFunction.h openwheel-gadget/src/functions/ScrollFunction.cpp openwheel-gadget/tests/test_scrollfunction.cpp openwheel-gadget/tests/CMakeLists.txt
git commit -m "gadget: add ScrollFunction with pluggable backend"
```

- [ ] **Step 7: Write `X11ScrollBackend.h` / `.cpp` (real XTest calls, no unit test — manual verification only)**

```cpp
// openwheel-gadget/src/functions/X11ScrollBackend.h
#pragma once
#include "ScrollBackend.h"
#include <X11/Xlib.h>

class X11ScrollBackend : public ScrollBackend {
public:
    X11ScrollBackend();
    ~X11ScrollBackend() override;

    bool isAvailable() const override;
    void scroll(int direction) override;

private:
    Display *m_display = nullptr;
};
```

```cpp
// openwheel-gadget/src/functions/X11ScrollBackend.cpp
#include "X11ScrollBackend.h"
#include <X11/extensions/XTest.h>

X11ScrollBackend::X11ScrollBackend()
{
    m_display = XOpenDisplay(nullptr);
}

X11ScrollBackend::~X11ScrollBackend()
{
    if (m_display) {
        XCloseDisplay(m_display);
    }
}

bool X11ScrollBackend::isAvailable() const
{
    return m_display != nullptr;
}

void X11ScrollBackend::scroll(int direction)
{
    if (!m_display) {
        return;
    }
    const unsigned int button = direction > 0 ? 4 : 5; // X11 scroll-wheel button convention
    XTestFakeButtonEvent(m_display, button, True, CurrentTime);
    XTestFakeButtonEvent(m_display, button, False, CurrentTime);
    XFlush(m_display);
}
```

- [ ] **Step 8: Write `UinputScrollBackend.h` / `.cpp` (best-effort Wayland path, no unit test — manual verification only)**

```cpp
// openwheel-gadget/src/functions/UinputScrollBackend.h
#pragma once
#include "ScrollBackend.h"

class UinputScrollBackend : public ScrollBackend {
public:
    UinputScrollBackend();
    ~UinputScrollBackend() override;

    bool isAvailable() const override;
    void scroll(int direction) override;

private:
    int m_fd = -1;
};
```

```cpp
// openwheel-gadget/src/functions/UinputScrollBackend.cpp
#include "UinputScrollBackend.h"
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <linux/uinput.h>

UinputScrollBackend::UinputScrollBackend()
{
    m_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (m_fd < 0) {
        return;
    }

    ioctl(m_fd, UI_SET_EVBIT, EV_REL);
    ioctl(m_fd, UI_SET_RELBIT, REL_WHEEL);
    ioctl(m_fd, UI_SET_EVBIT, EV_SYN);

    struct uinput_setup setup{};
    setup.id.bustype = BUS_VIRTUAL;
    setup.id.vendor = 0x1234;
    setup.id.product = 0x5678;
    std::strncpy(setup.name, "openwheel-gadget-scroll", sizeof(setup.name) - 1);

    if (ioctl(m_fd, UI_DEV_SETUP, &setup) < 0 || ioctl(m_fd, UI_DEV_CREATE) < 0) {
        close(m_fd);
        m_fd = -1;
    }
}

UinputScrollBackend::~UinputScrollBackend()
{
    if (m_fd >= 0) {
        ioctl(m_fd, UI_DEV_DESTROY);
        close(m_fd);
    }
}

bool UinputScrollBackend::isAvailable() const
{
    return m_fd >= 0;
}

void UinputScrollBackend::scroll(int direction)
{
    if (m_fd < 0) {
        return;
    }

    struct input_event event{};
    event.type = EV_REL;
    event.code = REL_WHEEL;
    event.value = direction;
    write(m_fd, &event, sizeof(event));

    struct input_event sync{};
    sync.type = EV_SYN;
    sync.code = SYN_REPORT;
    sync.value = 0;
    write(m_fd, &sync, sizeof(sync));
}
```

- [ ] **Step 9: Write the backend factory**

```cpp
// openwheel-gadget/src/functions/ScrollBackendFactory.h
#pragma once
#include <memory>
#include "ScrollBackend.h"

std::unique_ptr<ScrollBackend> createScrollBackend();
```

```cpp
// openwheel-gadget/src/functions/ScrollBackendFactory.cpp
#include "ScrollBackendFactory.h"
#include "X11ScrollBackend.h"
#include "UinputScrollBackend.h"
#include <QtGlobal>

std::unique_ptr<ScrollBackend> createScrollBackend()
{
    if (qEnvironmentVariableIsSet("DISPLAY")) {
        auto x11Backend = std::make_unique<X11ScrollBackend>();
        if (x11Backend->isAvailable()) {
            return x11Backend;
        }
    }

    auto uinputBackend = std::make_unique<UinputScrollBackend>();
    if (uinputBackend->isAvailable()) {
        return uinputBackend;
    }

    return nullptr;
}
```

- [ ] **Step 10: Wire into CMakeLists.txt with libXtst linking**

Modify `openwheel-gadget/CMakeLists.txt`'s `qt_add_library` block, adding:

```cmake
    src/functions/X11ScrollBackend.h
    src/functions/X11ScrollBackend.cpp
    src/functions/UinputScrollBackend.h
    src/functions/UinputScrollBackend.cpp
    src/functions/ScrollBackendFactory.h
    src/functions/ScrollBackendFactory.cpp
```

(The `${XTST_LIBRARIES}` link and `${XTST_INCLUDE_DIRS}` include set up in Task 1 already cover `X11ScrollBackend`'s `libXtst`/`libX11` dependency.)

Run: `cd openwheel-gadget/build && cmake --build .`
Expected: builds successfully (requires `libxtst-dev` / equivalent installed, per Global Constraints).

- [ ] **Step 11: Manual verification (X11 backend)**

On an X11 session, run:
```bash
QT_QPA_PLATFORM=offscreen ./openwheel-gadget/build/openwheel-gadget &
```
This isn't automatable further at this task's level (full wiring happens in Task 11) — defer full manual scroll verification to Task 11's end-to-end check.

- [ ] **Step 12: Commit**

```bash
git add openwheel-gadget/CMakeLists.txt openwheel-gadget/src/functions/X11ScrollBackend.h openwheel-gadget/src/functions/X11ScrollBackend.cpp openwheel-gadget/src/functions/UinputScrollBackend.h openwheel-gadget/src/functions/UinputScrollBackend.cpp openwheel-gadget/src/functions/ScrollBackendFactory.h openwheel-gadget/src/functions/ScrollBackendFactory.cpp
git commit -m "gadget: add X11 (XTest) and Wayland (uinput) scroll backends"
```

---

### Task 7: DBusListener

**Files:**
- Create: `openwheel-gadget/src/DBusListener.h`, `.cpp`
- Create: `openwheel-gadget/tests/test_dbuslistener.cpp`
- Modify: `openwheel-gadget/CMakeLists.txt`, `openwheel-gadget/tests/CMakeLists.txt`

**Interfaces:**
- Produces: `class DBusListener : public QObject { Q_OBJECT public: explicit DBusListener(QObject *parent = nullptr); signals: void rotated(int direction); void pressChanged(bool pressed); void daemonConnectedChanged(bool connected); };` — consumed by `DialController` (Task 8) and `main.cpp` (Task 11).
- Test note: this test connects to the **real session bus** (there is no fake for `QDBusConnection` itself) and emits synthetic `org.asus.dial` signals to verify the listener reacts. Requires an active D-Bus session bus; if none is running, wrap the test command with `dbus-run-session --`.

- [ ] **Step 1: Write the failing test**

```cpp
// openwheel-gadget/tests/test_dbuslistener.cpp
#include <QtTest/QtTest>
#include <QDBusConnection>
#include <QDBusMessage>
#include "DBusListener.h"

class TestDBusListener : public QObject {
    Q_OBJECT
private slots:
    void rotateSignalEmitsRotated();
    void pressSignalEmitsPressChanged();
};

void TestDBusListener::rotateSignalEmitsRotated()
{
    DBusListener listener;
    QSignalSpy spy(&listener, &DBusListener::rotated);

    QDBusMessage signal = QDBusMessage::createSignal(
        QStringLiteral("/org/asus/dial"), QStringLiteral("org.asus.dial"), QStringLiteral("Rotate"));
    signal << 1;
    QVERIFY(QDBusConnection::sessionBus().send(signal));

    QVERIFY(spy.wait(1000));
    QCOMPARE(spy.at(0).at(0).toInt(), 1);
}

void TestDBusListener::pressSignalEmitsPressChanged()
{
    DBusListener listener;
    QSignalSpy spy(&listener, &DBusListener::pressChanged);

    QDBusMessage signal = QDBusMessage::createSignal(
        QStringLiteral("/org/asus/dial"), QStringLiteral("org.asus.dial"), QStringLiteral("Press"));
    signal << 1;
    QVERIFY(QDBusConnection::sessionBus().send(signal));

    QVERIFY(spy.wait(1000));
    QCOMPARE(spy.at(0).at(0).toBool(), true);
}

QTEST_MAIN(TestDBusListener)
#include "test_dbuslistener.moc"
```

- [ ] **Step 2: Add test target, run to verify it fails**

Add to `openwheel-gadget/tests/CMakeLists.txt`:

```cmake
qt_add_executable(test_dbuslistener test_dbuslistener.cpp)
target_link_libraries(test_dbuslistener PRIVATE openwheel-gadget-core Qt6::Test Qt6::DBus)
add_test(NAME DBusListenerTest COMMAND test_dbuslistener)
```

Run: `cd openwheel-gadget/build && cmake --build . 2>&1 | tail -20`
Expected: FAIL — `fatal error: DBusListener.h: No such file or directory`.

- [ ] **Step 3: Write `DBusListener.h` / `.cpp`**

```cpp
// openwheel-gadget/src/DBusListener.h
#pragma once
#include <QObject>
#include <QDBusServiceWatcher>

class DBusListener : public QObject {
    Q_OBJECT
public:
    explicit DBusListener(QObject *parent = nullptr);

signals:
    void rotated(int direction);
    void pressChanged(bool pressed);
    void daemonConnectedChanged(bool connected);

private slots:
    void onRotate(int value);
    void onPress(int value);

private:
    QDBusServiceWatcher m_watcher;
};
```

```cpp
// openwheel-gadget/src/DBusListener.cpp
#include "DBusListener.h"
#include <QDBusConnection>

DBusListener::DBusListener(QObject *parent)
    : QObject(parent),
      m_watcher(QStringLiteral("org.asus.dial"), QDBusConnection::sessionBus(),
                QDBusServiceWatcher::WatchForOwnerChange)
{
    QDBusConnection::sessionBus().connect(QString(), QStringLiteral("/org/asus/dial"),
                                          QStringLiteral("org.asus.dial"), QStringLiteral("Rotate"),
                                          this, SLOT(onRotate(int)));
    QDBusConnection::sessionBus().connect(QString(), QStringLiteral("/org/asus/dial"),
                                          QStringLiteral("org.asus.dial"), QStringLiteral("Press"),
                                          this, SLOT(onPress(int)));

    connect(&m_watcher, &QDBusServiceWatcher::serviceRegistered, this,
            [this]() { emit daemonConnectedChanged(true); });
    connect(&m_watcher, &QDBusServiceWatcher::serviceUnregistered, this,
            [this]() { emit daemonConnectedChanged(false); });

    const bool initiallyConnected =
        QDBusConnection::sessionBus().interface()->isServiceRegistered(QStringLiteral("org.asus.dial"));
    emit daemonConnectedChanged(initiallyConnected);
}

void DBusListener::onRotate(int value) { emit rotated(value); }
void DBusListener::onPress(int value) { emit pressChanged(value != 0); }
```

- [ ] **Step 4: Wire into CMakeLists.txt**

Modify `openwheel-gadget/CMakeLists.txt`'s `qt_add_library` block, adding:

```cmake
    src/DBusListener.h
    src/DBusListener.cpp
```

- [ ] **Step 5: Run tests, verify pass**

Run: `cd openwheel-gadget/build && cmake --build . && QT_QPA_PLATFORM=offscreen ctest --output-on-failure -R DBusListenerTest`
Expected: `100% tests passed, 0 tests failed out of 1`. If it fails with a D-Bus connection error, re-run as: `dbus-run-session -- ctest --output-on-failure -R DBusListenerTest` (adjusting working directory as needed).

- [ ] **Step 6: Commit**

```bash
git add openwheel-gadget/CMakeLists.txt openwheel-gadget/src/DBusListener.h openwheel-gadget/src/DBusListener.cpp openwheel-gadget/tests/test_dbuslistener.cpp openwheel-gadget/tests/CMakeLists.txt
git commit -m "gadget: add DBusListener for Rotate/Press signals"
```

---

### Task 8: DialController

**Files:**
- Create: `openwheel-gadget/src/DialController.h`, `.cpp`
- Create: `openwheel-gadget/tests/test_dialcontroller.cpp`
- Modify: `openwheel-gadget/CMakeLists.txt`, `openwheel-gadget/tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `FunctionRegistry` (Task 2), `FakeDialFunction` (Task 2, test-only).
- Produces: `class DialController : public QObject { Q_OBJECT Q_PROPERTY(bool menuOpen READ isMenuOpen NOTIFY menuOpenChanged) Q_PROPERTY(int highlightedIndex READ highlightedIndex NOTIFY highlightedIndexChanged) public: DialController(FunctionRegistry *registry, const QString &settingsPath = QString(), QObject *parent = nullptr); bool isMenuOpen() const; int highlightedIndex() const; QString activeFunctionId() const; Q_INVOKABLE int functionCount() const; Q_INVOKABLE QString displayNameAt(int index) const; Q_INVOKABLE QString iconNameAt(int index) const; Q_INVOKABLE bool isAvailableAt(int index) const; public slots: void onRotated(int direction); void onPressChanged(bool pressed); void setEnabled(bool enabled); signals: void menuOpenChanged(); void highlightedIndexChanged(); void activeFunctionChanged(); void hudRequested(const QString &iconName, const QString &valueLabel); };` — consumed by `main.cpp` (Task 11) and QML (Task 10). `menuOpen`/`highlightedIndex` are `Q_PROPERTY`s (not just getters) specifically so QML (Task 10) can bind to `dialController.menuOpen` / `dialController.highlightedIndex` directly. `isAvailableAt` is what lets the radial menu (Task 10) gray out backends that aren't available, per the spec's error-handling section.

- [ ] **Step 1: Write the failing test**

```cpp
// openwheel-gadget/tests/test_dialcontroller.cpp
#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QTemporaryFile>
#include "DialController.h"
#include "FunctionRegistry.h"
#include "FakeDialFunction.h"

class TestDialController : public QObject {
    Q_OBJECT
private slots:
    void quickRotateAdjustsActiveFunctionWithoutOpeningMenu();
    void holdingPastThresholdOpensMenu();
    void rotatingWhileMenuOpenMovesHighlight();
    void releasingWhileMenuOpenConfirmsSelection();
    void disablingClosesMenuAndIgnoresInput();

private:
    QString tempSettingsPath();
};

QString TestDialController::tempSettingsPath()
{
    static QTemporaryFile file;
    file.setAutoRemove(true);
    file.open();
    return file.fileName();
}

void TestDialController::quickRotateAdjustsActiveFunctionWithoutOpeningMenu()
{
    FunctionRegistry registry;
    FakeDialFunction volume("volume");
    registry.registerFunction(&volume);
    DialController controller(&registry, tempSettingsPath());

    controller.onRotated(1);

    QCOMPARE(volume.adjustCallCount, 1);
    QCOMPARE(volume.lastDirection, 1);
    QVERIFY(!controller.isMenuOpen());
}

void TestDialController::holdingPastThresholdOpensMenu()
{
    FunctionRegistry registry;
    FakeDialFunction volume("volume");
    registry.registerFunction(&volume);
    DialController controller(&registry, tempSettingsPath());
    QSignalSpy spy(&controller, &DialController::menuOpenChanged);

    controller.onPressChanged(true);
    QVERIFY(spy.wait(1000));
    QVERIFY(controller.isMenuOpen());
}

void TestDialController::rotatingWhileMenuOpenMovesHighlight()
{
    FunctionRegistry registry;
    FakeDialFunction volume("volume");
    FakeDialFunction scroll("scroll");
    registry.registerFunction(&volume);
    registry.registerFunction(&scroll);
    DialController controller(&registry, tempSettingsPath());

    controller.onPressChanged(true);
    QSignalSpy spy(&controller, &DialController::menuOpenChanged);
    QVERIFY(spy.wait(1000));

    controller.onRotated(1);
    QCOMPARE(controller.highlightedIndex(), 1);
    QCOMPARE(volume.adjustCallCount, 0);
    QCOMPARE(scroll.adjustCallCount, 0);
}

void TestDialController::releasingWhileMenuOpenConfirmsSelection()
{
    FunctionRegistry registry;
    FakeDialFunction volume("volume");
    FakeDialFunction scroll("scroll");
    registry.registerFunction(&volume);
    registry.registerFunction(&scroll);
    DialController controller(&registry, tempSettingsPath());

    controller.onPressChanged(true);
    QSignalSpy menuSpy(&controller, &DialController::menuOpenChanged);
    QVERIFY(menuSpy.wait(1000));

    controller.onRotated(1);
    controller.onPressChanged(false);

    QVERIFY(!controller.isMenuOpen());
    QCOMPARE(controller.activeFunctionId(), QStringLiteral("scroll"));

    controller.onRotated(1);
    QCOMPARE(scroll.adjustCallCount, 1);
    QCOMPARE(volume.adjustCallCount, 0);
}

void TestDialController::disablingClosesMenuAndIgnoresInput()
{
    FunctionRegistry registry;
    FakeDialFunction volume("volume");
    registry.registerFunction(&volume);
    DialController controller(&registry, tempSettingsPath());

    controller.onPressChanged(true);
    QSignalSpy menuSpy(&controller, &DialController::menuOpenChanged);
    QVERIFY(menuSpy.wait(1000));

    controller.setEnabled(false);
    QVERIFY(!controller.isMenuOpen());

    controller.onRotated(1);
    QCOMPARE(volume.adjustCallCount, 0);
}

QTEST_MAIN(TestDialController)
#include "test_dialcontroller.moc"
```

- [ ] **Step 2: Add test target, run to verify it fails**

Add to `openwheel-gadget/tests/CMakeLists.txt`:

```cmake
qt_add_executable(test_dialcontroller test_dialcontroller.cpp)
target_link_libraries(test_dialcontroller PRIVATE openwheel-gadget-core Qt6::Test)
add_test(NAME DialControllerTest COMMAND test_dialcontroller)
```

Run: `cd openwheel-gadget/build && cmake --build . 2>&1 | tail -20`
Expected: FAIL — `fatal error: DialController.h: No such file or directory`.

- [ ] **Step 3: Write `DialController.h` / `.cpp`**

```cpp
// openwheel-gadget/src/DialController.h
#pragma once
#include <QObject>
#include <QTimer>
#include <QSettings>
#include "FunctionRegistry.h"

class DialController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool menuOpen READ isMenuOpen NOTIFY menuOpenChanged)
    Q_PROPERTY(int highlightedIndex READ highlightedIndex NOTIFY highlightedIndexChanged)
public:
    explicit DialController(FunctionRegistry *registry, const QString &settingsPath = QString(),
                             QObject *parent = nullptr);

    bool isMenuOpen() const;
    int highlightedIndex() const;
    QString activeFunctionId() const;

    Q_INVOKABLE int functionCount() const;
    Q_INVOKABLE QString displayNameAt(int index) const;
    Q_INVOKABLE QString iconNameAt(int index) const;
    Q_INVOKABLE bool isAvailableAt(int index) const;

public slots:
    void onRotated(int direction);
    void onPressChanged(bool pressed);
    void setEnabled(bool enabled);

signals:
    void menuOpenChanged();
    void highlightedIndexChanged();
    void activeFunctionChanged();
    void hudRequested(const QString &iconName, const QString &valueLabel);

private slots:
    void onHoldTimerFired();

private:
    FunctionRegistry *m_registry;
    QTimer m_holdTimer;
    QSettings m_settings;
    bool m_pressed = false;
    bool m_menuOpen = false;
    bool m_enabled = true;
    int m_highlightedIndex = 0;
    int m_activeIndex = 0;
};
```

```cpp
// openwheel-gadget/src/DialController.cpp
#include "DialController.h"

namespace {
constexpr int kHoldThresholdMs = 400;
}

DialController::DialController(FunctionRegistry *registry, const QString &settingsPath, QObject *parent)
    : QObject(parent), m_registry(registry),
      m_settings(settingsPath.isEmpty() ? QSettings() : QSettings(settingsPath, QSettings::IniFormat))
{
    m_holdTimer.setSingleShot(true);
    m_holdTimer.setInterval(kHoldThresholdMs);
    connect(&m_holdTimer, &QTimer::timeout, this, &DialController::onHoldTimerFired);

    const QString savedId = m_settings.value(QStringLiteral("dial/activeFunction")).toString();
    const int savedIndex = m_registry->indexOf(savedId);
    if (savedIndex >= 0) {
        m_activeIndex = savedIndex;
    } else {
        const int volumeIndex = m_registry->indexOf(QStringLiteral("volume"));
        m_activeIndex = volumeIndex >= 0 ? volumeIndex : 0;
    }
}

bool DialController::isMenuOpen() const { return m_menuOpen; }
int DialController::highlightedIndex() const { return m_highlightedIndex; }

QString DialController::activeFunctionId() const
{
    DialFunction *function = m_registry->at(m_activeIndex);
    return function ? function->id() : QString();
}

int DialController::functionCount() const { return m_registry->count(); }

QString DialController::displayNameAt(int index) const
{
    DialFunction *function = m_registry->at(index);
    return function ? function->displayName() : QString();
}

QString DialController::iconNameAt(int index) const
{
    DialFunction *function = m_registry->at(index);
    return function ? function->iconName() : QString();
}

bool DialController::isAvailableAt(int index) const
{
    DialFunction *function = m_registry->at(index);
    return function && function->isAvailable();
}

void DialController::onRotated(int direction)
{
    if (!m_enabled) {
        return;
    }

    const int count = m_registry->count();
    if (count == 0) {
        return;
    }

    if (m_menuOpen) {
        m_highlightedIndex = ((m_highlightedIndex + direction) % count + count) % count;
        emit highlightedIndexChanged();
        return;
    }

    DialFunction *function = m_registry->at(m_activeIndex);
    if (function && function->isAvailable()) {
        function->adjust(direction);
        emit hudRequested(function->iconName(), function->currentValueLabel());
    }
}

void DialController::onPressChanged(bool pressed)
{
    if (!m_enabled) {
        return;
    }

    m_pressed = pressed;
    if (pressed) {
        m_holdTimer.start();
        return;
    }

    if (m_holdTimer.isActive()) {
        m_holdTimer.stop();
        return; // quick click released before hold threshold: no v1 action defined
    }

    if (m_menuOpen) {
        m_activeIndex = m_highlightedIndex;
        m_menuOpen = false;
        DialFunction *function = m_registry->at(m_activeIndex);
        if (function) {
            m_settings.setValue(QStringLiteral("dial/activeFunction"), function->id());
        }
        emit menuOpenChanged();
        emit activeFunctionChanged();
    }
}

void DialController::setEnabled(bool enabled)
{
    m_enabled = enabled;
    if (!m_enabled) {
        m_holdTimer.stop();
        if (m_menuOpen) {
            m_menuOpen = false;
            emit menuOpenChanged();
        }
    }
}

void DialController::onHoldTimerFired()
{
    m_menuOpen = true;
    m_highlightedIndex = m_activeIndex;
    emit menuOpenChanged();
    emit highlightedIndexChanged();
}
```

- [ ] **Step 4: Wire into CMakeLists.txt**

Modify `openwheel-gadget/CMakeLists.txt`'s `qt_add_library` block, adding:

```cmake
    src/DialController.h
    src/DialController.cpp
```

- [ ] **Step 5: Run tests, verify pass**

Run: `cd openwheel-gadget/build && cmake --build . && QT_QPA_PLATFORM=offscreen ctest --output-on-failure -R DialControllerTest`
Expected: `100% tests passed, 0 tests failed out of 5`.

- [ ] **Step 6: Commit**

```bash
git add openwheel-gadget/CMakeLists.txt openwheel-gadget/src/DialController.h openwheel-gadget/src/DialController.cpp openwheel-gadget/tests/test_dialcontroller.cpp openwheel-gadget/tests/CMakeLists.txt
git commit -m "gadget: add DialController state machine"
```

---

### Task 9: TrayController + TrayIcon

**Files:**
- Create: `openwheel-gadget/src/TrayController.h`, `.cpp`
- Create: `openwheel-gadget/src/TrayIcon.h`, `.cpp`
- Create: `openwheel-gadget/tests/test_traycontroller.cpp`
- Modify: `openwheel-gadget/CMakeLists.txt`, `openwheel-gadget/tests/CMakeLists.txt`

**Interfaces:**
- Produces: `class TrayController : public QObject { Q_OBJECT public: explicit TrayController(QObject *parent = nullptr); bool isEnabled() const; public slots: void toggleEnabled(); signals: void enabledChanged(bool enabled); void quitRequested(); };` and `class TrayIcon : public QObject { public: TrayIcon(TrayController *controller, QObject *parent = nullptr); void show(); void setDaemonConnected(bool connected); };` — both consumed by `main.cpp` (Task 11). `TrayController::enabledChanged` connects to `DialController::setEnabled` (Task 8), and `DBusListener::daemonConnectedChanged` (Task 7) connects to `TrayIcon::setDaemonConnected`, both in Task 11. `setDaemonConnected` is what surfaces the spec's "tray icon shows a distinct disconnected state" requirement.
- Note: `TrayIcon` wraps a real `QSystemTrayIcon`/`QMenu` and is not unit tested (no tray protocol in a headless test environment) — only `TrayController`'s toggle logic is unit tested.

- [ ] **Step 1: Write the failing test**

```cpp
// openwheel-gadget/tests/test_traycontroller.cpp
#include <QtTest/QtTest>
#include <QSignalSpy>
#include "TrayController.h"

class TestTrayController : public QObject {
    Q_OBJECT
private slots:
    void startsEnabled();
    void toggleFlipsStateAndEmits();
};

void TestTrayController::startsEnabled()
{
    TrayController controller;
    QVERIFY(controller.isEnabled());
}

void TestTrayController::toggleFlipsStateAndEmits()
{
    TrayController controller;
    QSignalSpy spy(&controller, &TrayController::enabledChanged);

    controller.toggleEnabled();

    QVERIFY(!controller.isEnabled());
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toBool(), false);

    controller.toggleEnabled();
    QVERIFY(controller.isEnabled());
}

QTEST_MAIN(TestTrayController)
#include "test_traycontroller.moc"
```

- [ ] **Step 2: Add test target, run to verify it fails**

Add to `openwheel-gadget/tests/CMakeLists.txt`:

```cmake
qt_add_executable(test_traycontroller test_traycontroller.cpp)
target_link_libraries(test_traycontroller PRIVATE openwheel-gadget-core Qt6::Test)
add_test(NAME TrayControllerTest COMMAND test_traycontroller)
```

Run: `cd openwheel-gadget/build && cmake --build . 2>&1 | tail -20`
Expected: FAIL — `fatal error: TrayController.h: No such file or directory`.

- [ ] **Step 3: Write `TrayController.h` / `.cpp`**

```cpp
// openwheel-gadget/src/TrayController.h
#pragma once
#include <QObject>

class TrayController : public QObject {
    Q_OBJECT
public:
    explicit TrayController(QObject *parent = nullptr);
    bool isEnabled() const;

public slots:
    void toggleEnabled();

signals:
    void enabledChanged(bool enabled);
    void quitRequested();

private:
    bool m_enabled = true;
};
```

```cpp
// openwheel-gadget/src/TrayController.cpp
#include "TrayController.h"

TrayController::TrayController(QObject *parent) : QObject(parent) {}

bool TrayController::isEnabled() const { return m_enabled; }

void TrayController::toggleEnabled()
{
    m_enabled = !m_enabled;
    emit enabledChanged(m_enabled);
}
```

- [ ] **Step 4: Wire into CMakeLists.txt, run tests, verify pass**

Add to `qt_add_library` block:

```cmake
    src/TrayController.h
    src/TrayController.cpp
```

Run: `cd openwheel-gadget/build && cmake --build . && QT_QPA_PLATFORM=offscreen ctest --output-on-failure -R TrayControllerTest`
Expected: `100% tests passed, 0 tests failed out of 2`.

- [ ] **Step 5: Commit the tested logic**

```bash
git add openwheel-gadget/CMakeLists.txt openwheel-gadget/src/TrayController.h openwheel-gadget/src/TrayController.cpp openwheel-gadget/tests/test_traycontroller.cpp openwheel-gadget/tests/CMakeLists.txt
git commit -m "gadget: add TrayController"
```

- [ ] **Step 6: Write `TrayIcon.h` / `.cpp` (not unit tested — real QSystemTrayIcon wiring)**

```cpp
// openwheel-gadget/src/TrayIcon.h
#pragma once
#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include "TrayController.h"

class TrayIcon : public QObject {
    Q_OBJECT
public:
    explicit TrayIcon(TrayController *controller, QObject *parent = nullptr);
    void show();
    void setDaemonConnected(bool connected);

private:
    TrayController *m_controller;
    QMenu m_menu;
    QAction *m_enabledAction;
    QAction *m_quitAction;
    QSystemTrayIcon m_icon;
};
```

```cpp
// openwheel-gadget/src/TrayIcon.cpp
#include "TrayIcon.h"
#include <QIcon>

TrayIcon::TrayIcon(TrayController *controller, QObject *parent)
    : QObject(parent), m_controller(controller)
{
    m_enabledAction = m_menu.addAction(QStringLiteral("Enabled"));
    m_enabledAction->setCheckable(true);
    m_enabledAction->setChecked(m_controller->isEnabled());
    connect(m_enabledAction, &QAction::triggered, m_controller, &TrayController::toggleEnabled);
    connect(m_controller, &TrayController::enabledChanged, m_enabledAction, &QAction::setChecked);

    m_quitAction = m_menu.addAction(QStringLiteral("Quit"));
    connect(m_quitAction, &QAction::triggered, m_controller, &TrayController::quitRequested);

    m_icon.setIcon(QIcon::fromTheme(QStringLiteral("input-dialpad")));
    m_icon.setToolTip(QStringLiteral("Asus Dial"));
    m_icon.setContextMenu(&m_menu);
}

void TrayIcon::show() { m_icon.show(); }

void TrayIcon::setDaemonConnected(bool connected)
{
    m_icon.setIcon(QIcon::fromTheme(connected ? QStringLiteral("input-dialpad")
                                               : QStringLiteral("dialog-error")));
    m_icon.setToolTip(connected ? QStringLiteral("Asus Dial")
                                : QStringLiteral("Asus Dial (daemon disconnected)"));
}
```

- [ ] **Step 7: Wire into CMakeLists.txt**

Add to `qt_add_library` block:

```cmake
    src/TrayIcon.h
    src/TrayIcon.cpp
```

Run: `cd openwheel-gadget/build && cmake --build .`
Expected: builds successfully.

- [ ] **Step 8: Commit**

```bash
git add openwheel-gadget/CMakeLists.txt openwheel-gadget/src/TrayIcon.h openwheel-gadget/src/TrayIcon.cpp
git commit -m "gadget: add TrayIcon"
```

---

### Task 10: QML overlay (radial menu + HUD)

**Files:**
- Modify: `openwheel-gadget/qml/DialOverlay.qml`
- Create: `openwheel-gadget/qml/RadialMenu.qml`
- Create: `openwheel-gadget/qml/Hud.qml`
- Modify: `openwheel-gadget/CMakeLists.txt` (add new QML files to `qt_add_qml_module`'s `QML_FILES`)

**Interfaces:**
- Consumes: a context property named `dialController` of type `DialController*` (wired in Task 11), specifically `menuOpen` (bool property), `highlightedIndex` (int property), `hudRequested(iconName, valueLabel)` (signal), `functionCount()`, `displayNameAt(index)`, `iconNameAt(index)` (invokable methods) — all already implemented in Task 8.
- Note: no automated test — QML visuals are verified manually per the design's testing section. This task's "test" step is a manual launch checklist.
- **Decision (made during implementation):** `iconNameAt(index)`/`hudRequested`'s `iconName` are captured but deliberately **not rendered** in v1 — QML has no built-in way to load freedesktop icon-theme names (e.g. `"audio-volume-high"`) directly, and building that (a custom `QQuickImageProvider` wrapping `QIcon::fromTheme`, or bundling SVG assets) is new architecture outside this plan's scope. The radial menu and HUD are text-label-only for v1; this is a known, accepted limitation, not a defect — a follow-up task can add a `QQuickImageProvider` later without touching any already-built code, since `iconNameAt`/`iconName` are already fully wired through and just need a renderer.

- [ ] **Step 1: Write `RadialMenu.qml`**

```qml
// openwheel-gadget/qml/RadialMenu.qml
import QtQuick

Item {
    id: root
    readonly property int count: dialController.functionCount()

    Repeater {
        model: root.count
        delegate: Rectangle {
            required property int index
            width: 64
            height: 64
            radius: width / 2
            color: index === dialController.highlightedIndex ? "#3daee9" : "#444444"
            opacity: dialController.isAvailableAt(index) ? 1.0 : 0.35
            x: root.width / 2
               + (root.width / 2 - 40) * Math.cos(2 * Math.PI * index / root.count - Math.PI / 2)
               - width / 2
            y: root.height / 2
               + (root.height / 2 - 40) * Math.sin(2 * Math.PI * index / root.count - Math.PI / 2)
               - height / 2

            Text {
                anchors.centerIn: parent
                text: dialController.displayNameAt(index)
                color: "white"
                font.pixelSize: 10
                wrapMode: Text.WordWrap
                width: parent.width - 8
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
}
```

- [ ] **Step 2: Write `Hud.qml`**

```qml
// openwheel-gadget/qml/Hud.qml
import QtQuick

Item {
    id: root
    property string iconName: ""
    property string valueLabel: ""

    Rectangle {
        anchors.centerIn: parent
        width: 160
        height: 60
        radius: 12
        color: "#222222"
        opacity: 0.85

        Text {
            anchors.centerIn: parent
            text: root.valueLabel
            color: "white"
            font.pixelSize: 20
        }
    }
}
```

- [ ] **Step 3: Rewrite `DialOverlay.qml`**

```qml
// openwheel-gadget/qml/DialOverlay.qml
import QtQuick
import QtQuick.Window

Window {
    id: overlayWindow
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool
    color: "transparent"
    width: 320
    height: 320
    visible: dialController.menuOpen || hudTimer.running
    x: (Screen.width - width) / 2
    y: (Screen.height - height) / 2

    RadialMenu {
        anchors.fill: parent
        visible: dialController.menuOpen
    }

    Hud {
        id: hud
        anchors.fill: parent
        visible: !dialController.menuOpen && hudTimer.running
    }

    Timer {
        id: hudTimer
        interval: 1500
    }

    Connections {
        target: dialController
        function onHudRequested(iconName, valueLabel) {
            hud.iconName = iconName
            hud.valueLabel = valueLabel
            hudTimer.restart()
        }
    }
}
```

- [ ] **Step 4: Wire new QML files into CMakeLists.txt**

Modify `openwheel-gadget/CMakeLists.txt`'s `qt_add_qml_module` block:

```cmake
qt_add_qml_module(openwheel-gadget
    URI OpenWheelGadget
    VERSION 1.0
    QML_FILES
        qml/DialOverlay.qml
        qml/RadialMenu.qml
        qml/Hud.qml
)
```

- [ ] **Step 5: Build**

Run: `cd openwheel-gadget/build && cmake --build . 2>&1 | tail -20`
Expected: builds successfully (this task's `main.cpp` still doesn't set `dialController` as a context property yet, so runtime will show a QML `ReferenceError` — that's expected until Task 11 wires it up; a clean build is the only thing verified here).

- [ ] **Step 6: Commit**

```bash
git add openwheel-gadget/CMakeLists.txt openwheel-gadget/qml/DialOverlay.qml openwheel-gadget/qml/RadialMenu.qml openwheel-gadget/qml/Hud.qml
git commit -m "gadget: add radial menu and HUD QML overlay"
```

---

### Task 11: Wire everything together in main.cpp + end-to-end manual test

**Files:**
- Modify: `openwheel-gadget/src/main.cpp`

**Interfaces:**
- Consumes: every class produced by Tasks 2–10 (`FunctionRegistry`, `QProcessRunner`, `QtDBusCaller`, `resolveLogindSessionPath`, `readBacklightInfo`, `VolumeFunction`, `BrightnessFunction`, `MediaFunction`, `ScrollFunction`/`createScrollBackend`, `DBusListener`, `DialController`, `TrayController`, `TrayIcon`).

- [ ] **Step 1: Rewrite `main.cpp`**

```cpp
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
    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/OpenWheelGadget/DialOverlay.qml")));
    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
```

- [ ] **Step 2: Build**

Run: `cd openwheel-gadget/build && cmake --build . 2>&1 | tail -20`
Expected: builds successfully.

- [ ] **Step 3: Run the full test suite**

Run: `cd openwheel-gadget/build && QT_QPA_PLATFORM=offscreen ctest --output-on-failure`
Expected: all tests pass (run `dbus-run-session -- ctest --output-on-failure` instead if there's no active session bus).

- [ ] **Step 4: Manual end-to-end verification**

With the daemon **not required to be running** (the gadget only needs `org.asus.dial` signals, which can be injected directly), run:

```bash
cd openwheel-gadget/build
./openwheel-gadget &
sleep 1
dbus-send --session --type=signal /org/asus/dial org.asus.dial.Rotate int32:1
```
Expected: a HUD briefly appears showing the volume icon/level, and (if PulseAudio/PipeWire is running) system volume actually increases by 5%.

```bash
dbus-send --session --type=signal /org/asus/dial org.asus.dial.Press int32:1
sleep 1
dbus-send --session --type=signal /org/asus/dial org.asus.dial.Rotate int32:1
dbus-send --session --type=signal /org/asus/dial org.asus.dial.Press int32:0
```
Expected: after the ~400ms hold, the radial menu appears with 4 entries; the rotate moves the highlight to "Scroll"; the release closes the menu and Scroll becomes the active function (verify with another `Rotate` signal — it should scroll the focused window instead of changing volume).

Check the tray icon appears (in whatever tray applet the desktop provides) and that its "Enabled" checkbox toggling actually suppresses further `Rotate`/`Press` effects, and that "Quit" exits the process.

Kill the gadget when done: `kill %1`.

Since this depends on real system audio/brightness/media state and the actual desktop tray implementation, this step **cannot be fully automated** — record the result of this manual pass in the task notes rather than treating `ctest` alone as proof the feature works end-to-end.

- [ ] **Step 5: Commit**

```bash
git add openwheel-gadget/src/main.cpp
git commit -m "gadget: wire DBusListener, DialController, functions, and tray together in main.cpp"
```

---

### Task 12: Autostart + README documentation

**Files:**
- Create: `openwheel-gadget/openwheel-gadget.desktop`
- Modify: `README.md`

**Interfaces:**
- None (packaging/docs only).

- [ ] **Step 1: Write the autostart desktop entry**

```ini
# openwheel-gadget/openwheel-gadget.desktop
[Desktop Entry]
Type=Application
Name=Asus Dial Gadget
Comment=Tray icon and on-screen overlay for the Asus Dial
Exec=openwheel-gadget
Icon=input-dialpad
Terminal=false
Categories=Utility;
X-GNOME-Autostart-enabled=true
```

- [ ] **Step 2: Document build/run/setup steps in the README**

Read the current `README.md` first, then append a new section (use the Edit tool to insert after the existing content, not the Write tool, so the existing description/attribution lines are preserved):

```markdown

## openwheel-gadget (tray + overlay)

Build dependencies (Debian/Ubuntu naming): `qt6-base-dev qt6-declarative-dev libqt6svg6-dev libxtst-dev`.

Runtime dependencies (also needed to actually run the built binary, not just compile it):
`qml6-module-qtquick qml6-module-qtquick-window qml6-module-qtqml-workerscript`. Without these,
the binary builds and links fine but exits immediately with "module ... is not installed" QML
errors as soon as it tries to load `DialOverlay.qml`.

```bash
cd openwheel-gadget
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

Run `./openwheel-gadget` (requires `openwheel-daemon`'s `asus_wheel` running and emitting D-Bus
signals for the dial to actually do anything; the gadget also works with signals sent manually via
`dbus-send`, useful for testing without the physical hardware — see the daemon's D-Bus interface
above).

To start automatically with your session, copy `openwheel-gadget/openwheel-gadget.desktop` to
`~/.config/autostart/`.

**Wayland scroll support (optional):** the Scroll dial function uses X11's XTest extension by
default and works out of the box on any X11 session. On Wayland, scroll instead uses a `uinput`
virtual device, which requires one-time setup: add your user to the `input` group
(`sudo usermod -aG input $USER`, then log out and back in) so the gadget can open `/dev/uinput`
without root. If this isn't set up, the Scroll entry in the radial menu is simply disabled — every
other function works normally on Wayland regardless.
```

- [ ] **Step 3: Verify the desktop file is syntactically valid (if `desktop-file-validate` is available)**

Run: `desktop-file-validate openwheel-gadget/openwheel-gadget.desktop || true`
Expected: no errors (the `|| true` is only there because this tool may not be installed; a missing tool isn't a failure of this task).

- [ ] **Step 4: Commit**

```bash
git add openwheel-gadget/openwheel-gadget.desktop README.md
git commit -m "gadget: add autostart entry and build/run documentation"
```

---

## Post-plan follow-up (not part of this plan)

Update the project memory notes once this plan is executed: `openwheel-gadget` is no longer an
empty stub, and the daemon rough edges (`/dev/hidraw2` hardcoding, dead `find_hidraw_device()`,
unused `rotation_lb`) are still open for a future daemon-focused plan.
