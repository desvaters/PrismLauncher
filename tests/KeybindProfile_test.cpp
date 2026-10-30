#include <QTest>

#include <minecraft/KeybindProfile.h>

using KeybindProfile::InputBackend;

class KeybindProfileTest : public QObject {
    Q_OBJECT
   private:
    KeybindProfile::Profile profile() const
    {
        return {
            { "key.togglePerspective", "key.keyboard.r" },
            { "key.swapOffhand", "key.keyboard.g" },
            { "key.attack", "key.mouse.4" },
            { "key.drop", "key.keyboard.keypad.period" },
        };
    }

   private slots:
    void test_buildOptions_data()
    {
        QTest::addColumn<int>("backend");
        QTest::addColumn<int>("dataVersion");
        QTest::addColumn<QString>("expected");

        QTest::newRow("sdl") << static_cast<int>(InputBackend::Sdl) << 5023
                             << "version:5023\n"
                                "key_key.attack:key.mouse.4\n"
                                "key_key.drop:key.keyboard.keypad.period\n"
                                "key_key.swapOffhand:key.keyboard.g\n"
                                "key_key.togglePerspective:key.keyboard.r\n";
        QTest::newRow("glfw after swapOffhand rename") << static_cast<int>(InputBackend::Glfw) << 3955
                                                       << "version:3955\n"
                                                          "key_key.attack:key.mouse.4\n"
                                                          "key_key.drop:key.keyboard.keypad.decimal\n"
                                                          "key_key.swapOffhand:key.keyboard.g\n"
                                                          "key_key.togglePerspective:key.keyboard.r\n";
        QTest::newRow("glfw before swapOffhand rename") << static_cast<int>(InputBackend::Glfw) << 1631
                                                        << "version:1631\n"
                                                           "key_key.attack:key.mouse.4\n"
                                                           "key_key.drop:key.keyboard.keypad.decimal\n"
                                                           "key_key.swapHands:key.keyboard.g\n"
                                                           "key_key.togglePerspective:key.keyboard.r\n";
        QTest::newRow("lwjgl2") << static_cast<int>(InputBackend::Lwjgl2) << 0
                                << "key_key.attack:-97\n"
                                   "key_key.drop:83\n"
                                   "key_key.swapHands:34\n"
                                   "key_key.togglePerspective:19\n";
    }
    void test_buildOptions()
    {
        QFETCH(int, backend);
        QFETCH(int, dataVersion);
        QFETCH(QString, expected);

        QCOMPARE(KeybindProfile::buildOptions(profile(), static_cast<InputBackend>(backend), dataVersion), expected);
    }

    void test_buildOptions_skipsUnsupported()
    {
        const KeybindProfile::Profile profile = {
            { "key.chat", "key.keyboard.media.play" },
            { "key.jump", "key.keyboard.space" },
        };
        QStringList skipped;
        QCOMPARE(KeybindProfile::buildOptions(profile, InputBackend::Lwjgl2, 0, &skipped), QString("key_key.jump:57\n"));
        QCOMPARE(skipped, QStringList{ "key.chat" });
    }

    void test_keyDisplayName()
    {
        QCOMPARE(KeybindProfile::keyDisplayName("key.keyboard.left.shift"), QString("Left Shift"));
        QCOMPARE(KeybindProfile::keyDisplayName("key.keyboard.r"), QString("R"));
        QCOMPARE(KeybindProfile::keyDisplayName("key.mouse.4"), QString("Mouse Button 4"));
        QCOMPARE(KeybindProfile::keyDisplayName("key.keyboard.unknown"), QString("Not Bound"));
    }
};

QTEST_GUILESS_MAIN(KeybindProfileTest)

#include "KeybindProfile_test.moc"
