/*
to compile use (assuming cd'd to this directory @ /home/gm/programs/git/easy/shell):-

> g++ -c tag_cleanser.cpp -O2 -Wall
> gcc tag_executor.c tag_cleanser.o -O2 -Wall -ltag -lstdc++ -o tag_extractor

the filescanner is a bash script that runs like
/home/gm/programs/git/easy/shell/scantosqlite02.sh -p /home/gm/Music/20260308 -f /home/gm/Music/db01.sql

and hardcoded into the secondary scanner detailscan02.sh is the variables DB_FILE="/home/gm/Music/db01.sql"
EXTRACTOR="./tag_extractor" - which writes a table called 'asset_metadata' (file_id INTEGER PRIMARY KEY,    title TEXT,    creator TEXT,    performer TEXT,    creation_date TEXT,    recording_date TEXT,    creation_location TEXT,    recording_location TEXT,    FOREIGN KEY (file_id) REFERENCES files(id) ON DELETE CASCADE) into Music/db01.sql, and fills it in as best it can with details it finds in the old files, before stripping these old tags/pictures etc out and replacing them with new custom/consistent ones...

*/

#include <stdio.h>
#include <stdlib.h>
#include "tag_cleanser.h"

int main(int argc, char *argv[]) {
    // Guard clause: ensure a target file path was supplied
    if (argc < 2) {
        fprintf(stderr, "Error: Missing target file path parameter.\n");
        return 1;
    }

    const char *target_file = argv[1];
    struct UniversalMetadata meta;

    // Run the high-performance C++ TagLib engine in RAM
    int status = extract_and_purge_asset_tags(target_file, &meta);
    if (status != 0) {
        // Return a silent error code if the file is unreadable or corrupt
        return 1;
    }

    // Output a single, perfectly uniform TSV row straight to stdout for Bash/AWK
    // If fields are completely empty, we provide a literal "NULL" string placeholder
    printf("%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
        meta.title[0]              ? meta.title              : "NULL",
        meta.creator[0]            ? meta.creator            : "NULL",
        meta.performer[0]          ? meta.performer          : "NULL",
        meta.creation_date[0]      ? meta.creation_date      : "NULL",
        meta.recording_date[0]     ? meta.recording_date     : "NULL",
        meta.creation_location[0]  ? meta.creation_location  : "NULL",
        meta.recording_location[0] ? meta.recording_location : "NULL"
    );

    return 0;
}
