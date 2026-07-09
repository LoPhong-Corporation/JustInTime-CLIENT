//
// Created by LoPhongCorporation on 6/24/2026.
//

#ifndef ACTIVITY_H
#define ACTIVITY_H

#include <windows.h>
#include <time.h>

typedef struct
{
    wchar_t process_name[MAX_PATH];
    wchar_t window_title[512];
} ActiveWindow;

typedef struct
{
    wchar_t process_name[MAX_PATH];
    wchar_t window_title[512];

    time_t start_time;
    time_t end_time;

    long duration_seconds;

    int synced;
} ActivityRecord;

void monitor_activity(void);

#endif