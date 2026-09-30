// SPDX-FileCopyrightText: 2026 Blagovest Petrov <blagovest@petrovs.info>
// SPDX-FileCopyrightText: 2026 Vute Tech Ltd. <https://vute.tech>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.kde.plasma.configuration

ConfigModel {
    ConfigCategory {
        name: i18nc("@title", "General")
        icon: "configure"
        source: "ConfigGeneral.qml"
    }
}
