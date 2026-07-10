cmake_minimum_required(VERSION 3.28)
project(JustInTime C)

set(CMAKE_C_STANDARD 11)

add_executable(JustInTime WIN32
        src/main.c
        include/activity.h
        src/activity.c
        include/database.h
        src/database.c
        third_party/sqlite/sqlite3.c
        third_party/sqlite/sqlite3.h
        include/device.h
        src/device.c
        include/sync.h
        src/sync.c
        include/config.h
        include/network.h
        src/network.c
        include/jsonutil.h
        src/jsonutil.c
        include/backup.h
        src/backup.c
        include/settings.h
        src/settings.c
        include/auth.h
        src/auth.c
        include/tray.h
        src/tray.c
        include/ui_dialog.h
        src/ui_dialog.c
        include/error_codes.h)
include_directories(third_party)
include_directories(include)

target_link_libraries(
        JustInTime
        psapi
        winhttp
        crypt32
        shell32
        user32
        gdi32
        advapi32
)
