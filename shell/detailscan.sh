#!/usr/bin/env bash
set -euo pipefail
#this is set up to detail the entries in /home/gm/Music/db01.sql
DB_FILE="/home/gm/Music/db01.sql"

# 1. Create the music extension table if it does not exist
sqlite3 "$DB_FILE" <<EOF
CREATE TABLE IF NOT EXISTS music_metadata (
    file_id INTEGER PRIMARY KEY,
    duration REAL,
    bitrate INTEGER,
    FOREIGN KEY (file_id) REFERENCES files(id) ON DELETE CASCADE
);
EOF

echo "Extracting audio metadata..."

# 2. Query SQLite for files that need processing.
# We fetch the id, path, and name of files that are not already in music_metadata.
sqlite3 -separator $'\t' "$DB_FILE" "
    SELECT f.id, f.path || '/' || f.name 
    FROM files f
    LEFT JOIN music_metadata m ON f.id = m.file_id
    WHERE m.file_id IS NULL 
      AND (f.name LIKE '%.mp3' OR f.name LIKE '%.flac' OR f.name LIKE '%.wav' OR f.name LIKE '%.ogg');
" | \
awk -F'\t' -v db="$DB_FILE" '
    BEGIN {
        print "BEGIN TRANSACTION;"
    }
    {
        file_id = $1
        full_path = $2
        
        # Escape single quotes in the file path for the shell execution context
        gsub(/\x27/, "\x27\\x27\x27", full_path)
        
        # Use ffprobe to get duration and bitrate in a single lightweight execution
        # Output format: duration|bitrate (e.g., 242.104312|320000)
        cmd = "ffprobe -v error -show_entries format=duration,bit_rate -of default=noprint_wrappers=1:nokey=1 \x27" full_path "\x27 2>/dev/null"
        
        duration = 0
        bitrate = 0
        
        if ((cmd | getline line) > 0) {
            duration = line
            if ((cmd | getline line) > 0) {
                bitrate = line
            }
        }
        close(cmd)
        
        # Clean up any non-numeric output to prevent SQL execution failures
        if (duration !~ /^[0-9.]+$/) duration = "NULL"
        if (bitrate !~ /^[0-9]+$/) bitrate = "NULL"
        
        # Construct the SQL statement directly in the stream
        if (duration != "NULL" || bitrate != "NULL") {
            printf "INSERT OR REPLACE INTO music_metadata (file_id, duration, bitrate) VALUES (%s, %s, %s);\n", file_id, duration, bitrate
        }
    }
    END {
        print "COMMIT;"
    }
' | sqlite3 "$DB_FILE"

echo "Metadata enrichment complete."
