#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tstring.h>
#include <taglib/tpropertymap.h> // REQUIRED for PropertyMap definition
#include <cstdio>
#include <cstring>
#include "tag_cleanser.h"

extern "C" int extract_and_purge_asset_tags(const char *filepath, struct UniversalMetadata *out_meta) {
    // 1. Initialise the C struct strings to empty values
    std::memset(out_meta, 0, sizeof(struct UniversalMetadata));

    // 2. Open file via auto-detection (ogg, flac, mp3, wav, aac)
    TagLib::FileRef f(filepath);
    if(f.isNull() || !f.file() || !f.tag()) return 1;

    // 3. Extract existing tag properties using the multi-format map
    TagLib::PropertyMap props = f.file()->properties();

    std::string title     = !props["TITLE"].isEmpty()     ? props["TITLE"].front().to8Bit(true)     : "";
    std::string performer = !props["ARTIST"].isEmpty()    ? props["ARTIST"].front().to8Bit(true)    : "";
    std::string creator   = !props["COMPOSER"].isEmpty()  ? props["COMPOSER"].front().to8Bit(true)  : "";
    std::string rec_date   = !props["DATE"].isEmpty()      ? props["DATE"].front().to8Bit(true)      : "";
    std::string creat_date = !props["COMPOSITION_DATE"].isEmpty() ? props["COMPOSITION_DATE"].front().to8Bit(true) : "";
    std::string rec_loc   = !props["LOCATION"].isEmpty()  ? props["LOCATION"].front().to8Bit(true)  : "";
    std::string creat_loc = !props["COMPOSITION_LOCATION"].isEmpty() ? props["COMPOSITION_LOCATION"].front().to8Bit(true) : "";

    // Fallback: If custom creator/composer tag is missing, clone artist properties
    if (creator.empty() && !performer.empty()) {
        creator = performer;
    }

    // 4. PURGE AND WRITEBACK: 
    // Creating an completely empty PropertyMap and assigning it to the file
    // removes all standard tags, custom tags, and embedded audio properties natively.
    TagLib::PropertyMap empty_props;
    f.file()->setProperties(empty_props);

    // Rebuild only your clean custom keys
    TagLib::PropertyMap clean_props;
    if(!title.empty())     clean_props["TITLE"]                = TagLib::String(title,     TagLib::String::UTF8);
    if(!performer.empty()) clean_props["ARTIST"]               = TagLib::String(performer, TagLib::String::UTF8);
    if(!creator.empty())   clean_props["COMPOSER"]             = TagLib::String(creator,   TagLib::String::UTF8);
    if(!rec_date.empty())   clean_props["DATE"]                 = TagLib::String(rec_date,   TagLib::String::UTF8);
    if(!creat_date.empty()) clean_props["COMPOSITION_DATE"]    = TagLib::String(creat_date, TagLib::String::UTF8);
    if(!rec_loc.empty())   clean_props["LOCATION"]             = TagLib::String(rec_loc,   TagLib::String::UTF8);
    if(!creat_loc.empty()) clean_props["COMPOSITION_LOCATION"] = TagLib::String(creat_loc, TagLib::String::UTF8);

    // Apply the clean configuration maps and save changes to disk
    f.file()->setProperties(clean_props);
    f.file()->save();

    // 5. Safely copy inside the memory bounds allocated by the C struct
    std::snprintf(out_meta->title,              sizeof(out_meta->title),              "%s", title.c_str());
    std::snprintf(out_meta->creator,            sizeof(out_meta->creator),            "%s", creator.c_str());
    std::snprintf(out_meta->performer,          sizeof(out_meta->performer),          "%s", performer.c_str());
    std::snprintf(out_meta->creation_date,      sizeof(out_meta->creation_date),      "%s", creat_date.c_str());
    std::snprintf(out_meta->recording_date,     sizeof(out_meta->recording_date),     "%s", rec_date.c_str());
    std::snprintf(out_meta->creation_location,  sizeof(out_meta->creation_location),  "%s", creat_loc.c_str());
    std::snprintf(out_meta->recording_location, sizeof(out_meta->recording_location), "%s", rec_loc.c_str());

    return 0;
}
