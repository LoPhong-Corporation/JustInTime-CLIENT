//
// Created by LoPhongCorporation on 6/24/2026.
//

#ifndef DATABASE_H
#define DATABASE_H
#include "sync.h"
#include "activity.h"

int db_init(void);

int db_insert_activity(
    const ActivityRecord* record
);

void db_print_unsynced(void);

void db_close(void);

int db_get_unsynced_records(
    SyncRecord* records,
    int max_records
);

int db_mark_synced(int id);

int db_count_unsynced(void);

int db_delete_old_records(int days);

int db_mark_record_synced(
    int id
);

/*
 * Xuất toàn bộ dữ liệu ra file JSON (backup cục bộ).
 * Trả về 1 nếu thành công, 0 nếu thất bại.
 */
int db_export_json(
    const char* filepath
);

#endif