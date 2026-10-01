// SPDX-FileCopyrightText: 2026 Blagovest Petrov <blagovest@petrovs.info>
// SPDX-FileCopyrightText: 2026 Vute Tech Ltd. <https://vute.tech>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid

PlasmoidItem {
    id: root

    readonly property var backend: Plasmoid.backend

    // The stored settings; the backend follows them, and they follow the
    // backend when the popup changes the languages or the live switch.
    readonly property string configSource: Plasmoid.configuration.sourceLanguage
    readonly property string configTarget: Plasmoid.configuration.targetLanguage
    readonly property bool configLive: Plasmoid.configuration.liveTranslation
    readonly property bool configDetect: Plasmoid.configuration.detectDirection

    function applyLanguages(): void {
        if (root.configSource.length > 0) {
            root.backend.sourceLanguage = root.configSource;
        }
        if (root.configTarget.length > 0) {
            root.backend.targetLanguage = root.configTarget;
        }
    }

    onConfigSourceChanged: applyLanguages()
    onConfigTargetChanged: applyLanguages()
    onConfigLiveChanged: root.backend.liveTranslation = root.configLive
    onConfigDetectChanged: root.backend.detectDirection = root.configDetect

    Component.onCompleted: {
        applyLanguages();
        root.backend.liveTranslation = root.configLive;
        root.backend.detectDirection = root.configDetect;
    }

    Connections {
        target: root.backend

        function onLanguagesChanged(): void {
            Plasmoid.configuration.sourceLanguage = root.backend.sourceLanguage;
            Plasmoid.configuration.targetLanguage = root.backend.targetLanguage;
        }
        function onLiveTranslationChanged(): void {
            Plasmoid.configuration.liveTranslation = root.backend.liveTranslation;
        }
    }

    switchWidth: Kirigami.Units.gridUnit * 16
    switchHeight: Kirigami.Units.gridUnit * 14

    toolTipMainText: Plasmoid.title
    toolTipSubText: i18nc("@info:tooltip", "Translate text without an internet connection")

    // The language list is fetched when the popup opens, never at login:
    // that would start the daemon for nothing.
    onExpandedChanged: {
        if (root.expanded) {
            root.backend.refreshPairs();
            const full = root.fullRepresentationItem as FullRepresentation;
            if (full) {
                full.focusInput();
            }
        }
    }

    fullRepresentation: FullRepresentation {
        backend: root.backend
        onCloseRequested: root.expanded = false
    }
}
