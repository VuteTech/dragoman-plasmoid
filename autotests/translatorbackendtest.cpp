/*
    SPDX-FileCopyrightText: 2026 Blagovest Petrov <blagovest@petrovs.info>
    SPDX-FileCopyrightText: 2026 Vute Tech Ltd. <https://vute.tech>
    SPDX-License-Identifier: GPL-3.0-or-later
*/

// The widget's translation logic against the fake daemon.

#include "translatorbackend.h"
#include "fakedaemon.h"

#include <QSignalSpy>

#include <memory>

using namespace Qt::StringLiterals;

class TranslatorBackendTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        // A fake krakoman, found through PATH, that records its arguments.
        QVERIFY(m_bin.isValid());
        const QString script = m_bin.filePath(u"krakoman"_s);
        QFile file(script);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("#!/bin/sh\nprintf '%s\\n' \"$@\" > \"$0.args.tmp\" && mv \"$0.args.tmp\" \"$0.args\"\n");
        file.close();
        QVERIFY(file.setPermissions(file.permissions() | QFileDevice::ExeOwner));
        qputenv("PATH", QByteArray(m_bin.path().toLocal8Bit() + ':' + qgetenv("PATH")));

        m_daemon.start();
        if (QTest::currentTestFailed() || QTest::currentTestResolved()) {
            return; // skipped without dbus-daemon, or failed
        }
    }

    void cleanupTestCase()
    {
        m_daemon.stop();
    }

    void init()
    {
        m_backend = std::make_unique<TranslatorBackend>();
        m_backend->setLiveTranslation(false);
        m_backend->setSourceLanguage(u"bg"_s);
        m_backend->setTargetLanguage(u"en"_s);
        m_daemon.translator->batchSizes.clear();
        m_daemon.translator->installed = {u"bg-en"_s};
        m_daemon.translator->failWith.clear();
        m_daemon.translator->hold = false;
    }

    void cleanup()
    {
        m_backend.reset();
    }

    void segmentLines()
    {
        QCOMPARE(TranslatorBackend::segmentLines({u"one"_s, u""_s, u"  "_s, u"- - -"_s, u"2"_s}), QList<qsizetype>({0, 4}));
    }

    void listsLanguages()
    {
        QSignalSpy changed(m_backend.get(), &TranslatorBackend::pairsChanged);
        QVERIFY(!m_backend->pairsLoaded());
        m_backend->refreshPairs();
        QTRY_VERIFY(m_backend->pairsLoaded());
        QVERIFY(m_backend->pairsError().isEmpty());
        QStringList codes;
        for (const QVariant &language : m_backend->languages()) {
            const QVariantMap map = language.toMap();
            codes.append(map.value(u"code"_s).toString());
            QVERIFY(!map.value(u"name"_s).toString().isEmpty());
        }
        codes.sort();
        QCOMPARE(codes, QStringList({u"bg"_s, u"de"_s, u"en"_s}));
    }

    void keepsBlankLines()
    {
        m_backend->setSourceText(u"първи ред\n\n  \nвтори ред"_s);
        m_backend->translate();
        QTRY_VERIFY(!m_backend->isBusy());
        QVERIFY2(m_backend->errorText().isEmpty(), qPrintable(m_backend->errorText()));
        QCOMPARE(m_backend->translatedText(), u"ПЪРВИ РЕД\n\n  \nВТОРИ РЕД"_s);
        QCOMPARE(m_daemon.translator->batchSizes, QList<qsizetype>({2}));
        QVERIFY(m_backend->directionNotice().isEmpty());
    }

    void splitsLongTextIntoBatches()
    {
        QStringList lines;
        for (int i = 0; i < 300; ++i) {
            lines.append(u"ред %1"_s.arg(i));
        }
        m_backend->setSourceText(lines.join(u'\n'));
        m_backend->translate();
        QTRY_VERIFY(!m_backend->isBusy());
        QCOMPARE(m_daemon.translator->batchSizes, QList<qsizetype>({128, 128, 44}));
        QCOMPARE(m_backend->translatedText(), lines.join(u'\n').toUpper());
    }

    void liveTranslationAfterAPause()
    {
        m_backend->setLiveTranslation(true);
        m_backend->setSourceText(u"добър ден"_s);
        QVERIFY(m_backend->translatedText().isEmpty()); // not before the pause
        QTRY_COMPARE(m_backend->translatedText(), u"ДОБЪР ДЕН"_s);

        m_backend->setSourceText(QString());
        QVERIFY(m_backend->translatedText().isEmpty()); // cleared at once
    }

    void swapsDirectionForTextInTargetLanguage()
    {
        m_backend->setSourceLanguage(u"en"_s);
        m_backend->setTargetLanguage(u"bg"_s);
        QSignalSpy languages(m_backend.get(), &TranslatorBackend::languagesChanged);
        m_backend->setSourceText(u"здравей"_s);
        m_backend->translate();
        QTRY_COMPARE(m_backend->translatedText(), u"ЗДРАВЕЙ"_s);
        QCOMPARE(m_backend->sourceLanguage(), u"bg"_s);
        QCOMPARE(m_backend->targetLanguage(), u"en"_s);
        QCOMPARE(languages.count(), 1);
        QVERIFY(!m_backend->directionNotice().isEmpty());

        // Swapping back keeps the chosen direction for this text.
        m_daemon.translator->installed.insert(u"en-bg"_s);
        m_backend->revertDirectionSwap();
        QCOMPARE(m_backend->sourceLanguage(), u"en"_s);
        QCOMPARE(m_backend->targetLanguage(), u"bg"_s);
        QVERIFY(m_backend->directionNotice().isEmpty());
        QTRY_VERIFY(!m_backend->isBusy());
        QCOMPARE(m_backend->sourceLanguage(), u"en"_s);
        QCOMPARE(m_daemon.translator->batchSizes, QList<qsizetype>({1, 1}));
    }

    void keepsDirectionWhenDetectionIsOff()
    {
        m_backend->setDetectDirection(false);
        m_backend->setSourceLanguage(u"en"_s);
        m_backend->setTargetLanguage(u"bg"_s);
        m_backend->setSourceText(u"здравей"_s);
        m_backend->translate();
        QTRY_VERIFY(!m_backend->isBusy());
        QCOMPARE(m_backend->sourceLanguage(), u"en"_s);
        QVERIFY(m_backend->missingPair()); // en-bg is not installed in the fake
    }

    void reportsMissingPairWithoutDownloading()
    {
        m_backend->setDetectDirection(false);
        m_backend->setSourceLanguage(u"de"_s);
        m_backend->setLiveTranslation(true);
        const int prepareBefore = m_daemon.translator->prepareCalls;
        m_backend->setSourceText(u"guten Tag"_s);
        QTRY_VERIFY(m_backend->missingPair());
        QVERIFY(m_backend->translatedText().isEmpty());

        // Neither live translation nor an explicit translation downloads.
        m_backend->translate();
        QTRY_VERIFY(!m_backend->isBusy());
        QVERIFY(m_backend->missingPair());
        QCOMPARE(m_daemon.translator->prepareCalls, prepareBefore);
        QVERIFY(!m_daemon.translator->installed.contains(u"de-en"_s));
    }

    void installsExplicitly()
    {
        m_backend->setDetectDirection(false);
        m_backend->setSourceLanguage(u"de"_s);
        m_backend->setSourceText(u"guten Tag"_s);
        m_backend->translate();
        QTRY_VERIFY(m_backend->missingPair());

        const int prepareBefore = m_daemon.translator->prepareCalls;
        QSignalSpy installed(m_backend.get(), &TranslatorBackend::pairInstalled);
        bool sawInstalling = false;
        connect(m_backend.get(), &TranslatorBackend::stateChanged, this, [&] {
            sawInstalling = sawInstalling || m_backend->isInstalling();
        });
        m_backend->installPair();
        QVERIFY(m_backend->isInstalling());
        QVERIFY(!m_backend->missingPair());
        QTRY_COMPARE(installed.count(), 1);
        QVERIFY(sawInstalling);
        QVERIFY(!m_backend->isInstalling());
        QCOMPARE(m_daemon.translator->prepareCalls, prepareBefore + 1);
        QCOMPARE(m_backend->translatedText(), u"GUTEN TAG"_s);
        QVERIFY(m_daemon.translator->installed.contains(u"de-en"_s));
    }

    void cancels()
    {
        m_daemon.translator->hold = true;
        const int cancelsBefore = m_daemon.translator->cancelCalls;
        m_backend->setSourceText(u"текст"_s);
        m_backend->translate();
        QTRY_COMPARE(m_daemon.translator->batchSizes.size(), 1); // the request is held
        QVERIFY(m_backend->isBusy());
        m_backend->cancel();
        QVERIFY(!m_backend->isBusy());
        QTRY_COMPARE(m_daemon.translator->cancelCalls, cancelsBefore + 1);
        QVERIFY(m_backend->errorText().isEmpty());
        QVERIFY(m_backend->translatedText().isEmpty());
        QVERIFY(!m_backend->missingPair());
    }

    void cancelledInstallStaysMissing()
    {
        m_daemon.translator->hold = true;
        m_backend->setSourceText(u"текст"_s);
        m_backend->installPair();
        QTRY_COMPARE(m_daemon.translator->batchSizes.size(), 1);
        QVERIFY(m_backend->isInstalling());
        m_backend->cancel();
        QVERIFY(!m_backend->isBusy());
        QVERIFY(!m_backend->isInstalling());
        QVERIFY(m_backend->missingPair());
    }

    void choosingTheOtherLanguageSwaps()
    {
        m_backend->setTargetLanguage(u"bg"_s);
        QCOMPARE(m_backend->sourceLanguage(), u"en"_s);
        QCOMPARE(m_backend->targetLanguage(), u"bg"_s);
        m_backend->setSourceLanguage(u"bg"_s);
        QCOMPARE(m_backend->sourceLanguage(), u"bg"_s);
        QCOMPARE(m_backend->targetLanguage(), u"en"_s);
    }

    void swapMovesTranslationToSource()
    {
        m_backend->setSourceText(u"здравей"_s);
        m_backend->translate();
        QTRY_COMPARE(m_backend->translatedText(), u"ЗДРАВЕЙ"_s);
        m_backend->swapLanguages();
        QCOMPARE(m_backend->sourceLanguage(), u"en"_s);
        QCOMPARE(m_backend->targetLanguage(), u"bg"_s);
        QCOMPARE(m_backend->sourceText(), u"ЗДРАВЕЙ"_s);
        QVERIFY(m_backend->translatedText().isEmpty());
    }

    void reportsErrors()
    {
        m_daemon.translator->failWith = u"the engine failed"_s;
        m_backend->setSourceText(u"текст"_s);
        m_backend->translate();
        QTRY_VERIFY(!m_backend->isBusy());
        QCOMPARE(m_backend->errorText(), u"the engine failed"_s);
        QVERIFY(!m_backend->daemonMissing());
        QVERIFY(!m_backend->missingPair());
    }

    void reportsMissingDaemon()
    {
        QVERIFY(m_daemon.setRegistered(false));
        m_backend->setSourceText(u"текст"_s);
        m_backend->translate();
        QTRY_VERIFY(!m_backend->isBusy());
        m_backend->refreshPairs();
        QTRY_VERIFY(!m_backend->pairsError().isEmpty());
        QVERIFY(m_daemon.setRegistered(true));
        QVERIFY(m_backend->daemonMissing());
        QVERIFY(!m_backend->missingPair());
        QVERIFY(!m_backend->pairsLoaded());

        // Back again once the daemon answers.
        m_backend->translate();
        QTRY_COMPARE(m_backend->translatedText(), u"ТЕКСТ"_s);
        QVERIFY(!m_backend->daemonMissing());
    }

    void krakomanArguments()
    {
        QCOMPARE(TranslatorBackend::krakomanArguments(u"bg"_s, u"en"_s, u"-a text"_s),
                 QStringList({u"--source"_s, u"bg"_s, u"--target"_s, u"en"_s, u"--"_s, u"-a text"_s}));
        QCOMPARE(TranslatorBackend::krakomanArguments(u"bg"_s, u"en"_s, u"  "_s), QStringList({u"--source"_s, u"bg"_s, u"--target"_s, u"en"_s}));
    }

    void opensKrakoman()
    {
        QVERIFY(m_backend->krakomanAvailable());
        m_backend->setSourceText(u"ред\nдруг ред"_s);
        m_backend->openInKrakoman();
        QFile args(m_bin.filePath(u"krakoman.args"_s));
        QTRY_VERIFY(args.exists());
        QVERIFY(args.open(QIODevice::ReadOnly));
        QCOMPARE(QString::fromUtf8(args.readAll()), u"--source\nbg\n--target\nen\n--\nред\nдруг ред\n"_s);
    }

private:
    Fake::Daemon m_daemon;
    QTemporaryDir m_bin;
    std::unique_ptr<TranslatorBackend> m_backend;
};

QTEST_GUILESS_MAIN(TranslatorBackendTest)

#include "translatorbackendtest.moc"
