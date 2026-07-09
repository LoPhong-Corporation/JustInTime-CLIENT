//
// Created by LoPhongCorporation on 6/24/2026.
//

#include "../include/sync.h"
#include "../include/database.h"
#include "../include/network.h"

#include <stdio.h>

void sync_pending_records(void)
{
    SyncRecord records[MAX_RECORDS];

    int count =
        db_get_unsynced_records(
            records,
            MAX_RECORDS
        );

    if (count == 0)
    {
        wprintf(
            L"[SYNC] No pending records\n"
        );
        return;
    }

    wprintf(
        L"[SYNC] Found %d record(s)\n",
        count
    );

    for (int i = 0; i < count; i++)
    {
        if (network_send_record(&records[i]))
        {
            if (
                db_mark_record_synced(
                    records[i].id
                )
            )
            {
                wprintf(
                    L"[SYNC] Record %d synced\n",
                    records[i].id
                );
            }
            else
            {
                wprintf(
                    L"[SYNC] Failed to update database for record %d\n",
                    records[i].id
                );
            }
        }
        else
        {
            wprintf(
                L"[SYNC] Send failed: %d\n",
                records[i].id
            );
        }
    }
}