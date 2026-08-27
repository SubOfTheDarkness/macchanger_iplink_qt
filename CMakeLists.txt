cmake_minimum_required(VERSION 3.16)
project(macchanger-toolkit VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

add_executable(macchanger-toolkit
    main.cpp
    macchanger_widget.h
    macchanger_widget.cpp
    macchanger_widget.ui
    ping_tab.h
    ping_tab.cpp
    ping_tab.ui
    resources.qrc
    switch_button.h
    switch_button.cpp
    terminal_about_dialog.cpp
    terminal_about_dialog.h
    ping_graph.h
)

target_link_libraries(macchanger-toolkit PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)

target_include_directories(macchanger-toolkit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS macchanger-toolkit DESTINATION bin)

install(FILES macchanger.desktop DESTINATION share/applications)

install(FILES icon.png DESTINATION share/pixmaps RENAME macchanger-toolkit.png)

set(CPACK_PACKAGE_NAME "macchanger-toolkit")
set(CPACK_PACKAGE_VERSION "1.0.2")
set(CPACK_PACKAGE_RELEASE "1")
set(CPACK_PACKAGE_CONTACT "SubOfTheDarkness <204970490+SubOfTheDarkness@users.noreply.github.com>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Fast MAC address changer and network ping toolkit")
set(CPACK_PACKAGE_VENDOR "Sub")

set(CPACK_GENERATOR "DEB")

set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")

set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libqt6widgets6 (>= 6.0), iproute2, iputils-ping, policykit-1")

include(CPack)
