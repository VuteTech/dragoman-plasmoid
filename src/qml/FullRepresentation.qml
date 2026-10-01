// SPDX-FileCopyrightText: 2026 Blagovest Petrov <blagovest@petrovs.info>
// SPDX-FileCopyrightText: 2026 Vute Tech Ltd. <https://vute.tech>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents3
import org.kde.plasma.extras as PlasmaExtras

// The popup: the two languages, the text and its translation.
PlasmaExtras.Representation {
    id: full

    // The backend's type is registered only where libplasma builds the
    // QML module (6.4 and newer), so it is held untyped.
    required property var backend
    signal closeRequested()

    readonly property bool hasText: backend.sourceText.trim().length > 0

    function focusInput(): void {
        sourceArea.forceActiveFocus();
    }

    function ensurePairs(): void {
        if (!backend.pairsLoaded) {
            backend.refreshPairs();
        }
    }

    function handleKey(event: KeyEvent): void {
        if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter) && (event.modifiers & Qt.ControlModifier)) {
            if (full.hasText) {
                full.backend.translate();
            }
            event.accepted = true;
        } else if (event.key === Qt.Key_Escape) {
            full.closeRequested();
            event.accepted = true;
        }
    }

    Layout.preferredWidth: Kirigami.Units.gridUnit * 24
    Layout.preferredHeight: Kirigami.Units.gridUnit * 26
    Layout.minimumWidth: Kirigami.Units.gridUnit * 16
    Layout.minimumHeight: Kirigami.Units.gridUnit * 16

    collapseMarginsHint: true
    Keys.onPressed: event => handleKey(event)

    header: PlasmaExtras.PlasmoidHeading {
        contentItem: RowLayout {
            spacing: Kirigami.Units.smallSpacing

            LanguageComboBox {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                backend: full.backend
                code: full.backend.sourceLanguage
                onChosen: code => full.backend.sourceLanguage = code
                Accessible.name: i18nc("@label:listbox", "Source language")
            }

            PlasmaComponents3.ToolButton {
                icon.name: "exchange-positions"
                text: i18nc("@action:button", "Swap Languages")
                display: PlasmaComponents3.AbstractButton.IconOnly
                onClicked: full.backend.swapLanguages()
                PlasmaComponents3.ToolTip.text: text
                PlasmaComponents3.ToolTip.visible: hovered
                PlasmaComponents3.ToolTip.delay: Kirigami.Units.toolTipDelay
            }

            LanguageComboBox {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                backend: full.backend
                code: full.backend.targetLanguage
                onChosen: code => full.backend.targetLanguage = code
                Accessible.name: i18nc("@label:listbox", "Target language")
            }
        }
    }

    contentItem: ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            type: Kirigami.MessageType.Error
            visible: full.backend.daemonMissing || (!full.backend.pairsLoaded && full.backend.pairsError.length > 0)
            text: full.backend.daemonMissing || full.backend.pairsError.length === 0
                ? i18nc("@info", "The Dragomand translation service is not available. Check that dragomand is installed.")
                : i18nc("@info %1 is an error message", "Cannot reach the Dragomand translation service: %1", full.backend.pairsError)
            actions: Kirigami.Action {
                text: i18nc("@action:button", "Retry")
                icon.name: "view-refresh"
                onTriggered: {
                    full.backend.refreshPairs();
                    if (full.hasText) {
                        full.backend.translate();
                    }
                }
            }
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            type: Kirigami.MessageType.Error
            visible: full.backend.errorText.length > 0
            text: full.backend.errorText
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            type: Kirigami.MessageType.Information
            visible: full.backend.missingPair
            text: i18nc("@info %1 and %2 are language names",
                        "Translating from %1 into %2 needs language models that are not installed yet.",
                        full.backend.languageNameInSentence(full.backend.sourceLanguage),
                        full.backend.languageNameInSentence(full.backend.targetLanguage))
            actions: Kirigami.Action {
                text: i18nc("@action:button", "Install")
                icon.name: "download"
                tooltip: i18nc("@info:tooltip", "Download the language models and translate")
                onTriggered: full.backend.installPair()
            }
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            type: Kirigami.MessageType.Information
            visible: full.backend.directionNotice.length > 0
            text: full.backend.directionNotice
            actions: Kirigami.Action {
                text: i18nc("@action:button", "Swap Back")
                icon.name: "edit-undo"
                tooltip: i18nc("@info:tooltip", "Translate in the direction chosen before")
                onTriggered: full.backend.revertDirectionSwap()
            }
        }

        PlasmaComponents3.Label {
            Layout.fillWidth: true
            visible: full.backend.installing
            text: full.backend.progress < 0
                ? i18nc("@info:status", "Installing language models")
                : i18nc("@info:status %1 is a percentage", "Installing language models: %1%", Math.round(full.backend.progress * 100))
            wrapMode: Text.Wrap
        }

        RowLayout {
            Layout.fillWidth: true
            visible: full.backend.installing
            spacing: Kirigami.Units.smallSpacing

            PlasmaComponents3.ProgressBar {
                Layout.fillWidth: true
                from: 0
                to: 1
                value: Math.max(full.backend.progress, 0)
                indeterminate: full.backend.progress < 0
                Accessible.name: i18nc("@info:status", "Installing language models")
            }
            PlasmaComponents3.ToolButton {
                icon.name: "process-stop"
                text: i18nc("@action:button", "Cancel")
                onClicked: full.backend.cancel()
                PlasmaComponents3.ToolTip.text: i18nc("@info:tooltip", "Stop the download")
                PlasmaComponents3.ToolTip.visible: hovered
                PlasmaComponents3.ToolTip.delay: Kirigami.Units.toolTipDelay
            }
        }

        PlasmaComponents3.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: 1
            // The text wraps: never scroll sideways.
            PlasmaComponents3.ScrollBar.horizontal.policy: PlasmaComponents3.ScrollBar.AlwaysOff

            PlasmaComponents3.TextArea {
                id: sourceArea
                text: full.backend.sourceText
                placeholderText: i18nc("@info:placeholder", "Type or paste text to translate")
                wrapMode: TextEdit.Wrap
                focus: true
                Accessible.name: i18nc("@label:textbox", "Text to translate")
                onTextChanged: full.backend.sourceText = text
                onActiveFocusChanged: {
                    if (activeFocus) {
                        full.ensurePairs();
                    }
                }
                // Before the text area handles Return itself.
                Keys.onPressed: event => full.handleKey(event)
            }
        }

        PlasmaComponents3.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: 1
            // The text wraps: never scroll sideways.
            PlasmaComponents3.ScrollBar.horizontal.policy: PlasmaComponents3.ScrollBar.AlwaysOff

            PlasmaComponents3.TextArea {
                text: full.backend.translatedText
                placeholderText: full.backend.busy ? "" : i18nc("@info:placeholder", "Translation")
                readOnly: true
                selectByMouse: true
                wrapMode: TextEdit.Wrap
                opacity: full.backend.busy ? 0.6 : 1
                Accessible.name: i18nc("@label:textbox", "Translation")
                Keys.onPressed: event => full.handleKey(event)

                PlasmaComponents3.BusyIndicator {
                    anchors.centerIn: parent
                    running: full.backend.busy && !full.backend.installing
                    visible: running
                }
            }
        }

        PlasmaComponents3.Label {
            Layout.fillWidth: true
            visible: full.backend.pivot.length > 0
            text: i18nc("@info %1 is a language name", "Translated through %1: no direct model exists for this pair.",
                        full.backend.languageNameInSentence(full.backend.pivot))
            wrapMode: Text.Wrap
            font: Kirigami.Theme.smallFont
            opacity: 0.7
        }
    }

    footer: PlasmaExtras.PlasmoidHeading {
        position: PlasmaComponents3.ToolBar.Footer
        contentItem: RowLayout {
            spacing: Kirigami.Units.smallSpacing

            PlasmaComponents3.Switch {
                Layout.fillWidth: true
                text: i18nc("@option:check", "Translate while typing")
                checked: full.backend.liveTranslation
                onToggled: full.backend.liveTranslation = checked
            }
            PlasmaComponents3.Button {
                icon.name: "translate"
                text: i18nc("@action:button", "Translate")
                visible: !full.backend.liveTranslation
                enabled: full.hasText
                onClicked: full.backend.translate()
                PlasmaComponents3.ToolTip.text: i18nc("@info:tooltip", "Translate now (Ctrl+Return)")
                PlasmaComponents3.ToolTip.visible: hovered
                PlasmaComponents3.ToolTip.delay: Kirigami.Units.toolTipDelay
            }
            PlasmaComponents3.ToolButton {
                icon.name: "dev.l10n_bg.krakoman"
                text: i18nc("@action:button", "Open in Krakoman")
                display: PlasmaComponents3.AbstractButton.IconOnly
                visible: full.backend.krakomanAvailable
                onClicked: {
                    full.backend.openInKrakoman();
                    full.closeRequested();
                }
                PlasmaComponents3.ToolTip.text: text
                PlasmaComponents3.ToolTip.visible: hovered
                PlasmaComponents3.ToolTip.delay: Kirigami.Units.toolTipDelay
            }
            PlasmaComponents3.ToolButton {
                icon.name: "edit-copy"
                text: i18nc("@action:button", "Copy")
                enabled: full.backend.translatedText.length > 0
                onClicked: full.backend.copyTranslation()
                Accessible.name: i18nc("@action:button", "Copy Translation")
                PlasmaComponents3.ToolTip.text: i18nc("@info:tooltip", "Copy the translation to the clipboard")
                PlasmaComponents3.ToolTip.visible: hovered
                PlasmaComponents3.ToolTip.delay: Kirigami.Units.toolTipDelay
            }
        }
    }
}
