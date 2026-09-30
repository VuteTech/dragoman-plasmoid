// SPDX-FileCopyrightText: 2026 Blagovest Petrov <blagovest@petrovs.info>
// SPDX-FileCopyrightText: 2026 Vute Tech Ltd. <https://vute.tech>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.kde.plasma.components as PlasmaComponents3

// Every language the daemon knows, showing the language named by `code`.
PlasmaComponents3.ComboBox {
    id: combo

    required property TranslatorBackend backend
    required property string code
    signal chosen(string code)

    model: backend.languages
    textRole: "name"
    valueRole: "code"
    // Before the daemon answered, the list holds the chosen languages only.
    displayText: currentIndex < 0 ? backend.languageName(code) : currentText

    function sync(): void {
        currentIndex = indexOfValue(code);
    }

    onCodeChanged: sync()
    // The combo box picks the first entry of a new model: choose afterwards.
    onCountChanged: Qt.callLater(sync)
    onModelChanged: Qt.callLater(sync)
    Component.onCompleted: sync()
    onActivated: chosen(currentValue)

    // On the desktop the popup never "opens": fetch the list on first use.
    popup.onAboutToShow: {
        if (!backend.pairsLoaded) {
            backend.refreshPairs();
        }
    }
}
