//
// Created by LoPhongCorporation on 6/24/2026.
// Rewritten: gui du lieu qua Supabase Edge Function
// "sync-activity" (thay vi goi thang PostgREST) de
// service_role key khong bao gio phai nam trong client.
//

#include "../include/network.h"
#include "../include/config.h"
#include "../include/jsonutil.h"

#include <windows.h>
#include <winhttp.h>

#include <stdio.h>
#include <string.h>
#include <wchar.h>

#pragma comment(lib, "winhttp.lib")

/*
 * Tach phan host ra khoi SUPABASE_URL dang
 * "https://xxxx.supabase.co" -> "xxxx.supabase.co"
 */
static void extract_host(
    const char* url,
    wchar_t* host_out,
    int host_out_size)
{
    const char* p =
        strstr(url, "://");

    const char* host =
        p ? p + 3 : url;

    MultiByteToWideChar(
        CP_UTF8,
        0,
        host,
        -1,
        host_out,
        host_out_size
    );
}

/*
 * Chuyen mot SyncRecord thanh JSON body de POST len
 * bang activity_logs tren Supabase.
 * Khong gui "id" cuc bo, vi id nay chi co y nghia
 * trong SQLite cua tung may, khong phai khoa chung.
 */
static int build_json_body(
    const SyncRecord* rec,
    char* out,
    int out_size)
{
    char process_utf8[1024] = {0};
    char title_utf8[4096] = {0};

    WideCharToMultiByte(
        CP_UTF8, 0,
        rec->process_name, -1,
        process_utf8, sizeof(process_utf8),
        NULL, NULL
    );

    WideCharToMultiByte(
        CP_UTF8, 0,
        rec->window_title, -1,
        title_utf8, sizeof(title_utf8),
        NULL, NULL
    );

    char device_esc[256]   = {0};
    char process_esc[2048] = {0};
    char title_esc[8192]   = {0};

    json_escape(rec->device_id, device_esc, sizeof(device_esc));
    json_escape(process_utf8,   process_esc, sizeof(process_esc));
    json_escape(title_utf8,     title_esc,   sizeof(title_esc));

    return snprintf(
        out,
        out_size,
        "{"
        "\"device_id\":\"%s\","
        "\"process_name\":\"%s\","
        "\"window_title\":\"%s\","
        "\"duration_seconds\":%ld,"
        "\"start_time\":%lld,"
        "\"end_time\":%lld"
        "}",
        device_esc,
        process_esc,
        title_esc,
        rec->duration_seconds,
        rec->start_time,
        rec->end_time
    );
}

/*
 * Gui mot record len Edge Function "sync-activity" qua HTTPS.
 * Function ben server se dung service_role key (khong lo ra
 * client) de upsert vao bang activity_logs, dua tren
 * (device_id, start_time) de tranh trung du lieu neu record
 * da duoc gui thanh cong truoc do nhung chua kip danh dau
 * "synced" cuc bo.
 *
 * Yeu cau: da deploy Edge Function sync-activity, va bang
 * activity_logs co UNIQUE constraint tren (device_id, start_time).
 */
int network_send_record(
    const SyncRecord* rec)
{
    if (!rec)
        return 0;

    char body[16384] = {0};

    int body_len =
        build_json_body(
            rec,
            body,
            sizeof(body)
        );

    if (body_len <= 0 || body_len >= (int)sizeof(body))
    {
        wprintf(L"[SYNC] Khong the tao JSON body (qua dai hoac loi)\n");
        return 0;
    }

    wchar_t host[256] = {0};

    extract_host(
        SUPABASE_URL,
        host,
        256
    );

    wchar_t apikey[2048] = {0};

    MultiByteToWideChar(
        CP_UTF8, 0,
        SUPABASE_ANON_KEY, -1,
        apikey, 2048
    );

    int success = 0;

    HINTERNET hSession = WinHttpOpen(
        L"JustInTime-Agent/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );

    if (!hSession)
    {
        wprintf(L"[SYNC] WinHttpOpen that bai (%lu)\n", GetLastError());
        return 0;
    }

    HINTERNET hConnect = WinHttpConnect(
        hSession,
        host,
        INTERNET_DEFAULT_HTTPS_PORT,
        0
    );

    if (hConnect)
    {
        const wchar_t* path = L"/functions/v1/sync-activity";

        HINTERNET hRequest = WinHttpOpenRequest(
            hConnect,
            L"POST",
            path,
            NULL,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_SECURE
        );

        if (hRequest)
        {
            wchar_t headers[4096];

            swprintf(
                headers,
                4096,
                L"Content-Type: application/json\r\n"
                L"apikey: %ls\r\n"
                L"Authorization: Bearer %ls\r\n",
                apikey,
                apikey
            );

            BOOL sent = WinHttpSendRequest(
                hRequest,
                headers,
                (DWORD)-1,
                (LPVOID)body,
                (DWORD)body_len,
                (DWORD)body_len,
                0
            );

            if (sent && WinHttpReceiveResponse(hRequest, NULL))
            {
                DWORD status = 0;
                DWORD status_size = sizeof(status);

                WinHttpQueryHeaders(
                    hRequest,
                    WINHTTP_QUERY_FLAG_NUMBER | WINHTTP_QUERY_STATUS_CODE,
                    WINHTTP_HEADER_NAME_BY_INDEX,
                    &status,
                    &status_size,
                    WINHTTP_NO_HEADER_INDEX
                );

                if (status >= 200 && status < 300)
                {
                    success = 1;
                }
                else
                {
                    /*
                     * Đọc body thực tế để biết chính xác
                     * Supabase từ chối vì lý do gì
                     * (thay vì chỉ biết mã HTTP).
                     */
                    char resp_body[4096] = {0};
                    DWORD total_read = 0;

                    for (;;)
                    {
                        DWORD available = 0;

                        if (
                            !WinHttpQueryDataAvailable(hRequest, &available)
                            || available == 0
                        )
                            break;

                        if (available > sizeof(resp_body) - 1 - total_read)
                            available = sizeof(resp_body) - 1 - total_read;

                        if (available == 0)
                            break;

                        DWORD bytes_read = 0;

                        if (
                            !WinHttpReadData(
                                hRequest,
                                resp_body + total_read,
                                available,
                                &bytes_read
                            )
                            || bytes_read == 0
                        )
                            break;

                        total_read += bytes_read;
                    }

                    resp_body[total_read] = '\0';

                    wprintf(
                        L"[SYNC] Supabase tra ve loi HTTP %lu: %hs\n",
                        status,
                        resp_body
                    );
                }
            }
            else
            {
                wprintf(
                    L"[SYNC] Gui request that bai (%lu)\n",
                    GetLastError()
                );
            }

            WinHttpCloseHandle(hRequest);
        }
        else
        {
            wprintf(L"[SYNC] WinHttpOpenRequest that bai (%lu)\n", GetLastError());
        }

        WinHttpCloseHandle(hConnect);
    }
    else
    {
        wprintf(L"[SYNC] WinHttpConnect that bai (%lu)\n", GetLastError());
    }

    WinHttpCloseHandle(hSession);

    return success;
}
