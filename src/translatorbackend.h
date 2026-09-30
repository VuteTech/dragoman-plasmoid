/*
    SPDX-FileCopyrightText: 2026 Blagovest Petrov <blagovest@petrovs.info>
    SPDX-FileCopyrightText: 2026 Vute Tech Ltd. <https://vute.tech>
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include "dragomanclient.h"

#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QVariantList>
#include <qqmlintegration.h>

/**
 * The state behind the widget's popup: the chosen languages, the source
 * text and its translation, and the request in flight. It knows nothing of
 * Plasma; the QML side stores the settings in the applet configuration.
 *
 * Before translating, the text's language is detected among the two
 * chosen ones; when it is reliably the target language, the languages are
 * swapped (unless the user chose them for this text). A missing pair is
 * only reported (missingPair): it is downloaded by installPair() alone,
 * never by typing or by translate().
 *
 * Nothing talks to the daemon until the first call that needs it, so a
 * widget sitting in the panel never starts the daemon by itself.
 */
class TranslatorBackend : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("The applet provides the translator backend.")

    Q_PROPERTY(QString sourceLanguage READ sourceLanguage WRITE setSourceLanguage NOTIFY languagesChanged)
    Q_PROPERTY(QString targetLanguage READ targetLanguage WRITE setTargetLanguage NOTIFY languagesChanged)
    /// Every language the daemon knows, as {code, name} maps sorted by name.
    Q_PROPERTY(QVariantList languages READ languages NOTIFY pairsChanged)
    Q_PROPERTY(bool pairsLoaded READ pairsLoaded NOTIFY pairsChanged)
    /// Why the language list could not be fetched, or empty.
    Q_PROPERTY(QString pairsError READ pairsError NOTIFY pairsChanged)

    Q_PROPERTY(QString sourceText READ sourceText WRITE setSourceText NOTIFY sourceTextChanged)
    Q_PROPERTY(QString translatedText READ translatedText NOTIFY translatedTextChanged)
    Q_PROPERTY(bool liveTranslation READ liveTranslation WRITE setLiveTranslation NOTIFY liveTranslationChanged)
    Q_PROPERTY(bool detectDirection READ detectDirection WRITE setDetectDirection NOTIFY detectDirectionChanged)

    Q_PROPERTY(bool busy READ isBusy NOTIFY stateChanged)
    /// Whether a missing pair is being downloaded and loaded.
    Q_PROPERTY(bool installing READ isInstalling NOTIFY stateChanged)
    /// Installation progress in [0, 1], or -1 while unknown.
    Q_PROPERTY(double progress READ progress NOTIFY stateChanged)
    Q_PROPERTY(QString stage READ stage NOTIFY stateChanged)
    /// The language the last translation pivoted through, if any.
    Q_PROPERTY(QString pivot READ pivot NOTIFY stateChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY stateChanged)
    /// The daemon is not running and could not be started.
    Q_PROPERTY(bool daemonMissing READ daemonMissing NOTIFY stateChanged)
    /// The chosen pair is not installed; installPair() downloads it.
    Q_PROPERTY(bool missingPair READ missingPair NOTIFY stateChanged)
    /// Says that the languages were swapped to match the text, or empty.
    Q_PROPERTY(QString directionNotice READ directionNotice NOTIFY stateChanged)

    Q_PROPERTY(bool krakomanAvailable READ krakomanAvailable CONSTANT)

