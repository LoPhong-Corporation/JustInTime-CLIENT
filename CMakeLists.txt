cmake_minimum_required(VERSION 3.28)
project(JustInTime C)

set(CMAKE_C_STANDARD 11)

add_executable(JustInTime src/main.c
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
        src/backup.c)
include_directories(third_party)
include_directories(include)

target_link_libraries(
        JustInTime
        psapi
        winhttp
)