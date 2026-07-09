//
// Created by LoPhongCorporation on 6/24/2026.
//
#include "../include/database.h"
#include "../include/device.h"
#include "../include/jsonutil.h"

#include "../third_party/sqlite/sqlite3.h"

#include <windows.h>

#include <stdio.h>

static sqlite3* g_db = NULL;

/*
 * UTF16 -> UTF8
 */
static void wide_to_utf8(
    const wchar_t* wide,
    char* utf8,
    int utf8_size)
{
    WideCharToMultiByte(
        CP_UTF8,
        0,
        wide,
        -1,
        utf8,
        utf8_size,
        NULL,
        NULL
    );
}

static void utf8_to_wide(
    const char* utf8,
    wchar_t* wide,
    int wide_size)
{
    if (!utf8)
    {
        wide[0] = L'\0';
        return;
    }

    MultiByteToWideChar(
        CP_UTF8,
        0,
        utf8,
        -1,
        wide,
        wide_size
    );
}

/*
 * Khởi tạo database
 */
int db_init(void)
{
    int rc = sqlite3_open(
        "justintime.db",
        &g_db
    );

    if (rc != SQLITE_OK)
    {
        wprintf(
            L"Failed to open database\n"
        );

        return 0;
    }

    const char* sql =
        "CREATE TABLE IF NOT EXISTS activity_logs ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"

        "device_id TEXT NOT NULL,"

        "process_name TEXT NOT NULL,"
        "window_title TEXT NOT NULL,"

        "duration_seconds INTEGER NOT NULL,"

        "start_time INTEGER NOT NULL,"
        "end_time INTEGER NOT NULL,"

        "synced INTEGER DEFAULT 0,"

        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");";

    char* error = NULL;

    rc = sqlite3_exec(
        g_db,
        sql,
        NULL,
        NULL,
        &error
    );

    if (rc != SQLITE_OK)
    {
        wprintf(
            L"Create table failed: %hs\n",
            error
        );

        sqlite3_free(error);

        return 0;
    }

    wprintf(
        L"Database initialized\n"
    );

    return 1;
}

/*
 * Lưu activity
 */
