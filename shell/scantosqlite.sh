#!/usr/bin/env bash
set -euo pipefail

TARGET_DIR=""
DB_FILE=""
REPLACE_MODE=0

while getopts "p:f:r" opt; do
    case "$opt" in
        p) TARGET_DIR="$OPTARG" ;;   
        f) DB_FILE="$OPTARG" ;;   
        r) REPLACE_MODE=1 ;;      
        *) echo "Usage: $0 -p <dir_path> -f <db_path> [-r]" >&2; exit 1 ;;
    esac
done

if [[ -z "$TARGET_DIR" || -z "$DB_FILE" ]]; then
    echo "Error: Both -p (directory) and -f (database) are mandatory." >&2
    exit 1
fi

TARGET_DIR=$(readlink -f "$TARGET_DIR" 2>/dev/null || realpath "$TARGET_DIR")
mkdir -p "$(dirname "$DB_FILE")"
# ==========================================
# STEP 1: Setup and Configurations (Uses <<EOF)
# ==========================================
sqlite3 "$DB_FILE" <<EOF
PRAGMA journal_mode = WAL;
PRAGMA synchronous = NORMAL;

-- Keeps your exact target schema intact
CREATE TABLE IF NOT EXISTS files (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    path TEXT,
    name TEXT,
    size INTEGER
);

$( [[ "$REPLACE_MODE" -eq 1 ]] && echo "DELETE FROM files; DELETE FROM sqlite_sequence WHERE name='files';" )
EOF

# ==========================================
# STEP 2: The Data Pipeline (Streams SQL)
# ==========================================
# We wrap the inserts inside an explicit transaction block for maximum speed.
# awk converts the size to an integer (+0) and escapes dangerous single quotes.
find "$TARGET_DIR" -mindepth 1 -not -name '.*' -printf "%h\t%f\t%s\n" | \
awk '
    BEGIN { 
        FS = "\t"
        print "BEGIN TRANSACTION;"
    }
    {
        path = $1
        name = $2
        size = $3
        
				gsub(/\x27/, "\x27\x27", path)
				gsub(/\x27/, "\x27\x27", name)

				printf "INSERT INTO files (path, name, size) VALUES (\x27%s\x27, \x27%s\x27, %s);\n", path, name, size

    }
    END {
        print "COMMIT;"
    }
' | sqlite3 "$DB_FILE"
