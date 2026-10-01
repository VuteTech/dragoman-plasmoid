// SPDX-FileCopyrightText: 2026 Blagovest Petrov <blagovest@petrovs.info>
// SPDX-FileCopyrightText: 2026 Vute Tech Ltd. <https://vute.tech>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM
import org.kde.plasma.plasmoid

KCM.SimpleKCM {
    id: page

    // Every entry of config/main.xml, with its default: the dialog sets them all.
    property string cfg_sourceLanguage
    property string cfg_sourceLanguageDefault
    property string cfg_targetLanguage
    property string cfg_targetLanguageDefault
    property bool cfg_liveTranslation
    property bool cfg_liveTranslationDefault
    property bool cfg_detectDirection
    property bool cfg_detectDirectionDefault

    readonly property var backend: Plasmoid.backend
    // An empty setting stands for the languages the widget starts with.
    readonly property string source: cfg_sourceLanguage.length > 0 ? cfg_sourceLanguage : backend.sourceLanguage
    readonly property string target: cfg_targetLanguage.length > 0 ? cfg_targetLanguage : backend.targetLanguage

    function choose(isSource: bool, code: string): void {
        const other = isSource ? target : source;
        const previous = isSource ? source : target;
        // Choosing the other side's language swaps the two.
        const swapped = code === other ? previous : other;
        cfg_sourceLanguage = isSource ? code : swapped;
        cfg_targetLanguage = isSource ? swapped : code;
    }

    Component.onCompleted: {
        if (!backend.pairsLoaded) {
            backend.refreshPairs();
        }
    }

    Kirigami.FormLayout {
        QQC2.ComboBox {
            id: sourceBox
            Kirigami.FormData.label: i18nc("@label:listbox", "Translate from:")
            model: page.backend.languages
            textRole: "name"
            valueRole: "code"
            displayText: currentIndex < 0 ? page.backend.languageName(page.source) : currentText
            function sync(): void {
                currentIndex = indexOfValue(page.source);
            }
            // The combo box picks the first entry of a new model: choose afterwards.
            onCountChanged: Qt.callLater(sync)
            onModelChanged: Qt.callLater(sync)
            Component.onCompleted: sync()
            Connections {
                target: page
                function onSourceChanged(): void {
                    sourceBox.sync();
                }
            }
            onActivated: page.choose(true, currentValue)
            Accessible.name: i18nc("@label:listbox", "Source language")
        }

        QQC2.ComboBox {
            id: targetBox
            Kirigami.FormData.label: i18nc("@label:listbox", "Translate into:")
            model: page.backend.languages
            textRole: "name"
            valueRole: "code"
            displayText: currentIndex < 0 ? page.backend.languageName(page.target) : currentText
            function sync(): void {
                currentIndex = indexOfValue(page.target);
            }
            // The combo box picks the first entry of a new model: choose afterwards.
            onCountChanged: Qt.callLater(sync)
            onModelChanged: Qt.callLater(sync)
            Component.onCompleted: sync()
            Connections {
                target: page
                function onTargetChanged(): void {
                    targetBox.sync();
                }
            }
            onActivated: page.choose(false, currentValue)
            Accessible.name: i18nc("@label:listbox", "Target language")
        }

        Item {
            Kirigami.FormData.isSection: true
        }

        QQC2.CheckBox {
            Kirigami.FormData.label: i18nc("@title:group", "Translation:")
            text: i18nc("@option:check", "Translate while typing")
            checked: page.cfg_liveTranslation
            onToggled: page.cfg_liveTranslation = checked
        }

        QQC2.CheckBox {
            text: i18nc("@option:check", "Swap the languages when the text is in the target language")
            checked: page.cfg_detectDirection
            onToggled: page.cfg_detectDirection = checked
        }
    }
}
