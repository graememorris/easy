#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <db.h>

typedef struct {
    int is_directory;
} FileMeta;

// We change the argument signature to accept your network socket descriptor: ns
void stream_bdb_to_socket(DB *dbp, int ns) {
    DBC *dbcp = NULL;
    DBT key, data;
    int ret;

    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));

    if ((ret = dbp->cursor(dbp, NULL, &dbcp, 0)) != 0) {
        // Log locally to the server's stderr if the cursor fails
        dbp->err(dbp, ret, "DB->cursor failed");
        return;
    }

    // 1. Send the starting JSON array bracket directly down the socket
    dprintf(ns, "[\n");
    int first_item = 1;

    while ((ret = dbcp->get(dbcp, &key, &data, DB_NEXT)) == 0) {
        if (key.size == 0 || data.size == 0) continue;

        char *item_name = (char *)key.data;
        size_t name_len = key.size;
        FileMeta *meta = (FileMeta *)data.data;

        if (!first_item) {
            dprintf(ns, ",\n");
        }
        first_item = 0;

        // 2. Stream the JSON item structure block directly across the network wire
        dprintf(ns, "  {\n");
        dprintf(ns, "    \"name\": \"%.*s\",\n", (int)name_len, item_name);
        dprintf(ns, "    \"type\": \"%s\"\n", (meta->is_directory) ? "directory" : "file");
        dprintf(ns, "  }");
    }

    // 3. Send the final closing JSON array bracket to conclude the transmission
    dprintf(ns, "\n]\n");

    // Cleanup cursor resource
    if (dbcp != NULL) {
        dbcp->close(dbcp);
    }

    if (ret != DB_NOTFOUND) {
        dbp->err(dbp, ret, "Cursor scanning error encountered");
    }
}
