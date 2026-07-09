//
// backup.c
// Backup dữ liệu cục bộ ra file JSON, tách biệt
// hoàn toàn với việc đồng bộ lên Supabase, để đảm
// bảo luôn có ít nhất một bản sao dữ liệu an toàn
// ngay cả khi mất kết nối mạng dài ngày.
//

#include "../include/backup.h"
#include "../include/database.h"
#include "../include/config.h"

#include <windows.h>

#include <stdio.h>
#include <string.h>
#include <time.h>

/*
 * Đảm bảo thư mục backup tồn tại.
 */
static void ensure_backup_dir(void)
{
    CreateDirectoryA(
        BACKUP_DIR,
        NULL
    );
}

/*
 * Xóa bớt các file backup cũ, chỉ giữ lại
 * BACKUP_KEEP_COUNT file gần nhất.
 * Vì tên file có định dạng backup_YYYYMMDD_HHMMSS.json
 * nên sắp xếp theo thứ tự chữ cái cũng chính là
 * sắp xếp theo thời gian.
 */
static void cleanup_old_backups(void)
{
    char pattern[MAX_PATH];

    snprintf(
        pattern,
        sizeof(pattern),
        "%s\\backup_*.json",
        BACKUP_DIR
    );

    char names[512][MAX_PATH];
    int count = 0;

    WIN32_FIND_DATAA fd;

    HANDLE h = FindFirstFileA(
        pattern,
        &fd
    );

    if (h == INVALID_HANDLE_VALUE)
        return;

    do
    {
        if (
            !(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            count < 512
        )
        {
            snprintf(
                names[count],
                MAX_PATH,
                "%s",
                fd.cFileName
            );

            count++;
        }
    } while (FindNextFileA(h, &fd));

    FindClose(h);

    /*
     * Sắp xếp tăng dần (insertion sort, count nhỏ nên ổn).
     */
    for (int i = 1; i < count; i++)
    {
        char key[MAX_PATH];

        snprintf(key, MAX_PATH, "%s", names[i]);

        int j = i - 1;

        while (j >= 0 && strcmp(names[j], key) > 0)
        {
            snprintf(names[j + 1], MAX_PATH, "%s", names[j]);
            j--;
        }

        snprintf(names[j + 1], MAX_PATH, "%s", key);
    }

    int to_delete = count - BACKUP_KEEP_COUNT;

    for (int i = 0; i < to_delete; i++)
    {
        char full_path[MAX_PATH];

        snprintf(
            full_path,
            sizeof(full_path),
            "%s\\%s",
            BACKUP_DIR,
            names[i]
        );

        DeleteFileA(full_path);
    }
}

int backup_create_snapshot(void)
{
    ensure_backup_dir();

    time_t now = time(NULL);
    struct tm t;

    localtime_s(&t, &now);

    char filepath[MAX_PATH];

    snprintf(
        filepath,
        sizeof(filepath),
        "%s\\backup_%04d%02d%02d_%02d%02d%02d.json",
        BACKUP_DIR,
        t.tm_year + 1900,
        t.tm_mon + 1,
        t.tm_mday,
        t.tm_hour,
        t.tm_min,
        t.tm_sec
    );

    int ok = db_export_json(filepath);

    if (ok)
    {
        wprintf(
            L"[BACKUP] Da sao luu du lieu vao %hs\n",
            filepath
        );

        cleanup_old_backups();
    }
    else
    {
        wprintf(
            L"[BACKUP] Sao luu that bai\n"
        );
    }

    return ok;
}
