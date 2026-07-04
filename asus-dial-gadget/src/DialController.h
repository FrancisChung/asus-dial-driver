#pragma once
#include <QObject>
#include <QTimer>
#include <QSettings>
#include "FunctionRegistry.h"
#include "RotateDispatcher.h"

class DialController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool menuOpen READ isMenuOpen NOTIFY menuOpenChanged)
    Q_PROPERTY(int highlightedIndex READ highlightedIndex NOTIFY highlightedIndexChanged)
public:
    explicit DialController(FunctionRegistry *registry, RotateDispatcher *dispatcher,
                             const QString &settingsPath = QString(), QObject *parent = nullptr);

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
    RotateDispatcher *m_dispatcher;
    QTimer m_holdTimer;
    QSettings m_settings;
    bool m_pressed = false;
    bool m_menuOpen = false;
    bool m_enabled = true;
    int m_highlightedIndex = 0;
    int m_activeIndex = 0;
};
