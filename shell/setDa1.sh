#!/usr/bin/env bash
set -euo pipefail

# Default values
TARGET_DIR=""
CREATE_LIST=""
DELETE_LIST=""

# 1. Parse Command Line Arguments using getopts
while getopts "p:c:d:" opt; do
    case "$opt" in
        p) TARGET_DIR="$OPTARG" ;;   
        c) CREATE_LIST="$OPTARG" ;;   
        d) DELETE_LIST="$OPTARG" ;;   
        *) 
            echo "Usage: $0 -p <dir_path> [-c file1,file2] [-d file3,file4]" >&2
            exit 1
            ;;
    esac
done

# Validate that a directory path was provided
if [[ -z "$TARGET_DIR" ]]; then
    echo "Error: You must specify a target directory using the -p flag." >&2
    exit 1
fi

# Resolve TARGET_DIR to an absolute canonical path to prevent symlink bypass tricks
TARGET_DIR=$(readlink -f "$TARGET_DIR" 2>/dev/null || realpath "$TARGET_DIR")

# Ensure the target directory exists and is readable/writable
if [[ ! -d "$TARGET_DIR" ]]; then
    echo "Error: Directory '$TARGET_DIR' does not exist." >&2
    exit 1
fi

# Helper function to split comma-separated strings safely into an array
parse_csv_list() {
    local raw_string="$1"
    IFS=',' read -ra elements <<< "$raw_string"
    for item in "${elements[@]}"; do
        local cleaned
        cleaned=$(echo "$item" | xargs)
        if [[ -n "$cleaned" ]]; then
            echo "$cleaned"
        fi
    done
}

# 2. Execute Deletions Only If Requested
if [[ -n "$DELETE_LIST" ]]; then
    if [[ ! -w "$TARGET_DIR" ]]; then
        echo "Error: Directory '$TARGET_DIR' is not writable; cannot delete items." >&2
        exit 1
    fi
    readarray -t FILES_TO_DELETE < <(parse_csv_list "$DELETE_LIST")
    for file in "${FILES_TO_DELETE[@]}"; do
        # Strip trailing slash if present to avoid breaking path resolution
        CLEANED_FILE="${file%/}"
        RAW_PATH="$TARGET_DIR/$CLEANED_FILE"
        FULL_PATH=$(readlink -f "$RAW_PATH" 2>/dev/null || realpath -m "$RAW_PATH")
        
        # --- CRITICAL SAFETY GUARDS ---
        
        # Guard A: Prevent deleting root or core system folders if a relative path escapes via "../"
        case "$FULL_PATH" in
            ""|"/"|"/boot"|"/dev"|"/etc"|"/home"|"/proc"|"/sys"|"/usr"|"/var"|"/bin"|"/sbin"|"/lib"|"/lib64")
                echo "Security Error: Destructive action blocked on system critical directory: $FULL_PATH" >&2
                exit 1
                ;;
        esac

        # Guard B: Prevent deleting the target root directory itself
        if [[ "$FULL_PATH" == "$TARGET_DIR" ]]; then
            echo "Security Error: Blocked attempt to delete the root TARGET_DIR: $FULL_PATH" >&2
            exit 1
        fi

        # Guard C: Robust Path Traversal Check (Ensures FULL_PATH begins with TARGET_DIR)
        if [[ "$FULL_PATH" != "$TARGET_DIR"* ]]; then
            echo "Security Error: Blocked path traversal attempt outside TARGET_DIR: $FULL_PATH" >&2
            exit 1
        fi
        
        # --- END OF SAFETY GUARDS ---

        # Crucial Guard: ONLY delete regular files, never directories, never recursively
        if [[ -f "$FULL_PATH" ]]; then
            rm "$FULL_PATH"
        fi
    done
fi

# 3. Execute Creations Only If Requested
if [[ -n "$CREATE_LIST" ]]; then
    if [[ ! -w "$TARGET_DIR" ]]; then
        echo "Error: Directory '$TARGET_DIR' is not writable; cannot create items." >&2
        exit 1
    fi
    readarray -t FILES_TO_CREATE < <(parse_csv_list "$CREATE_LIST")
    for file in "${FILES_TO_CREATE[@]}"; do
        FULL_PATH="$TARGET_DIR/$file"
        
        if [[ "$file" == */ ]]; then
            if [[ ! -d "$FULL_PATH" ]]; then
                mkdir -p "$FULL_PATH"
            fi
        else
            PARENT_DIR=$(dirname "$FULL_PATH")
            if [[ ! -d "$PARENT_DIR" ]]; then
                mkdir -p "$PARENT_DIR"
            fi
            if [[ ! -e "$FULL_PATH" ]]; then
                touch "$FULL_PATH"
            fi
        fi
    done
fi

# 4. Stream Raw HTML List
find "$TARGET_DIR" -maxdepth 1 -mindepth 1 -not -name '.*' -printf "%P\t%Y\n" | sort -f | awk 'BEGIN{FS="\t"} {printf("<div>%s%s</div>", $1, ($2=="d" || $2=="l") ? "/" : "");}'
