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
