/*
    SPDX-FileCopyrightText: 2026 Blagovest Petrov <blagovest@petrovs.info>
    SPDX-FileCopyrightText: 2026 Vute Tech Ltd. <https://vute.tech>
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include "translatorbackend.h"

#include <Plasma/Applet>

/// The widget: hands the translation backend to its QML as Plasmoid.backend.
class TranslatorApplet : public Plasma::Applet
{
    Q_OBJECT
    Q_PROPERTY(TranslatorBackend *backend READ backend CONSTANT)

public:
    TranslatorApplet(QObject *parent, const KPluginMetaData &data, const QVariantList &args);
    ~TranslatorApplet() override;

    [[nodiscard]] TranslatorBackend *backend() const
    {
        return m_backend;
    }

private:
    TranslatorBackend *m_backend;
};
