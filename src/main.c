//
// Created by LoPhongCorporation on 6/24/2026.
// Updated: them backup dinh ky, don dep an toan, va
// xu ly tat chuong trinh (Ctrl+C / dong cua so) khong
// mat du lieu.
//

#include "../include/activity.h"
#include "../include/database.h"
#include "../include/sync.h"
#include "../include/backup.h"
#include "../include/config.h"

#include <windows.h>

#include <stdio.h>
#include <time.h>

#include <locale.h>
#include <io.h>
#include <fcntl.h>

#define MONITOR_INTERVAL_MS   1000
#define SYNC_INTERVAL_SEC      30
#define BACKUP_INTERVAL_SEC  3600

/*
 * Cờ điều khiển vòng lặp chính. volatile vì có thể
 * bị thay đổi từ luồng xử lý tín hiệu console.
 */
static volatile BOOL g_running = TRUE;

/*
 * Xử lý các sự kiện điều khiển console (Ctrl+C, đóng
 * cửa sổ, logoff, shutdown) để đảm bảo không mất dữ
 * liệu khi người dùng tắt agent.
 *
 * - Với CTRL_C/CTRL_BREAK: chỉ đặt cờ dừng, để vòng
 *   lặp chính tự thoát và dọn dẹp (an toàn hơn vì
 *   không truy cập DB đồng thời từ 2 luồng).
 * - Với CLOSE/LOGOFF/SHUTDOWN: Windows chỉ cho một
 *   khoảng thời gian ngắn trước khi buộc kết thúc tiến
 *   trình, nên phải đồng bộ + backup + đóng DB ngay
 *   trong handler này.
 */
static BOOL WINAPI console_handler(DWORD signal)
{
    switch (signal)
    {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
            g_running = FALSE;
            return TRUE;

        case CTRL_CLOSE_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            wprintf(L"\n[SYSTEM] Dang dung agent, dong bo lan cuoi...\n");

            g_running = FALSE;

            sync_pending_records();
            backup_create_snapshot();
            db_close();

            return TRUE;

        default:
            return FALSE;
    }
}

int main(void)
{
    /*
     * Hỗ trợ Unicode trên Windows Console
     */
    _setmode(
        _fileno(stdout),
        _O_U16TEXT
    );

    setlocale(
        LC_ALL,
        ""
    );

    wprintf(
        L"[CORE][SUCCESS] JustInTime Agent Started\n"
    );

    /*
     * Khởi tạo database
     */
    if (!db_init())
    {
        wprintf(
            L"[CORE][ERROR][001] Database initialization failed.\n"
        );

        return 1;
    }

    /*
     * Đăng ký xử lý tắt an toàn
     */
    SetConsoleCtrlHandler(
        console_handler,
        TRUE
    );

    /*
     * Backup ngay khi khởi động để luôn có bản sao
     * lưu mới nhất, kể cả khi agent chưa chạy được lâu.
     */
    backup_create_snapshot();

    time_t last_sync =
        time(NULL);

    time_t last_backup =
        time(NULL);

    /*
     * Main Loop
     */
    while (g_running)
    {
        /*
         * Theo dõi cửa sổ đang hoạt động
         */
        monitor_activity();

        time_t now =
            time(NULL);

        /*
         * Đồng bộ theo chu kỳ lên Supabase
         */
        if (
            now - last_sync
            >= SYNC_INTERVAL_SEC
        )
        {
            sync_pending_records();

            last_sync = now;
        }

        /*
         * Backup cục bộ theo chu kỳ + dọn dẹp
         * bản ghi ĐÃ đồng bộ quá hạn giữ lại.
         */
        if (
            now - last_backup
            >= BACKUP_INTERVAL_SEC
        )
        {
            backup_create_snapshot();

            db_delete_old_records(
                RETENTION_DAYS
            );

            last_backup = now;
        }

        /*
         * Giảm tải CPU
         */
        Sleep(
            MONITOR_INTERVAL_MS
        );
    }

    /*
     * Thoát vòng lặp do CTRL_C/CTRL_BREAK: đồng bộ,
     * backup lần cuối rồi mới đóng database.
     */
    sync_pending_records();
    backup_create_snapshot();
    db_close();

    return 0;
}
