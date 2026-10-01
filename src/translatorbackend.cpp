/*
    SPDX-FileCopyrightText: 2026 Blagovest Petrov <blagovest@petrovs.info>
    SPDX-FileCopyrightText: 2026 Vute Tech Ltd. <https://vute.tech>
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "translatorbackend.h"

#include "languagenames.h"

#include <KLocalizedString>

#include <QClipboard>
#include <QCollator>
#include <QGuiApplication>
#include <QLocale>
#include <QProcess>
#include <QStandardPaths>

#include <algorithm>
#include <utility>

using namespace Qt::StringLiterals;

namespace
{

// Below the daemon's per-request limits (256 segments, 1 MiB).
constexpr qsizetype maxBatchSegments = 128;
constexpr qsizetype maxBatchChars = 256 * 1024;
constexpr int liveDelayMs = 600;
// Detection reads the beginning of the text; this is plenty.
constexpr qsizetype detectionChars = 4096;

QString defaultTarget()
{
    const QString system = QLocale().name().section(u'_', 0, 0);
    return system.isEmpty() || system == u"en" || system == u"C" ? u"bg"_s : system;
}

// The bus could not reach the daemon: not installed, or it failed to start.
bool isMissingDaemon(const QString &errorName)
{
    return errorName == u"org.freedesktop.DBus.Error.ServiceUnknown" || errorName == u"org.freedesktop.DBus.Error.NameHasNoOwner"
        || errorName.startsWith(u"org.freedesktop.DBus.Error.Spawn.");
}

} // namespace

TranslatorBackend::TranslatorBackend(QObject *parent)
    : QObject(parent)
    , m_krakoman(QStandardPaths::findExecutable(u"krakoman"_s))
    , m_source(u"en"_s)
    , m_target(defaultTarget())
{
    if (m_source == m_target) {
        m_target = u"bg"_s;
    }
    m_liveTimer.setSingleShot(true);
    m_liveTimer.setInterval(liveDelayMs);
    connect(&m_liveTimer, &QTimer::timeout, this, [this] {
        start(false);
    });
}

TranslatorBackend::~TranslatorBackend() = default;

QVariantList TranslatorBackend::languages() const
{
    QStringList codes = m_languageCodes;
    // The chosen languages are listed even before the daemon answered.
    for (const QString &code : {m_source, m_target}) {
        if (!codes.contains(code)) {
            codes.append(code);
        }
    }
    QList<std::pair<QString, QString>> named;
    named.reserve(codes.size());
    for (const QString &code : std::as_const(codes)) {
        named.append({Dragoman::languageName(code), code});
    }
    QCollator collator;
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::ranges::sort(named, [&collator](const auto &a, const auto &b) {
        return collator.compare(a.first, b.first) < 0;
    });
    QVariantList result;
    result.reserve(named.size());
    for (const auto &[name, code] : std::as_const(named)) {
        result.append(QVariantMap{{u"code"_s, code}, {u"name"_s, name}});
    }
    return result;
}

void TranslatorBackend::setSourceLanguage(const QString &code)
{
    if (code.isEmpty() || code == m_source) {
        return;
    }
    if (code == m_target) {
        m_target = m_source;
    }
    m_source = code;
    languagesEdited();
}

void TranslatorBackend::setTargetLanguage(const QString &code)
{
    if (code.isEmpty() || code == m_target) {
        return;
    }
    if (code == m_source) {
        m_source = m_target;
    }
    m_target = code;
    languagesEdited();
}

void TranslatorBackend::languagesEdited()
{
    m_directionChosen = !m_sourceText.trimmed().isEmpty();
    m_directionNotice.clear();
    Q_EMIT languagesChanged();
    if (m_live && m_directionChosen) {
        start(false);
    } else {
        cancel();
        setTranslatedText({});
    }
}

void TranslatorBackend::setSourceText(const QString &text)
{
    if (text == m_sourceText) {
        return;
    }
    m_sourceText = text;
    Q_EMIT sourceTextChanged();
    if (text.trimmed().isEmpty()) {
        m_directionChosen = false;
        m_directionNotice.clear();
        start(false); // clears at once
    } else if (m_live) {
        m_liveTimer.start();
    }
}

void TranslatorBackend::setLiveTranslation(bool live)
{
    if (live == m_live) {
        return;
    }
    m_live = live;
    Q_EMIT liveTranslationChanged();
    if (live && !m_sourceText.trimmed().isEmpty()) {
        start(false);
    }
}

void TranslatorBackend::setDetectDirection(bool detect)
{
    if (detect != m_detectDirection) {
        m_detectDirection = detect;
        Q_EMIT detectDirectionChanged();
    }
}

void TranslatorBackend::refreshPairs()
{
    m_client.listPairs([this](const QList<Dragoman::PairInfo> &pairs, const QString &error) {
        m_pairsError = error;
        if (error.isEmpty()) {
            QStringList codes;
            for (const Dragoman::PairInfo &pair : pairs) {
                for (const QString &code : {pair.source, pair.target}) {
                    if (!codes.contains(code)) {
                        codes.append(code);
                    }
                }
            }
            m_languageCodes = codes;
            m_pairsLoaded = true;
        }
        Q_EMIT pairsChanged();
    });
}

void TranslatorBackend::translate()
{
    start(false);
}

void TranslatorBackend::installPair()
{
    start(true);
}

void TranslatorBackend::swapLanguages()
{
    std::swap(m_source, m_target);
    m_directionNotice.clear();
    if (!m_translatedText.isEmpty() && !isBusy()) {
        m_sourceText = std::exchange(m_translatedText, {});
        Q_EMIT sourceTextChanged();
        Q_EMIT translatedTextChanged();
    }
    m_directionChosen = !m_sourceText.trimmed().isEmpty();
    Q_EMIT languagesChanged();
    if (m_live && m_directionChosen) {
        start(false);
    } else {
        cancel();
        setTranslatedText({});
    }
}

void TranslatorBackend::cancel()
{
    m_liveTimer.stop();
    ++m_generation;
    // A cancelled installation leaves the pair missing: offer it again.
    if (m_installing) {
        m_missingPair = true;
    }
    dropJob();
    Q_EMIT stateChanged();
}

void TranslatorBackend::copyTranslation() const
{
    QGuiApplication::clipboard()->setText(m_translatedText);
}

void TranslatorBackend::revertDirectionSwap()
{
    if (m_directionNotice.isEmpty()) {
        return;
    }
    std::swap(m_source, m_target);
    languagesEdited();
    if (!m_live && m_directionChosen) {
        start(false); // the user asked for this translation
    }
}

void TranslatorBackend::openInKrakoman() const
{
    if (!m_krakoman.isEmpty()) {
        QProcess::startDetached(m_krakoman, krakomanArguments(m_source, m_target, m_sourceText));
    }
}

QString TranslatorBackend::languageName(const QString &code) const
{
    return Dragoman::languageName(code);
}

QString TranslatorBackend::languageNameInSentence(const QString &code) const
{
    return Dragoman::languageNameInSentence(code);
}

QList<qsizetype> TranslatorBackend::segmentLines(const QStringList &lines)
{
    QList<qsizetype> result;
    for (qsizetype i = 0; i < lines.size(); ++i) {
        if (std::ranges::any_of(lines.at(i), [](QChar c) {
                return c.isLetterOrNumber();
            })) {
            result.append(i);
        }
    }
    return result;
}

QStringList TranslatorBackend::krakomanArguments(const QString &source, const QString &target, const QString &text)
{
    QStringList arguments{u"--source"_s, source, u"--target"_s, target};
    if (!text.trimmed().isEmpty()) {
        // After "--", a text that starts with a dash is not an option.
        arguments << u"--"_s << text;
    }
    return arguments;
}

void TranslatorBackend::start(bool install)
{
    m_liveTimer.stop();
    ++m_generation;
    dropJob();
    m_errorText.clear();
    m_daemonMissing = false;
    m_missingPair = false;
    m_pivot.clear();

    m_lines = m_sourceText.split(u'\n');
    m_segmentLines = segmentLines(m_lines);
    m_results.clear();
    if (m_segmentLines.isEmpty()) {
        setTranslatedText(m_sourceText.trimmed().isEmpty() ? QString() : m_sourceText);
        Q_EMIT stateChanged();
        return;
    }
    m_install = install;
    m_installedPair = false;
    m_installing = install;

    // An installation was asked for this very pair: keep it.
    if (!m_detectDirection || m_directionChosen || install) {
        nextBatch();
        return;
    }
    m_detecting = true;
    Q_EMIT stateChanged();
    m_client.detectLanguage(
        m_sourceText.left(detectionChars),
        {m_source, m_target},
        [this, generation = m_generation](const Dragoman::Detection &detection, const QString &error) {
            if (generation != m_generation) {
                return; // superseded or cancelled
            }
            m_detecting = false;
            // Without detection, translate as chosen; a
            // missing daemon is reported by the translation.
            if (error.isEmpty() && detection.reliable && detection.language == m_target) {
                std::swap(m_source, m_target);
                m_directionNotice =
                    i18nc("@info %1 is a language name", "The text is in %1, so the languages were swapped.", Dragoman::languageNameInSentence(m_source));
                Q_EMIT languagesChanged();
            }
            nextBatch();
        });
}

void TranslatorBackend::nextBatch()
{
    const auto done = m_results.size();
    if (done == m_segmentLines.size()) {
        QStringList lines = m_lines;
        for (qsizetype i = 0; i < m_segmentLines.size(); ++i) {
            lines[m_segmentLines.at(i)] = m_results.at(i);
        }
        setTranslatedText(lines.join(u'\n'));
        m_installing = false;
        Q_EMIT stateChanged();
        if (m_installedPair) {
            Q_EMIT pairInstalled();
            refreshPairs();
        }
        return;
    }

    QStringList batch;
    qsizetype chars = 0;
    for (auto i = done; i < m_segmentLines.size() && batch.size() < maxBatchSegments; ++i) {
        const QString &line = m_lines.at(m_segmentLines.at(i));
        if (!batch.isEmpty() && chars + line.size() > maxBatchChars) {
            break;
        }
        batch.append(line);
        chars += line.size();
    }

    Dragoman::Client::TranslateOptions options;
    options.installOnDemand = m_install;
    m_job = m_client.translate(m_source, m_target, batch, options);
    connect(m_job, &Dragoman::Job::progress, this, [this](double fraction, const QString &stage) {
        m_installing = true;
        m_progress = fraction;
        m_stage = stage;
        Q_EMIT stateChanged();
    });
    connect(m_job, &Dragoman::Job::finished, this, [this, expected = batch.size()](const Dragoman::Reply &reply) {
        finishBatch(reply, expected);
    });
    Q_EMIT stateChanged();
}

void TranslatorBackend::finishBatch(const Dragoman::Reply &reply, qsizetype expected)
{
    m_job = nullptr;
    m_progress = -1;
    m_stage.clear();
    if (!reply.ok()) {
        m_installing = false;
        if (reply.errorName == Dragoman::Errors::NotInstalled) {
            m_missingPair = true;
        } else if (isMissingDaemon(reply.errorName)) {
            m_daemonMissing = true;
        } else if (!reply.cancelled()) {
            m_errorText = reply.error;
        }
        Q_EMIT stateChanged();
        return;
    }
    const QStringList translations = reply.translations();
    if (translations.size() != expected) {
        m_installing = false;
        m_errorText = i18nc("@info", "The translation service returned %1 lines for %2.", translations.size(), expected);
        Q_EMIT stateChanged();
        return;
    }
    // Once installed, the remaining batches need no installation.
    if (reply.prepared) {
        m_installedPair = true;
        m_installing = false;
    }
    m_pivot = reply.pivot();
    m_results.append(translations);
    nextBatch();
}

void TranslatorBackend::dropJob()
{
    if (m_job) {
        disconnect(m_job, nullptr, this, nullptr);
        m_job->cancel();
        m_job = nullptr;
    }
    m_detecting = false;
    m_installing = false;
    m_progress = -1;
    m_stage.clear();
}

void TranslatorBackend::setTranslatedText(const QString &text)
{
    if (text != m_translatedText) {
        m_translatedText = text;
        Q_EMIT translatedTextChanged();
    }
}
