# SPDX-FileCopyrightText: 2026 Blagovest Petrov <blagovest@petrovs.info>
# SPDX-FileCopyrightText: 2026 Vute Tech Ltd. <https://vute.tech>
# SPDX-License-Identifier: GPL-3.0-or-later

include(ECMAddTests)

# Against libdragoman-qt's fake daemon on a private bus.
ecm_add_test(translatorbackendtest.cpp ${DragomanQt_TESTING_INCLUDE_DIR}/fakedaemon.h
    TEST_NAME translatorbackendtest
    LINK_LIBRARIES translatorbackend Qt6::Test)
target_include_directories(translatorbackendtest PRIVATE ${DragomanQt_TESTING_INCLUDE_DIR})
