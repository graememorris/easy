/*
this should compile with gcc controller01.c animation_analyzer.c -o visual_inspector -lsqlite3 -lavformat -lavcodec -lavutil
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>      // For usleep/sleep throttling
#include <sqlite3.h>     // SQLite3 C API

// Include your existing header file exactly as it is named
#include "getanim.h" 

// Function Prototypes
int process_single_file(sqlite3 *db, sqlite3_stmt *select_stmt, sqlite3_stmt *insert_stmt, sqlite3_stmt *update_stmt);
int setup_statements(sqlite3 *db, sqlite3_stmt **select_stmt, sqlite3_stmt **insert_stmt, sqlite3_stmt **update_stmt);
void finalize_statements(sqlite3_stmt *select_stmt, sqlite3_stmt *insert_stmt, sqlite3_stmt *update_stmt);

int main() {
    sqlite3 *db;
    sqlite3_stmt *select_stmt = NULL;
    sqlite3_stmt *insert_stmt = NULL;
    sqlite3_stmt *update_stmt = NULL;

    // Open your SQLite database file
    if (sqlite3_open("/home/gm/Pictures/.x/testdb.sql", &db) != SQLITE_OK) {
        fprintf(stderr, "FATAL: Cannot open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    // Ensure the 'dims' table exists and links back to 'files' via an ID column
    const char *create_table_sql = 
        "CREATE TABLE IF NOT EXISTS dims ("
        "  id INTEGER PRIMARY KEY," // Maps 1:1 to files(id)
        "  width INTEGER,"
        "  height INTEGER,"
        "  n_frames INTEGER,"
        "  codec TEXT,"
        "  n_video_streams INTEGER,"
        "  n_audio_streams INTEGER,"
        "  FOREIGN KEY(id) REFERENCES files(id)"
        ");";
    
    if (sqlite3_exec(db, create_table_sql, NULL, NULL, NULL) != SQLITE_OK) {
        fprintf(stderr, "FATAL: Failed to verify 'dims' table: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    // Prepare SQL queries once in memory for ultra-low CPU overhead
    if (!setup_statements(db, &select_stmt, &insert_stmt, &update_stmt)) {
        sqlite3_close(db);
        return 1;
    }

    printf("Visual Inspector engine initialized. Processing queue safely...\n");

    // Main background loop
    while (1) {
        // Process a file if one is available
        int file_processed = process_single_file(db, select_stmt, insert_stmt, update_stmt);
        
        if (file_processed == 1) {
            // THROTTLE: Rest for 250ms between files to protect drive and CPU
            usleep(250000); 
        } else if (file_processed == 0) {
            // QUEUE EMPTY: Sleep longer (5 seconds) before checking again
            sleep(5);
        } else {
            // CRITICAL ERROR: Break out safely to prevent infinite looping
            fprintf(stderr, "Encountered critical database error. Halting.\n");
            break;
        }
    }

    // Clean up memory before exit
    finalize_statements(select_stmt, insert_stmt, update_stmt);
    sqlite3_close(db);
    return 0;
}

int setup_statements(sqlite3 *db, sqlite3_stmt **select_stmt, sqlite3_stmt **insert_stmt, sqlite3_stmt **update_stmt) {
    // Pull files that do not exist in the 'dims' table yet
    const char *select_sql = 
        "SELECT f.id, f.path, f.name FROM files f "
        "LEFT JOIN dims d ON f.id = d.id "
        "WHERE d.id IS NULL LIMIT 1;";

    // Insert metadata into the dims table
    const char *insert_sql = 
        "INSERT OR REPLACE INTO dims (id, width, height, n_frames, codec, n_video_streams, n_audio_streams) "
        "VALUES (?, ?, ?, ?, ?, ?, ?);";

    // If a file is completely unreadable or broken, insert placeholder values 
    // to mark it as processed so it isn't repeatedly scanned in an infinite loop
    const char *update_sql = 
        "INSERT OR REPLACE INTO dims (id, width, height, n_frames, codec, n_video_streams, n_audio_streams) "
        "VALUES (?, -1, -1, -1, 'FAILED', 0, 0);";

    if (sqlite3_prepare_v2(db, select_sql, -1, select_stmt, NULL) != SQLITE_OK ||
        sqlite3_prepare_v2(db, insert_sql, -1, insert_stmt, NULL) != SQLITE_OK ||
        sqlite3_prepare_v2(db, update_sql, -1, update_stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "SQL Statement Preparation Failed: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    return 1;
}

int process_single_file(sqlite3 *db, sqlite3_stmt *select_stmt, sqlite3_stmt *insert_stmt, sqlite3_stmt *update_stmt) {
    int result = 0; // 0 = no files, 1 = file processed, -1 = database error

    // Execute the query to find a pending file path
    int rc = sqlite3_step(select_stmt);
    
    if (rc == SQLITE_ROW) {
		int file_id = sqlite3_column_int(select_stmt, 0);
        const char *file_path_dir = (const char *)sqlite3_column_text(select_stmt, 1);
        const char *file_name = (const char *)sqlite3_column_text(select_stmt, 2);
        
        // Create a buffer to hold the combined full path
        char full_path[4096]; 
        snprintf(full_path, sizeof(full_path), "%s/%s", file_path_dir, file_name);
        
        printf("Safely reading metadata: %s\n", full_path);
        
        struct AnimationProperties props;
        memset(&props, 0, sizeof(struct AnimationProperties));

        // Call your existing function inside AnimationAnalyzer.c
        // 0 = Success, non-zero = Failure
        int scan_status = extract_animation_properties(full_path, &props);

        // Secure database transactions to protect integrity against mid-write power drops
        sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);

        if (scan_status == 0) {
            // Bind extracted data to the parameters safely (prevents quote errors)
            sqlite3_bind_int(insert_stmt, 1, file_id);
            sqlite3_bind_int(insert_stmt, 2, props.width);
            sqlite3_bind_int(insert_stmt, 3, props.height);
            sqlite3_bind_int(insert_stmt, 4, props.frame_count); // Adjust to match your exact struct field
            sqlite3_bind_text(insert_stmt, 5, props.codec_name, -1, SQLITE_TRANSIENT); // Adjust field name
            sqlite3_bind_int(insert_stmt, 6, props.video_stream_count); // Adjust field name
            sqlite3_bind_int(insert_stmt, 7, props.audio_stream_count); // Adjust field name

            sqlite3_step(insert_stmt);
            sqlite3_reset(insert_stmt);
            sqlite3_clear_bindings(insert_stmt);
        } else {
            // File is unreadable by libavformat, write placeholder failures
            fprintf(stderr, "WARNING: File unreadable or corrupted. Flagging ID %d.\n", file_id);
            sqlite3_bind_int(update_stmt, 1, file_id);
            sqlite3_step(update_stmt);
            sqlite3_reset(update_stmt);
            sqlite3_clear_bindings(update_stmt);
        }

        sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
        result = 1; // File processed successfully
    } else if (rc == SQLITE_DONE) {
        result = 0; // No pending files left in the queue
    } else {
        result = -1; // Database error occurred
    }

    // Reset the query so it can execute freshly on the next loop iteration
    sqlite3_reset(select_stmt);
    return result;
}

void finalize_statements(sqlite3_stmt *select_stmt, sqlite3_stmt *insert_stmt, sqlite3_stmt *update_stmt) {
    if (select_stmt) sqlite3_finalize(select_stmt);
    if (insert_stmt) sqlite3_finalize(insert_stmt);
    if (update_stmt) sqlite3_finalize(update_stmt);
}
