# SPDX-FileCopyrightText: 2026 Blagovest Petrov <blagovest@petrovs.info>
# SPDX-FileCopyrightText: 2026 Vute Tech Ltd. <https://vute.tech>
# SPDX-License-Identifier: GPL-3.0-or-later

set(applet_id dev.l10n_bg.dragomand.translator)
set(translation_domain "plasma_applet_${applet_id}")

# The translation logic, free of Plasma: the applet compiles it into its
# QML module, the autotests link it as a plain library.
set(backend_sources
    translatorbackend.cpp
    translatorbackend.h)
set(backend_libraries
    DragomanQt::DragomanQt
    Qt6::Gui
    Qt6::Qml
    KF6::I18n)

plasma_add_applet(${applet_id}
    QML_SOURCES
        qml/main.qml
        qml/FullRepresentation.qml
        qml/LanguageComboBox.qml
        qml/config.qml
        qml/ConfigGeneral.qml
    RESOURCES
        config/main.xml
    CPP_SOURCES
        translatorapplet.cpp
        translatorapplet.h
        ${backend_sources})
target_compile_definitions(${applet_id} PRIVATE TRANSLATION_DOMAIN="${translation_domain}")
target_link_libraries(${applet_id} PRIVATE
    ${backend_libraries}
    Qt6::Quick
    KF6::CoreAddons
    Plasma::Plasma)

if(BUILD_TESTING)
    add_library(translatorbackend STATIC ${backend_sources})
    target_compile_definitions(translatorbackend PUBLIC TRANSLATION_DOMAIN="${translation_domain}")
    target_include_directories(translatorbackend PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(translatorbackend PUBLIC ${backend_libraries})
endif()
