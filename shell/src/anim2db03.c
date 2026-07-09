// compile with makefile @ ../
// program requires two parameters: the database file to read/write, and the query to use to filter which records to process
// it would be nice to make it sufficiently modular that a revision of some idtags in a music database could use the same framework as a set of dimensions for small image and animation files

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>      // For usleep/sleep throttling and getopt
#include <sqlite3.h>     // SQLite3 C API

#include "getanim.h"

int setup_statements(sqlite3 *db, sqlite3_stmt **select_stmt, sqlite3_stmt **insert_stmt, sqlite3_stmt **update_stmt, const char *scan_dir)
{
	// Modify select SQL if a specific directory path filtering is provided
	char select_sql[2048];
	if (scan_dir != NULL) {
	// Safe string formatting for target directory filtering
	snprintf(select_sql, sizeof(select_sql),
	"SELECT f.id, f.path, f.name FROM files f "
	"LEFT JOIN dims d ON f.id = d.id "
	"WHERE d.id IS NULL AND f.path = '%s' LIMIT 1;", scan_dir);
	} else {
	strcpy(select_sql, 
	"SELECT f.id, f.path, f.name FROM files f "
	"LEFT JOIN dims d ON f.id = d.id "
	"WHERE d.id IS NULL LIMIT 1;");
	}

	// Insert metadata into the dims table
	const char *insert_sql = 
	"INSERT OR REPLACE INTO dims (id, width, height, n_frames, codec, n_video_streams, n_audio_streams) "
	"VALUES (?, ?, ?, ?, ?, ?, ?);";

	// If a file is completely unreadable or broken, insert placeholder values 
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
	int result=0; // 0 = no files, 1 = file processed, -1 = database error
	int rc=sqlite3_step(select_stmt);
	if (rc==SQLITE_ROW){
		int file_id=sqlite3_column_int(select_stmt, 0);
		const char *file_path_dir=(const char *)sqlite3_column_text(select_stmt, 1);
		const char *file_name=(const char *)sqlite3_column_text(select_stmt, 2);
		char full_path[4096];
		snprintf(full_path, sizeof(full_path), "%s/%s", file_path_dir, file_name);
		printf("Safely reading metadata: %s\n", full_path);
		struct AnimationProperties props;
		memset(&props, 0, sizeof(struct AnimationProperties));
		int scan_status=extract_animation_properties(full_path, &props);
		sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
		if (scan_status==0){
			sqlite3_bind_int(insert_stmt, 1, file_id);
			sqlite3_bind_int(insert_stmt, 2, props.width);
			sqlite3_bind_int(insert_stmt, 3, props.height);
			sqlite3_bind_int(insert_stmt, 4, props.frame_count); 
			sqlite3_bind_text(insert_stmt, 5, props.codec_name, -1, SQLITE_TRANSIENT); 
			sqlite3_bind_int(insert_stmt, 6, props.video_stream_count); 
			sqlite3_bind_int(insert_stmt, 7, props.audio_stream_count); 

			sqlite3_step(insert_stmt);
			sqlite3_reset(insert_stmt);
			sqlite3_clear_bindings(insert_stmt);
		} else {
			fprintf(stderr, "WARNING: File unreadable or corrupted. Flagging ID %d.\n", file_id);
			sqlite3_bind_int(update_stmt, 1, file_id);
			sqlite3_step(update_stmt);
			sqlite3_reset(update_stmt);
			sqlite3_clear_bindings(update_stmt);
		}
		sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
		result=1; // File processed successfully
	} else if (rc == SQLITE_DONE) {
		result=0; // No pending files left in the queue
	} else {
		result=-1; // Database error occurred
	}
	sqlite3_reset(select_stmt);
	return result;
}

void finalize_statements(sqlite3_stmt *select_stmt, sqlite3_stmt *insert_stmt, sqlite3_stmt *update_stmt) {
    if (select_stmt) sqlite3_finalize(select_stmt);
    if (insert_stmt) sqlite3_finalize(insert_stmt);
    if (update_stmt) sqlite3_finalize(update_stmt);
}

int main(int argc, char *argv[])
{
	if (argc!=3){printf("usage: %s <database> <filter>\n", argv[0]); return 0;}
	char *db_path=argv[1];
	char *stmt=argv[2];

	sqlite3 *db;
	sqlite3_stmt *select_stmt = NULL;
	sqlite3_stmt *insert_stmt = NULL;
	sqlite3_stmt *update_stmt = NULL;

	if (sqlite3_open(db_path, &db)!=SQLITE_OK){fprintf(stderr, "can't open database %s\n", db_path); return 1;}
	const char *create_table_sql="create table if not exists dims(id integer primary key, width integer, height integer, n_frames integer, codec text, n_vstreams integer, n_astreams integer, foreign key(id) references files(id));"
	if (sqlite3_exec(db, create_table_sql, NULL, NULL, NULL)!=SQLITE_OK){fprintf(stderr, "can't verify 'dims' table: %s\n", sqlite3_errmsg(db)); sqlite3_close(db); return 1;}
	if (!setup_statements(db, &select_stmt, &insert_stmt, &update_stmt, scan_dir)){sqlite3_close(db); return 1;}
	while (1){
		int file_processed=process_single_file(db, select_stmt, insert_stmt, update_stmt);
		finalize_statements(select_stmt, insert_stmt, update_stmt);
    sqlite3_close(db);
    return 0;
	}
}
