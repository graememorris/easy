#ifndef TAG_CLEANSER_H
#define TAG_CLEANSER_H

#ifdef __cplusplus
extern "C" {
#endif

struct UniversalMetadata {
    char title[256];
    char creator[256];
    char performer[256];
    char creation_date[64];
    char recording_date[64];
    char creation_location[256];
    char recording_location[256];
};

// Declare the function so main.c can read it natively
int extract_and_purge_asset_tags(const char *filepath, struct UniversalMetadata *out_meta);

#ifdef __cplusplus
}
#endif

#endif
