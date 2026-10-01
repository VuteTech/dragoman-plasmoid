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

set(applet_qml
    qml/main.qml
    qml/FullRepresentation.qml
    qml/LanguageComboBox.qml
    qml/config.qml
    qml/ConfigGeneral.qml)

if(COMMAND plasma_add_applet)
    # libplasma 6.4 and newer: the QML is compiled into the plugin.
    plasma_add_applet(${applet_id}
        QML_SOURCES ${applet_qml}
        RESOURCES
            config/main.xml
        CPP_SOURCES
            translatorapplet.cpp
            translatorapplet.h
            ${backend_sources})
else()
    # Older libplasma (Debian 13 has 6.3): a plain plugin, plus the QML
    # installed as a classic package (contents/ui, contents/config).
    kcoreaddons_add_plugin(${applet_id}
        SOURCES
            translatorapplet.cpp
            translatorapplet.h
            ${backend_sources}
        INSTALL_NAMESPACE "plasma/applets")
    set(package ${CMAKE_CURRENT_BINARY_DIR}/package)
    file(MAKE_DIRECTORY ${package}/contents/ui ${package}/contents/config)
    foreach(qml ${applet_qml})
        get_filename_component(name ${qml} NAME)
        if(name STREQUAL "config.qml")
            configure_file(${qml} ${package}/contents/config/config.qml COPYONLY)
        else()
            configure_file(${qml} ${package}/contents/ui/${name} COPYONLY)
        endif()
    endforeach()
    configure_file(config/main.xml ${package}/contents/config/main.xml COPYONLY)
    configure_file(metadata.json ${package}/metadata.json COPYONLY)
    plasma_install_package(${package} ${applet_id})
endif()
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
