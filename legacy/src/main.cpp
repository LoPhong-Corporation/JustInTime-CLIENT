// main.cpp
//
// Điểm vào app: khởi tạo Qt, tray icon, và một worker thread
// chạy các hàm lõi C (monitor_activity/sync/backup/summary)
// y hệt logic của bản Win32 thuần trước đây - chỉ có lớp UI
// (tray, dialog) là được viết lại bằng Qt/C++.

#include <QApplication>
#include <QMessageBox>
#include <QSystemTrayIcon>

#include <windows.h>

#include <cstdio>
#include <ctime>
#include <atomic>
#include <thread>

#include <io.h>
#include <fcntl.h>
#include <clocale>

extern "C" {
#include "../include/activity.h"
#include "../include/database.h"
#include "../include/sync.h"
#include "../include/backup.h"
#include "../include/config.h"
#include "../include/settings.h"
#include "../include/auth.h"
#include "../include/error_codes.h"
#include "../include/remoteview.h"
}

#include "trayicon.h"

static std::atomic<bool> g_workerRunning{true};

/*
 * Worker thread: chạy vòng lặp theo dõi/sync/backup/báo cáo.
 * Không đụng tới bất kỳ đối tượng Qt/QWidget nào (chỉ gọi
 * hàm C thuần), nên an toàn khi chạy trên thread riêng,
 * tách biệt khỏi GUI thread để tray/dialog không bị đứng
 * hình mỗi khi có lệnh gọi mạng (WinHTTP là đồng bộ/blocking).
 */
static void workerLoop(TrayIcon *tray)
{
    time_t lastSync    = time(nullptr);
    time_t lastBackup  = time(nullptr);
    time_t lastSummary = time(nullptr);

    backup_create_snapshot();

    while (g_workerRunning.load())
    {
        AppSettings s;
        settings_get(&s);

        if (!tray->isPaused())
        {
            monitor_activity();
        }

        time_t now = time(nullptr);

        if (now - lastSync >= s.sync_interval_sec)
        {
            sync_pending_records();
            lastSync = now;
        }

        if (now - lastBackup >= s.backup_interval_sec)
        {
            backup_create_snapshot();
            db_delete_old_records(RETENTION_DAYS);
            lastBackup = now;
        }

        if (now - lastSummary >= s.summary_interval_sec)
        {
            db_print_daily_summary();
            lastSummary = now;
        }

        Sleep(1000);
    }
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    /*
     * Không thoát app khi đóng dialog cuối cùng (Settings,
     * Login...) - app chỉ thoát khi bấm "Exit" trong tray.
     */
    app.setQuitOnLastWindowClosed(false);

    /*
     * Console debug: tạo sẵn nhưng ẩn mặc định, để mọi
     * wprintf() trong code C lõi vẫn hoạt động bình thường,
     * chỉ là ẩn khỏi mắt người dùng trừ khi bật trong menu.
     */
    AllocConsole();

    FILE *dummy;
    freopen_s(&dummy, "CONOUT$", "w", stdout);
    freopen_s(&dummy, "CONOUT$", "w", stderr);

    _setmode(_fileno(stdout), _O_U16TEXT);
    setlocale(LC_ALL, "");

    HWND consoleWnd = GetConsoleWindow();

    if (consoleWnd)
        ShowWindow(consoleWnd, SW_HIDE);

    wprintf(L"JustInTime Agent Started (Qt UI)\n");

    if (!QSystemTrayIcon::isSystemTrayAvailable())
    {
        QMessageBox::critical(
            nullptr,
            "JustInTime",
            QString("[%1] System tray is not available on this system.")
                .arg(ERR_UI_TRAY_INIT_FAIL)
        );

        return 1;
    }

    if (!db_init())
    {
        QMessageBox::critical(
            nullptr,
            "JustInTime",
            QString("[%1] Failed to initialize database.").arg(ERR_DB_OPEN_FAIL)
        );

        return 1;
    }

    AppSettings settings;
    settings_load(&settings);

    auth_load_session();

    remoteview_start();

    TrayIcon tray;
    tray.show();

    if (!auth_is_logged_in())
    {
        QMessageBox::information(
            nullptr,
            "JustInTime",
            "Welcome to JustInTime!\n\n"
            "Your activity is still being tracked and saved locally.\n"
            "Log in via the system tray icon's right-click menu\n"
            "to sync your data to the cloud."
        );
    }

    std::thread worker(workerLoop, &tray);

    int ret = app.exec();

    /*
     * Thoát: dừng worker thread trước, join xong mới đụng
     * tới database từ main thread để tránh tranh chấp.
     */
    g_workerRunning = false;

    if (worker.joinable())
        worker.join();

    remoteview_stop();

    sync_pending_records();
    backup_create_snapshot();
    db_print_daily_summary();
    db_close();

    return ret;
}