public:
    explicit TranslatorBackend(QObject *parent = nullptr);
    ~TranslatorBackend() override;

    [[nodiscard]] QString sourceLanguage() const
    {
        return m_source;
    }
    [[nodiscard]] QString targetLanguage() const
    {
        return m_target;
    }
    [[nodiscard]] QVariantList languages() const;
    [[nodiscard]] bool pairsLoaded() const
    {
        return m_pairsLoaded;
    }
    [[nodiscard]] QString pairsError() const
    {
        return m_pairsError;
    }
    [[nodiscard]] QString sourceText() const
    {
        return m_sourceText;
    }
    [[nodiscard]] QString translatedText() const
    {
        return m_translatedText;
    }
    [[nodiscard]] bool liveTranslation() const
    {
        return m_live;
    }
    [[nodiscard]] bool detectDirection() const
    {
        return m_detectDirection;
    }
    [[nodiscard]] bool isBusy() const
    {
        return m_job != nullptr || m_detecting;
    }
    [[nodiscard]] bool isInstalling() const
    {
        return m_installing;
    }
    [[nodiscard]] double progress() const
    {
        return m_progress;
    }
    [[nodiscard]] QString stage() const
    {
        return m_stage;
    }
    [[nodiscard]] QString pivot() const
    {
        return m_pivot;
    }
    [[nodiscard]] QString errorText() const
    {
        return m_errorText;
    }
    [[nodiscard]] bool daemonMissing() const
    {
        return m_daemonMissing;
    }
    [[nodiscard]] bool missingPair() const
    {
        return m_missingPair;
    }
    [[nodiscard]] QString directionNotice() const
    {
        return m_directionNotice;
    }
    [[nodiscard]] bool krakomanAvailable() const
    {
        return !m_krakoman.isEmpty();
    }

    /// Choosing the other side's language swaps the two.
    void setSourceLanguage(const QString &code);
    void setTargetLanguage(const QString &code);
    void setSourceText(const QString &text);
    void setLiveTranslation(bool live);
    void setDetectDirection(bool detect);

    /// Fetches the language list from the daemon.
    Q_INVOKABLE void refreshPairs();
    /// Translates now. A missing pair is reported, not installed.
    Q_INVOKABLE void translate();
    /// Downloads and loads the missing pair, then translates the text.
    Q_INVOKABLE void installPair();
    /// Swaps the languages; the translation becomes the new source text.
    Q_INVOKABLE void swapLanguages();
    /// Stops the translation or installation in flight.
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void copyTranslation() const;
    /// Undoes the swap that directionNotice reports, and keeps the
    /// languages for this text.
    Q_INVOKABLE void revertDirectionSwap();
    /// Opens the text in Krakoman, the full translation application.
    Q_INVOKABLE void openInKrakoman() const;
    /// The name of a language in the user's language.
    Q_INVOKABLE QString languageName(const QString &code) const;

    /// Splits @p lines into the ones to translate: every line with a letter
    /// or digit in it. Public for the tests.
    [[nodiscard]] static QList<qsizetype> segmentLines(const QStringList &lines);
    /// Krakoman's command line for the text and the languages.
    [[nodiscard]] static QStringList krakomanArguments(const QString &source, const QString &target, const QString &text);

Q_SIGNALS:
    void languagesChanged();
    void pairsChanged();
    void sourceTextChanged();
    void translatedTextChanged();
    void liveTranslationChanged();
    void detectDirectionChanged();
    void stateChanged();
    /// A translation installed a pair on the way.
    void pairInstalled();

private:
    void start(bool install);
    void nextBatch();
    void finishBatch(const Dragoman::Reply &reply, qsizetype expected);
    void dropJob();
    void setTranslatedText(const QString &text);
    void languagesEdited();

    Dragoman::Client m_client;
    QPointer<Dragoman::Job> m_job;
    QTimer m_liveTimer;
    // Bumped by every new start, so that late detection answers are ignored.
    quint64 m_generation = 0;
    QString m_krakoman;

    QString m_source;
    QString m_target;
    QStringList m_languageCodes;
    bool m_pairsLoaded = false;
    QString m_pairsError;

    QString m_sourceText;
    QString m_translatedText;
    bool m_live = true;
    bool m_detectDirection = true;
    // The user chose the languages for the current text: keep them.
    bool m_directionChosen = false;

    // The translation in progress: the source lines, which of them are
    // sent, and the translations received so far.
    QStringList m_lines;
    QList<qsizetype> m_segmentLines;
    QStringList m_results;
    bool m_install = false;
    bool m_installedPair = false;
    bool m_detecting = false;

    bool m_installing = false;
    double m_progress = -1;
    QString m_stage;
    QString m_pivot;
    QString m_errorText;
    bool m_daemonMissing = false;
    bool m_missingPair = false;
    QString m_directionNotice;
};
