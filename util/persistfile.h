#ifndef PERSISTFILE_H
#define PERSISTFILE_H

typedef struct PersistedLevelIndexFooter {
    static const unsigned char MajorVersionValue = 1;
    static const unsigned char MinorVersionValue = 0;

    int16_t       count;
    unsigned char majorVersion:4;
    unsigned char minorVersion:4;
    char          magic[3];
} PersistedLevelIndexFooter;

typedef struct {
    std::int16_t level;
    std::int16_t size;
    std::int32_t offset;
} PersistedLevelIndex;

typedef struct PersistedLevelRecord {
    static const unsigned char MajorVersionValue = 1;
    static const unsigned char MinorVersionValue = 0;

    char          magic[3];
    unsigned char majorVersion:4;
    unsigned char minorVersion:4;
    int16_t       level;
    int16_t       count;
    unsigned char moves[0];
} PersistedLevelRecord;

#define PERSISTED_INDEX_FOOTER_MAGIC_VALUE { 'L', 'T', 'i' }
#define PERSISTED_LEVEL_RECORD_MAGIC_VALUE { 'L', 'T', 'r' }

#endif // PERSISTFILE_H
