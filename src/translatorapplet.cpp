/*
    SPDX-FileCopyrightText: 2026 Blagovest Petrov <blagovest@petrovs.info>
    SPDX-FileCopyrightText: 2026 Vute Tech Ltd. <https://vute.tech>
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "translatorapplet.h"

#include <KPluginFactory>

#include <QIcon>

using namespace Qt::StringLiterals;

TranslatorApplet::TranslatorApplet(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : Plasma::Applet(parent, data, args)
    , m_backend(new TranslatorBackend(this))
{
    // Dragomand installs its icon; without it, the icon theme's own.
    if (!QIcon::hasThemeIcon(icon())) {
        setIcon(u"translate"_s);
    }
}

TranslatorApplet::~TranslatorApplet() = default;

K_PLUGIN_CLASS_WITH_JSON(TranslatorApplet, "metadata.json")

#include "translatorapplet.moc"
