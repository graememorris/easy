#!/usr/bin/env bash
set -euo pipefail

DB_FILE="/home/gm/Music/db01.sql"
EXTRACTOR="./tag_extractor"

# 1. Initialise the rich extension schema table
sqlite3 "$DB_FILE" <<EOF
PRAGMA journal_mode = WAL;
PRAGMA synchronous = NORMAL;

CREATE TABLE IF NOT EXISTS asset_metadata (
    file_id INTEGER PRIMARY KEY,
    title TEXT,
    creator TEXT,
    performer TEXT,
    creation_date TEXT,
    recording_date TEXT,
    creation_location TEXT,
    recording_location TEXT,
    FOREIGN KEY (file_id) REFERENCES files(id) ON DELETE CASCADE
);
EOF

echo "Beginning deep metadata extraction & cleansing pass..."

# 2. Stream unprocessed files out of SQLite and pipe them directly into AWK
sqlite3 -separator $'\t' "$DB_FILE" "
    SELECT f.id, f.path || '/' || f.name 
    FROM files f
    LEFT JOIN asset_metadata a ON f.id = a.file_id
    WHERE a.file_id IS NULL 
      AND (f.name LIKE '%.mp3' OR f.name LIKE '%.flac' OR f.name LIKE '%.ogg' OR f.name LIKE '%.wav');
" | \
awk -F'\t' -v db="$DB_FILE" -v bin="$EXTRACTOR" '
    BEGIN {
        print "BEGIN TRANSACTION;"
    }
    {
        file_id = $1
        full_path = $2
        
        # Escape single quotes to protect the shell command execution block
        gsub(/\x27/, "\x27\\x27\x27", full_path)
        
        # Run your compiled binary directly in-memory
        cmd = bin " \x27" full_path "\x27 2>/dev/null"
        
        if ((cmd | getline line) > 0) {
            # Split the 7 clean TSV columns produced by the C wrapper
            split(line, tags, "\t")
            
            title     = tags[1]
            creator   = tags[2]
            performer = tags[3]
            c_date    = tags[4]
            r_date    = tags[5]
            c_loc     = tags[6]
            r_loc     = tags[7]
            
            # Escape internal single quotes to make strings completely SQL-injection safe
            gsub(/\x27/, "\x27\x27", title)
            gsub(/\x27/, "\x27\x27", creator)
            gsub(/\x27/, "\x27\x27", performer)
            gsub(/\x27/, "\x27\x27", c_date)
            gsub(/\x27/, "\x27\x27", r_date)
            gsub(/\x27/, "\x27\x27", c_loc)
            gsub(/\x27/, "\x27\x27", r_loc)
            
            # Convert text "NULL" flags back to native unquoted SQL NULL datatypes
            t_val  = (title     == "NULL") ? "NULL" : "\x27" title "\x27"
            c_val  = (creator   == "NULL") ? "NULL" : "\x27" creator "\x27"
            p_val  = (performer == "NULL") ? "NULL" : "\x27" performer "\x27"
            cd_val = (c_date    == "NULL") ? "NULL" : "\x27" c_date "\x27"
            rd_val = (r_date    == "NULL") ? "NULL" : "\x27" r_date "\x27"
            cl_val = (c_loc     == "NULL") ? "NULL" : "\x27" c_loc "\x27"
            rl_val = (r_loc     == "NULL") ? "NULL" : "\x27" r_loc "\x27"
            
            # Build the atomic operational insert statement
            printf "INSERT OR REPLACE INTO asset_metadata (file_id, title, creator, performer, creation_date, recording_date, creation_location, recording_location) VALUES (%s, %s, %s, %s, %s, %s, %s, %s);\n", file_id, t_val, c_val, p_val, cd_val, rd_val, cl_val, rl_val
        }
        close(cmd)
    }
    END {
        print "COMMIT;"
    }
' | sqlite3 "$DB_FILE"

echo "Deep scan and data purge completed successfully."
