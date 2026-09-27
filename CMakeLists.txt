cmake_minimum_required(VERSION 3.20)
project(RobloxGameMaintenanceTool VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

qt_standard_project_setup()

set(SOURCES
    src/main.cpp
    src/SplashScreen.cpp
    src/BloxstrapManager.cpp
    src/MainWindow.cpp
    src/AddGameDialog.cpp
    src/GameEntry.cpp
    src/GameCardWidget.cpp
    src/FlowLayout.cpp
    src/RobloxApiClient.cpp
    src/Theme.cpp
    src/RoundButton.cpp
    src/SecureTokenStore.cpp
    src/GitHubClient.cpp
    src/GitRunner.cpp
    src/TokenPromptDialog.cpp
    src/RemoteSetupDialog.cpp
   app.rc
)

set(HEADERS
    src/SplashScreen.h
    src/BloxstrapManager.h
    src/MainWindow.h
    src/AddGameDialog.h
    src/GameEntry.h
    src/GameCardWidget.h
    src/FlowLayout.h
    src/RobloxApiClient.h
    src/Theme.h
    src/RoundButton.h
    src/SecureTokenStore.h
    src/GitHubClient.h
    src/GitRunner.h
    src/TokenPromptDialog.h
    src/RemoteSetupDialog.h
)

qt_add_executable(RobloxGameMaintenanceTool WIN32
    ${SOURCES}
    ${HEADERS}
)

target_link_libraries(RobloxGameMaintenanceTool PRIVATE
    Qt6::Widgets
    Qt6::Network
)

if(WIN32)
    target_link_libraries(RobloxGameMaintenanceTool PRIVATE Advapi32 Shell32 Crypt32)
endif()

set_target_properties(RobloxGameMaintenanceTool PROPERTIES
    WIN32_EXECUTABLE ON
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(RobloxGameMaintenanceTool)
endif()