int db_insert_activity(
    const ActivityRecord* record)
{
    if (!record)
        return 0;

    char device_id[128] = {0};

    get_device_id(
        device_id,
        sizeof(device_id)
    );

    char process_utf8[512] = {0};
    char title_utf8[2048] = {0};

    wide_to_utf8(
        record->process_name,
        process_utf8,
        sizeof(process_utf8)
    );

    wide_to_utf8(
        record->window_title,
        title_utf8,
        sizeof(title_utf8)
    );

    const char* sql =
        "INSERT INTO activity_logs ("
        "device_id,"
        "process_name,"
        "window_title,"
        "duration_seconds,"
        "start_time,"
        "end_time,"
        "synced"
        ") "
        "VALUES (?, ?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt = NULL;

    int rc = sqlite3_prepare_v2(
        g_db,
        sql,
        -1,
        &stmt,
        NULL
    );

    if (rc != SQLITE_OK)
    {
        wprintf(
            L"Prepare failed: %hs\n",
            sqlite3_errmsg(g_db)
        );

        return 0;
    }

    sqlite3_bind_text(
        stmt,
        1,
        device_id,
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_text(
        stmt,
        2,
        process_utf8,
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_text(
        stmt,
        3,
        title_utf8,
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_int64(
        stmt,
        4,
        record->duration_seconds
    );

    sqlite3_bind_int64(
        stmt,
        5,
        (sqlite3_int64)record->start_time
    );

    sqlite3_bind_int64(
        stmt,
        6,
        (sqlite3_int64)record->end_time
    );

    sqlite3_bind_int(
        stmt,
        7,
        record->synced
    );

    rc = sqlite3_step(
        stmt
    );

    sqlite3_finalize(
        stmt
    );

    if (rc != SQLITE_DONE)
    {
        wprintf(
            L"Insert failed: %hs\n",
            sqlite3_errmsg(g_db)
        );

        return 0;
    }

    wprintf(
        L"Activity saved\n"
    );

    return 1;
}

/*
 * Đóng database
 */
void db_close(void)
{
    if (g_db)
    {
        sqlite3_close(
            g_db
        );

        g_db = NULL;
    }
}

/*
 * Database print unsynced
 */
void db_print_unsynced(void)
{
    if (!g_db)
        return;

    const char* sql =
        "SELECT id, process_name, duration_seconds "
        "FROM activity_logs "
        "WHERE synced = 0;";

    sqlite3_stmt* stmt = NULL;

    if (sqlite3_prepare_v2(
            g_db,
            sql,
            -1,
            &stmt,
            NULL
        ) != SQLITE_OK)
    {
        return;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        int id =
            sqlite3_column_int(stmt, 0);

        const char* process_utf8 =
            (const char*)sqlite3_column_text(stmt, 1);

        wchar_t process[512];

        utf8_to_wide(
            process_utf8,
            process,
            512
        );

        int duration =
            sqlite3_column_int(stmt, 2);

        wprintf(
            L"[UNSYNCED] id=%d process=%ls duration=%d\n",
            id,
            process,
            duration
        );
    }

    sqlite3_finalize(stmt);
}
int db_get_unsynced_records(
    SyncRecord* records,
    int max_records)
{
    if (!g_db)
        return 0;

    const char* sql =
        "SELECT "
        "id,"
        "device_id,"
        "process_name,"
        "window_title,"
        "duration_seconds,"
        "start_time,"
        "end_time "
        "FROM activity_logs "
        "WHERE synced = 0 "
        "LIMIT ?;";

    sqlite3_stmt* stmt = NULL;

    if (sqlite3_prepare_v2(
            g_db,
            sql,
            -1,
            &stmt,
            NULL
        ) != SQLITE_OK)
    {
        return 0;
    }

    sqlite3_bind_int(
        stmt,
        1,
        max_records
    );

    int count = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW &&
           count < max_records)
    {
        SyncRecord* rec =
            &records[count];

        rec->id =
            sqlite3_column_int(stmt, 0);

        snprintf(
            rec->device_id,
            sizeof(rec->device_id),
            "%s",
            sqlite3_column_text(stmt, 1)
        );

        utf8_to_wide(
            (const char*)sqlite3_column_text(stmt, 2),
            rec->process_name,
            512
        );

        utf8_to_wide(
            (const char*)sqlite3_column_text(stmt, 3),
            rec->window_title,
            2048
        );

        rec->duration_seconds =
            sqlite3_column_int64(stmt, 4);

        rec->start_time =
            sqlite3_column_int64(stmt, 5);

        rec->end_time =
            sqlite3_column_int64(stmt, 6);

        count++;
    }

    sqlite3_finalize(stmt);

    return count;
}

int db_mark_synced(int id)
{
    const char* sql =
        "UPDATE activity_logs "
        "SET synced = 1 "
        "WHERE id = ?;";

    sqlite3_stmt* stmt = NULL;

    if (
        sqlite3_prepare_v2(
            g_db,
            sql,
            -1,
            &stmt,
            NULL
        ) != SQLITE_OK
    )
    {
        return 0;
    }

    sqlite3_bind_int(
        stmt,
        1,
        id
    );

    int rc =
        sqlite3_step(stmt);

    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

int db_count_unsynced(void)
{
    const char* sql =
        "SELECT COUNT(*) "
        "FROM activity_logs "
        "WHERE synced = 0;";

    sqlite3_stmt* stmt = NULL;

    if (
        sqlite3_prepare_v2(
            g_db,
            sql,
            -1,
            &stmt,
            NULL
        ) != SQLITE_OK
    )
    {
        return 0;
    }

    int count = 0;

    if (
        sqlite3_step(stmt)
        == SQLITE_ROW
    )
    {
        count =
            sqlite3_column_int(
                stmt,
                0
            );
    }

    sqlite3_finalize(stmt);

    return count;
}

int db_delete_old_records(
    int days)
{
    /*
     * QUAN TRỌNG: chỉ xóa các bản ghi ĐÃ synced=1.
     * Không bao giờ xóa dữ liệu chưa kịp đồng bộ lên
     * Supabase, để tránh mất dữ liệu vĩnh viễn.
     */
    const char* sql =
        "DELETE FROM activity_logs "
        "WHERE synced = 1 "
        "AND created_at "
        "< datetime('now', ?);";

    sqlite3_stmt* stmt = NULL;

    if (
        sqlite3_prepare_v2(
            g_db,
            sql,
            -1,
            &stmt,
            NULL
        ) != SQLITE_OK
    )
    {
        return 0;
    }

    char filter[32];

    snprintf(
        filter,
        sizeof(filter),
        "-%d day",
        days
    );

    sqlite3_bind_text(
        stmt,
        1,
        filter,
        -1,
        SQLITE_TRANSIENT
    );

    int rc =
        sqlite3_step(stmt);

    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

/*
 * Xuất TOÀN BỘ bảng activity_logs (kể cả đã sync
 * lẫn chưa sync) ra một file JSON, dùng cho mục
 * đích backup cục bộ, độc lập với việc đồng bộ
 * lên Supabase.
 */
int db_export_json(
    const char* filepath)
{
    if (!g_db || !filepath)
        return 0;

    FILE* f = fopen(
        filepath,
        "wb"
    );

    if (!f)
        return 0;

    const char* sql =
        "SELECT "
        "id, device_id, process_name, window_title, "
        "duration_seconds, start_time, end_time, "
        "synced, created_at "
        "FROM activity_logs "
        "ORDER BY id;";

    sqlite3_stmt* stmt = NULL;

    if (
        sqlite3_prepare_v2(
            g_db,
            sql,
            -1,
            &stmt,
            NULL
        ) != SQLITE_OK
    )
    {
        fclose(f);
        return 0;
    }

    fprintf(f, "[\n");

    int first = 1;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        if (!first)
            fprintf(f, ",\n");

        first = 0;

        int id =
            sqlite3_column_int(stmt, 0);

        const char* device_id =
            (const char*)sqlite3_column_text(stmt, 1);

        const char* process_name =
            (const char*)sqlite3_column_text(stmt, 2);

        const char* window_title =
            (const char*)sqlite3_column_text(stmt, 3);

        long duration =
            (long)sqlite3_column_int64(stmt, 4);

        long long start_time =
            sqlite3_column_int64(stmt, 5);

        long long end_time =
            sqlite3_column_int64(stmt, 6);

        int synced =
            sqlite3_column_int(stmt, 7);

        const char* created_at =
            (const char*)sqlite3_column_text(stmt, 8);

        char device_esc[256]   = {0};
        char process_esc[2048] = {0};
        char title_esc[8192]   = {0};
        char created_esc[64]   = {0};

        json_escape(device_id,     device_esc,  sizeof(device_esc));
        json_escape(process_name,  process_esc, sizeof(process_esc));
        json_escape(window_title,  title_esc,   sizeof(title_esc));
        json_escape(created_at,    created_esc, sizeof(created_esc));

        fprintf(
            f,
            "  {\"id\": %d, \"device_id\": \"%s\", "
            "\"process_name\": \"%s\", \"window_title\": \"%s\", "
            "\"duration_seconds\": %ld, \"start_time\": %lld, "
            "\"end_time\": %lld, \"synced\": %d, "
            "\"created_at\": \"%s\"}",
            id,
            device_esc,
            process_esc,
            title_esc,
            duration,
            start_time,
            end_time,
            synced,
            created_esc
        );
    }

    fprintf(f, "\n]\n");

    sqlite3_finalize(stmt);
    fclose(f);

    return 1;
}

/*
 * Đánh dấu record đã sync.
 */
int db_mark_record_synced(
    int id)
{
    if (!g_db)
        return 0;

    const char* sql =
        "UPDATE activity_logs "
        "SET synced = 1 "
        "WHERE id = ?;";

    sqlite3_stmt* stmt = NULL;

    int rc =
        sqlite3_prepare_v2(
            g_db,
            sql,
            -1,
            &stmt,
            NULL
        );

    if (rc != SQLITE_OK)
        return 0;

    sqlite3_bind_int(
        stmt,
        1,
        id
    );

    rc =
        sqlite3_step(
            stmt
        );

    sqlite3_finalize(
        stmt
    );

    return rc == SQLITE_DONE;
}