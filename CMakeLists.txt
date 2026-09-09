cmake_minimum_required(VERSION 3.16)
project(macchanger-toolkit VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)


add_executable(macchanger-toolkit
    src/main.cpp
    src/macchanger_widget.h
    src/macchanger_widget.cpp
    src/macchanger_widget.ui
    src/ping_tab.h
    src/ping_tab.cpp
    src/ping_tab.ui
    src/switch_button.h
    src/switch_button.cpp
    src/terminal_about_dialog.cpp
    src/terminal_about_dialog.h
    src/ping_graph.h
    src/scan_worker.h
    src/scan_worker.cpp
    src/terminal_help_dialog.cpp
    src/terminal_help_dialog.h
    resources/resources.qrc
    src/ping_logger_dialog.cpp
    src/ping_logger_dialog.h
    src/ping_logger_dialog.ui
)

target_link_libraries(macchanger-toolkit PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network)

target_include_directories(macchanger-toolkit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

install(TARGETS macchanger-toolkit DESTINATION bin)

install(FILES config/macchanger.desktop DESTINATION share/applications)

install(FILES resources/icon.png DESTINATION share/pixmaps RENAME macchanger-toolkit.png)

set(CPACK_PACKAGE_NAME "macchanger-toolkit")
set(CPACK_PACKAGE_VERSION "1.0.3")
set(CPACK_PACKAGE_RELEASE "1")
set(CPACK_PACKAGE_CONTACT "SubOfTheDarkness <204970490+SubOfTheDarkness@users.noreply.github.com>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Fast MAC address changer and network ping toolkit")
set(CPACK_PACKAGE_VENDOR "Sub")

set(CPACK_GENERATOR "DEB")

set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")

set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libqt6widgets6 (>= 6.0), iproute2, iputils-ping, policykit-1")

include(CPack)
