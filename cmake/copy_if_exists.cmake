# Copy SRC to DEST only if SRC exists. Used as a POST_BUILD step for
# dashboard-go/dashboard.exe, which is built separately (with `go
# build`, not by this CMake project) and may not exist yet on a fresh
# checkout — in that case this is a silent no-op instead of a build
# error, so the C++ app still builds fine and the tray's "Open Local
# Dashboard (Go)" item just shows a "not found, build it first"
# message until the Go binary is built.
if(EXISTS "${SRC}")
    file(COPY "${SRC}" DESTINATION "${DEST_DIR}")
endif()
