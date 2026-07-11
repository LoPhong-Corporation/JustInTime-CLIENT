cmake_minimum_required(VERSION 3.28)
project(JustInTime LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt: tự sinh moc cho các class có Q_OBJECT (TrayIcon, LoginDialog,
# SettingsDialog, SupabaseSetupDialog).
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Nếu CMake không tự tìm thấy Qt6, set CMAKE_PREFIX_PATH trỏ tới
# thư mục cài Qt, ví dụ:
#   -DCMAKE_PREFIX_PATH="C:/Qt/6.7.0/msvc2019_64"
set(CMAKE_PREFIX_PATH "C:/Qt/6.11.1/msvc2022_64")

find_package(Qt6 REQUIRED COMPONENTS Widgets)
add_executable(JustInTime WIN32
        # ---- Lõi C (giữ nguyên logic gốc) ----
        src/activity.c
        include/activity.h
        src/database.c
        include/database.h
        third_party/sqlite/sqlite3.c
        third_party/sqlite/sqlite3.h
        src/device.c
        include/device.h
        src/sync.c
        include/sync.h
        include/config.h
        include/network.h
        src/network.c
        src/jsonutil.c
        include/jsonutil.h
        src/backup.c
        include/backup.h
        src/settings.c
        include/settings.h
        src/auth.c
        include/auth.h
        include/error_codes.h

        # ---- UI Qt (C++) ----
        src/main.cpp
        src/trayicon.h
        src/trayicon.cpp
        src/logindialog.h
        src/logindialog.cpp
        src/settingsdialog.h
        src/settingsdialog.cpp
        src/supabasesetupdialog.h
        src/supabasesetupdialog.cpp
)

include_directories(third_party)
include_directories(include)

target_link_libraries(
        JustInTime
        Qt6::Widgets
        psapi
        winhttp
        crypt32
        advapi32
)
