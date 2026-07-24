//
// Created by LoPhongCorporation on 6/24/2026.
//
#include "../include/activity.h"
#include "../include/database.h"
#include "../include/settings.h"

#include <windows.h>
#include <psapi.h>

#include <stdio.h>
#include <wchar.h>
#include <time.h>

static ActiveWindow g_last_window = {0};
static ActivityRecord g_current_record = {0};

static CRITICAL_SECTION g_current_lock;
static int g_current_lock_ready = 0;

static void ensure_current_lock(void)
{
    if (!g_current_lock_ready)
    {
        InitializeCriticalSection(&g_current_lock);
        g_current_lock_ready = 1;
    }
}

/*
 * Lấy snapshot activity đang diễn ra NGAY LÚC NÀY (dùng
 * cho remote view - xem real-time không qua cloud).
 * Thread-safe: có thể gọi từ thread khác (HTTP server thread)
 * trong khi worker thread vẫn đang cập nhật g_current_record.
 */
void activity_get_current(
    wchar_t* process_out, int process_size,
    wchar_t* title_out, int title_size,
    time_t* since_out)
{
    ensure_current_lock();

    EnterCriticalSection(&g_current_lock);

    if (process_out && process_size > 0)
        wcscpy_s(process_out, process_size, g_current_record.process_name);

    if (title_out && title_size > 0)
        wcscpy_s(title_out, title_size, g_current_record.window_title);

    if (since_out)
        *since_out = g_current_record.start_time;

    LeaveCriticalSection(&g_current_lock);
}

/*
 * Lấy thông tin cửa sổ hiện tại
 */
static int get_active_window_info(
    ActiveWindow* window)
{
    if (!window)
        return 0;

    HWND hwnd = GetForegroundWindow();

    if (!hwnd)
        return 0;

    GetWindowTextW(
        hwnd,
        window->window_title,
        sizeof(window->window_title) /
        sizeof(window->window_title[0])
    );

    DWORD pid = 0;

    GetWindowThreadProcessId(
        hwnd,
        &pid
    );

    HANDLE process = OpenProcess(
        PROCESS_QUERY_INFORMATION |
        PROCESS_VM_READ,
        FALSE,
        pid
    );

    if (!process)
        return 0;

    int success = GetModuleBaseNameW(
        process,
        NULL,
        window->process_name,
        MAX_PATH
    );

    CloseHandle(process);

    return success;
}

/*
 * In activity vừa hoàn thành
 */
static void print_activity_report(
    const ActivityRecord* record)
{
    long total =
        record->duration_seconds;

    long hours =
        total / 3600;

    long minutes =
        (total % 3600) / 60;

    long seconds =
        total % 60;

    wprintf(
        L"\n"
        L"=====================================\n"
        L"[ACTIVITY]\n"
        L"Process : %ls\n"
        L"Title   : %ls\n"
        L"Used    : %02ld:%02ld:%02ld\n"
        L"=====================================\n",
        record->process_name,
        record->window_title,
        hours,
        minutes,
        seconds
    );
}

/*
 * Bắt đầu activity mới
 */
static void start_new_record(
    const ActiveWindow* window)
{
    ensure_current_lock();
    EnterCriticalSection(&g_current_lock);

    wcscpy_s(
        g_current_record.process_name,
        MAX_PATH,
        window->process_name
    );

    wcscpy_s(
        g_current_record.window_title,
        512,
        window->window_title
    );

    g_current_record.start_time =
        time(NULL);

    g_current_record.end_time = 0;

    g_current_record.duration_seconds = 0;

    g_current_record.synced = 0;

    LeaveCriticalSection(&g_current_lock);
}

/*
 * Kết thúc activity hiện tại
 */
static void finish_current_record(void)
{
    ensure_current_lock();
    EnterCriticalSection(&g_current_lock);

    g_current_record.end_time =
        time(NULL);

    g_current_record.duration_seconds =
        (long)(
            g_current_record.end_time -
            g_current_record.start_time
        );

    LeaveCriticalSection(&g_current_lock);

    /*
     * Bỏ qua app nằm trong danh sách loại trừ (cấu hình
     * trong menu Cài đặt của tray).
     */
    if (
        settings_is_process_excluded(
            g_current_record.process_name
        )
    )
        return;

    /*
     * Bỏ qua record quá ngắn (ngưỡng lấy từ settings,
     * mặc định 2 giây).
     */
    AppSettings s;
    settings_get(&s);

    if (g_current_record.duration_seconds < s.min_duration_sec)
        return;

    print_activity_report(
        &g_current_record
    );

    db_insert_activity(
        &g_current_record
    );
}

/*
 * Theo dõi activity
 */
void monitor_activity(void)
{
    ActiveWindow current = {0};

    if (!get_active_window_info(
            &current))
    {
        return;
    }

    /*
     * Lần chạy đầu tiên
     */
    if (
        g_last_window.process_name[0]
        == L'\0'
    )
    {
        g_last_window = current;

        start_new_record(
            &current
        );

        wprintf(
            L"[START] %ls\n",
            current.process_name
        );

        return;
    }

    int process_changed =
        wcscmp(
            current.process_name,
            g_last_window.process_name
        ) != 0;

    int title_changed =
        wcscmp(
            current.window_title,
            g_last_window.window_title
        ) != 0;

    /*
     * Nếu không thay đổi thì bỏ qua
     */
    if (
        !process_changed &&
        !title_changed
    )
    {
        return;
    }

    /*
     * Kết thúc record cũ
     */
    finish_current_record();

    /*
     * Log chuyển app
     */
    wprintf(
        L"\n"
        L"=====================================\n"
        L"[SWITCH]\n"
        L"Process : %ls\n"
        L"Title   : %ls\n"
        L"=====================================\n",
        current.process_name,
        current.window_title
    );

    /*
     * Record mới
     */
    start_new_record(
        &current
    );

    g_last_window = current;
}