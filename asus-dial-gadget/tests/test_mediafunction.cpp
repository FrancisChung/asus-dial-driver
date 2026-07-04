#include <QtTest/QtTest>
#include "functions/MediaFunction.h"
#include "FakeDBusCaller.h"

class TestMediaFunction : public QObject {
    Q_OBJECT
private slots:
    void isAvailableFindsMprisPlayer();
    void isAvailableFalseWhenNoPlayer();
    void adjustSeeksFiveSecondsInMicroseconds();
    void currentValueLabelIsJustMediaRegardlessOfPlayer();
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

void TestMediaFunction::currentValueLabelIsJustMediaRegardlessOfPlayer()
{
    // The MPRIS bus name is an internal D-Bus identifier, not a friendly
    // player name — e.g. Chromium registers as
    // "org.mpris.MediaPlayer2.chromium.instance6005", which used to leak
    // straight into the on-screen HUD as "Media: chromium.instance6005".
    // There's no per-player display name to show instead, so the label is
    // just "Media" regardless of which player is active.
    FakeDBusCaller caller;
    caller.nextReplyArgs = {QStringList{"org.mpris.MediaPlayer2.chromium.instance6005"}};
    MediaFunction media(&caller);

    QCOMPARE(media.currentValueLabel(), QStringLiteral("Media"));
}

QTEST_MAIN(TestMediaFunction)
#include "test_mediafunction.moc"
